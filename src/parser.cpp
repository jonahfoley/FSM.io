#include "../include/parser.hpp"
#include "../include/FSM_elements.hpp"
#include "../include/tree.hpp"
#include "../include/utility.hpp"

#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <cassert>
#include <utility>
#include <algorithm>

#include <zlib.h>
#include <curl/curl.h>
#include "tinyxml2.h"
#include "fmt/format.h"

namespace parser
{
    // namespaces aliases
    using namespace tinyxml2;
    namespace views  = std::views;
    namespace ranges = std::ranges;

    // type aliases
    using TransitionTree = utility::binary_tree<FSMTransition>;
    using TransitionNode = utility::Node<FSMTransition>;
    using TransitionMatrix  = std::vector<std::vector<std::optional<bool>>>;

    // helper functions
    namespace helpers {
        static auto to_bool(std::string_view str) 
        {
            return str == "1" | str == "True" | str == "true";
        }

        // checks if a given xmCell element is a given type based on its "style"
        static auto elem_is_type(XMLElement *element, std::string_view test_type) -> bool
        {
            if(auto style = element->Attribute("style"); style != nullptr)
            {
                auto res = std::string_view{style} 
                    | views::split(';') 
                    | ranges::views::filter([test_type](auto &&rng) {
                        auto actual_type = std::string_view(&*rng.begin(), ranges::distance(rng));
                        return actual_type == test_type;
                    });
                return !res.empty();
            }
            return false;             
        };

        // specialisations of elem type checked lambda
        static auto is_predicate(XMLElement* element)
        { 
            return elem_is_type(element, "rhombus");
        };

        static auto is_arrow(XMLElement* element)
        { 
            return elem_is_type(element, "edgeStyle=orthogonalEdgeStyle");
        };

        static auto is_state(XMLElement* element)
        { 
            return !is_predicate(element) & !is_arrow(element); 
        };
    }

    // handle errors during parsing and token generation
    void HandleParseError(const ParseError err)
    {   
        switch(err)
        {
            case ParseError::InvalidDrawioFile:
                throw std::runtime_error("you provided an invalid drawio file!");
                break;
            case ParseError::DecisionPathError:
                throw std::runtime_error("you have an unrouted decision block");
                break;
            default:
                throw std::runtime_error("oh no! anyway..");
                break;
        }
    }

    auto extract_encoded_drawio(std::string_view file_name) -> tl::expected<std::string, ParseError>
    {
        XMLDocument doc;
        doc.LoadFile(file_name.data());

        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidDrawioFile);
        }

        XMLElement *pRootElement = doc.RootElement();
        if (pRootElement != nullptr)
        {
            auto *pDiagram = pRootElement->FirstChildElement("diagram");
            if (pDiagram != nullptr)
            {
                return pDiagram->GetText();
            }
        }
        return tl::unexpected<ParseError>(ParseError::ExtractingDrawioString);
    }

    auto inflate(std::string_view str) -> tl::expected<std::string, ParseError>
    {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
            return tl::unexpected<ParseError>(ParseError::InflationError);

        zs.next_in = (Bytef *)str.data();
        zs.avail_in = str.size();

        int ret;
        char outbuffer[32768];
        std::string outstring;

        do
        {
            zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
            zs.avail_out = sizeof(outbuffer);
            ret = inflate(&zs, Z_NO_FLUSH);
            if (outstring.size() < zs.total_out)
            {
                outstring.append(outbuffer, zs.total_out - outstring.size());
            }

        } while (ret == Z_OK);

        inflateEnd(&zs);
        if (ret != Z_STREAM_END)
        {
            return tl::unexpected<ParseError>(ParseError::InflationError);
        }

        return outstring;
    }

    auto base64_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>
    {

        std::string out;
        std::unordered_map<unsigned char, int> T;
        for (int i = 0; i < 64; i++)
            T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

        int val = 0, valb = -8;
        for (unsigned char c : encoded_str)
        {
            if (!T.contains(c))
                break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0)
            {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    auto url_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>
    {
        CURL *curl = curl_easy_init();
        int outlen;
        if (curl)
        {
            char *decoded = curl_easy_unescape(curl, encoded_str.data(), encoded_str.length(), &outlen);
            if (decoded)
            {
                std::string decoded_str(decoded, outlen);
                curl_free(decoded);
                curl_easy_cleanup(curl);
                return decoded_str;
            }
        }
        return tl::unexpected<ParseError>(ParseError::URLDecodeError);
    }

    auto drawio_to_tokens(std::string_view drawio_xml_str) 
        -> tl::expected<std::tuple<
            std::vector<FSMState>,
            std::vector<FSMPredicate>,
            std::vector<FSMArrow>
            >, ParseError>
    {
        XMLDocument doc;
        doc.Parse(drawio_xml_str.data());
        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidDrawioFile);
        }

        XMLElement *pGraphModel = doc.RootElement();
        std::vector<FSMToken> toks;

        if (pGraphModel)
        {
            XMLElement *pRoot = pGraphModel->FirstChildElement("root");
            if(pRoot) 
            {   
                // extract all the cells from the diagram
                XMLElement *pCell = pRoot->FirstChildElement("mxCell");
                std::vector<XMLElement *> elements;
                while (pCell)
                {
                    if (pCell->Attribute("style")) 
                    {
                        elements.push_back(pCell);
                    }
                    pCell = pCell->NextSiblingElement("mxCell");
                }

                // construct the FSMStates and copy into output tokens
                auto states = elements 
                    | views::filter(helpers::is_state)
                    | views::transform([](XMLElement *el) { 
                        std::vector<std::string> outputs;
                        ranges::copy(
                            views::transform(
                                std::string_view{el->Attribute("value")} | views::split(std::string_view{"<br>"}),
                                [](auto &&rng) { return std::string(&*rng.begin(), ranges::distance(rng)); }
                            ),
                            std::back_inserter(outputs)
                        );
                        return FSMState{
                            el->Attribute("id"),
                            outputs
                        }; 
                    })
                    | utility::to<std::vector<FSMState>>();

                // we later index the decision blocks - which cant be done with a ranges::filter_view
                // so copy into a sector
                auto predicates = elements 
                    | views::filter(helpers::is_predicate)
                    | views::transform([](XMLElement *el) { 
                        return FSMPredicate{
                            el->Attribute("id"),
                            el->Attribute("value")
                        }; 
                    })
                    | utility::to<std::vector<FSMPredicate>>();
                
                // a range of FSMArrows
                auto arrows = elements 
                    | views::filter(helpers::is_arrow)
                    | views::transform([](XMLElement *el) { 
                        FSMArrow arrow{
                            el->Attribute("id"),
                            el->Attribute("source"), 
                            el->Attribute("target")
                        };
                        // if the arrow is relating toa decision block, it'll have a value
                        const char* pValue = el->Attribute("value");
                        if (pValue) arrow.m_value = helpers::to_bool(pValue);
                        return arrow;
                    })
                    | utility::to<std::vector<FSMArrow>>();

                return std::make_tuple(states,predicates,arrows);
            }
        }
        return tl::unexpected<ParseError>(ParseError::InvalidDrawioFile);
    }
}