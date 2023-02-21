#include "../include/parser.hpp"
#include "../include/FSM_elements.hpp"
#include "../include/tree.hpp"
#include "../include/utility.hpp"
#include "../include/ranges_helpers.hpp"

#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <cassert>
#include <utility>
#include <algorithm>
#include <regex>

#include <zlib.h>
#include <curl/curl.h>
#include <tinyxml2.h>
#include <fmt/format.h>

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
    namespace helpers 
    {
        static auto to_bool(std::string_view str) 
        {
            // TODO: transform to lower first to reduce ammount of checks..
            if (str == "1" | str == "True" | str == "true" | str == "Yes" | str == "yes") 
            {
                return true;
            } 
            else if (str == "0" | str == "False" | str == "false" | str == "No" | str == "no" )
            {
                return false;
            }
            else 
            {
                return false; // TODO: proper error resolution here
            }
        }

        // checks if a given xmCell element is a given type based on its "style"
        static auto elem_is_type(XMLElement *element, std::string_view test_type) -> bool
        {
            if(auto style = element->Attribute("style"); style != nullptr)
            {
                auto res = std::string_view{style} 
                    | views::split(';') 
                    | ranges::views::filter([test_type](auto &&r) {
                        auto actual_type = std::string_view(r.begin(), r.end());
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

        static auto is_text(XMLElement* element)
        { 
            return elem_is_type(element, "text");
        };

        static auto is_state(XMLElement* element)
        { 
            return !is_predicate(element) & !is_arrow(element) & !is_text(element); 
        };
    }

    auto extract_encoded_drawio(const std::filesystem::path& path) -> tl::expected<std::string, ParseError>
    {
        if (path.empty())
        {
            return tl::unexpected<ParseError>(ParseError::EmptyPath);
        }

        XMLDocument doc;
        doc.LoadFile(path.c_str());

        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidEncodedDrawioFile);
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

    auto drawio_to_tokens(std::string_view drawio_xml_str) -> tl::expected<parser::token_tuple, ParseError>
    {
        XMLDocument doc;
        doc.Parse(drawio_xml_str.data());
        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidDecodedDrawioFile);
        }

        XMLElement *origin = doc.RootElement();
        std::vector<FSMToken> toks;

        if (origin)
        {
            XMLElement *pRoot = origin->FirstChildElement("root");
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
                        /*
                            the state has a value of the form either:
                            (1) $STATE=xyz;                    // explicit state name, no outputs
                            (2) $OUTPUTS={a,b,c,d};            // implicity inferred state name
                            (3) {a,b,c,d}                      // implicity inferred outputs and state name
                            (4) $STATE=xyz;$OUTPUTS={a,b,c,d}; // explicit state name
                            (5) __                             // no outputs, implicity inferred state name
                        */

                        // sanitise the input token of all font, size, and line breaks
                        std::string value{el->Attribute("value")};
                        value = std::regex_replace(value, std::regex("<[^>]*>"), "");

                        auto toks = value
                            | views::split(';')
                            | views::transform([](auto&& r) { return std::string_view(r.begin(), r.end()); });

                        auto name = toks
                            | views::filter([](std::string_view tok){ 
                                return std::regex_match(std::string(tok), std::regex("\\$STATE=[A-Za-z0-9_@./#&+-]+")); 
                            })
                            | views::transform([](std::string_view tok) {
                                tok.remove_prefix(std::min(tok.find_first_of("=")+1, tok.size()));
                                return tok;
                            });
                        
                        auto outputs = toks
                            | views::filter([](std::string_view tok){ 
                                return std::regex_match(std::string(tok), 
                                    std::regex("\\$OUTPUTS=\\{[A-Za-z0-9,_@./#&+-]+\\}|\\{[A-Za-z0-9,_@./#&+-]+\\}"));
                            })
                            | views::transform([](std::string_view tok) {
                                tok.remove_prefix(std::min(tok.find_first_of("{")+1, tok.size()));
                                tok.remove_suffix(std::min(tok.size()-tok.find_first_of("}"), tok.size()));
                                return tok 
                                    | views::split(',') 
                                    | views::transform([](auto r){ return std::string(r.begin(), r.end()); })
                                    | utility::to<std::vector<std::string>>();
                            });

                        if (name.empty()) // implicitly generated name
                        {
                            return FSMState{
                                el->Attribute("id"),
                                outputs.front()
                            };
                        }
                        else
                        {
                            return FSMState{
                                el->Attribute("id"),
                                name.front(),
                                outputs.front()
                            };
                        }
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
                        auto* pValue = el->Attribute("value");
                        if (pValue) arrow.m_value = helpers::to_bool(pValue);
                        return arrow;
                    })
                    | utility::to<std::vector<FSMArrow>>();

                return std::make_tuple(states,predicates,arrows);
            }
        }
        return tl::unexpected<ParseError>(ParseError::InvalidDecodedDrawioFile);
    }

    // handle errors during parsing and token generation
    void HandleParseError(const ParseError err)
    {   
        switch(err)
        {
            case ParseError::EmptyPath:
                throw std::runtime_error(
                    "<EMPTY PATH> you provided an empty path to the draw.io diagram");
                break;
            case ParseError::InvalidEncodedDrawioFile:
                throw std::runtime_error(
                    "<INVALID ENCODED DRAWIO FILE ERROR> : you provided an invalid drawio file!");
                break;
            case ParseError::ExtractingDrawioString:
                throw std::runtime_error(
                    "<EXTRACT STRING ERROR> : Could not extract encoded Draw.IO string"
                    "- are you sure you exported the XML in encoded format?");
                break;
            case ParseError::URLDecodeError:
                throw std::runtime_error(
                    "<URL DECODE ERROR> : Could not decode the Draw.IO diagram"
                    "- are you sure you exported the XML in encoded format?");
                break;
            case ParseError::Base64DecodeError:
                throw std::runtime_error(
                    "<BASE64 DECODE ERROR> : Could not decode the Draw.IO diagram"
                    "- are you sure you exported the XML in encoded format?");
                break;
            case ParseError::InflationError:
                throw std::runtime_error(
                    "<INFLATION DECODE ERROR> : Could not decode the Draw.IO diagram"
                    "- are you sure you exported the XML in encoded format?");
                break;
            case ParseError::DrawioToToken:
                throw std::runtime_error(
                    "<DRAWIO TO TOKEN ERROR> : Could not transform the decoded Draw.IO file to a set of token"
                    "- are you sure that you have used *only* rhombus', rectangles, and arrow elements?");
                break;
            case ParseError::DecisionPathError:
                throw std::runtime_error(
                    "<DECISION PATH ERROR> : You have an unrouted decision block");
                break;
            case ParseError::InvalidDecodedDrawioFile:
                throw std::runtime_error(
                    "<INVALID DECODED DRAWIO FILE ERROR> : decoded Draw.IO"
                    "file was invalid");
                break;
            default:
                throw std::runtime_error(
                    "Something unexpected went wrong ... try again.");
                break;
        }
    }
}