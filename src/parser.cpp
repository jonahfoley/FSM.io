#include "../include/parser.hpp"
#include "../include/FSM_elements.hpp"

#include <iostream>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <ranges>

#include <zlib.h>
#include <curl/curl.h>
#include "tinyxml2.h"
#include "fmt/format.h"

namespace parser
{
    using namespace tinyxml2;
    namespace views  = std::views;
    namespace ranges = std::ranges;

    void HandleParseError(const ParseError err)
    {
        if (err == ParseError::InvalidDrawioFile)
            throw std::runtime_error("you provided an invalid drawio file!");
        else
            throw std::runtime_error("oh no! anyway..");
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

    auto drawio_to_tokens(std::string_view drawio_xml_str) -> tl::expected<std::vector<FSMToken>, ParseError>
    {
        XMLDocument doc;
        doc.Parse(drawio_xml_str.data());
        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidDrawioFile);
        }

        auto elem_is_type = [](XMLElement *element, std::string_view test_type) -> bool
        {
            auto actual_type = std::string_view{element->Attribute("style")} 
                | views::split(';') 
                | ranges::views::filter([test_type](auto &&rng) {
                    auto actual_type = std::string_view(&*rng.begin(), ranges::distance(rng));
                    return actual_type == test_type;
                });
            return !actual_type.empty();
        };

        // specialisations of elem type checked lambda
        auto is_state = [&elem_is_type](XMLElement* element){ return elem_is_type(element, "rounded=0");};
        auto is_decision = [&elem_is_type](XMLElement* element){ return elem_is_type(element, "rhombus");};
        auto is_arrow = [&elem_is_type](XMLElement* element){ return elem_is_type(element, "edgeStyle=orthogonalEdgeStyle");};

        XMLElement *pRootElement = doc.RootElement();
        std::vector<FSMToken> toks;

        if (pRootElement != nullptr)
        {
            XMLElement *pCell = pRootElement->FirstChildElement("mxCell");
            std::vector<XMLElement *> elements;
            while (pCell)
            {
                const char* dummy;
                if (pCell->QueryAttribute("style", &dummy) == XML_SUCCESS)
                {
                    elements.push_back(pCell);
                }
                pCell = pCell->NextSiblingElement("mxCell");
            }

            // a range of FSMStates's
            auto states = elements 
                | views::filter(is_state)
                | views::transform([](XMLElement *el) { 
                    auto outputs = std::string_view{el->Attribute("value")} 
                        | views::split(std::string_view{"&lt;br&gt;"})
                        | ranges::views::transform([](auto &&rng) {
                            return std::string_view(&*rng.begin(), ranges::distance(rng)); // construct sv from range (eugh!)
                        });
                    return FSMState{el->Attribute("id"), std::vector<std::string>(outputs.begin(), outputs.end())}; 
                });

            // a range of FSMDecision's
            auto decisions = elements 
                | views::filter(is_decision)
                | views::transform([](XMLElement *el) { 
                    return FSMDecision{el->Attribute("id"), el->Attribute("value")}; 
                });

            // a range of FSMArrow's
            auto arrows = elements 
                | std::views::filter(is_arrow)
                | views::transform([](XMLElement *el) { 
                    return FSMArrow{el->Attribute("id"), el->Attribute("source"), el->Attribute("target")}; 
                });

            // copy the state elements into toks
            ranges::copy(states, std::back_inserter(toks));
            ranges::copy(decisions, std::back_inserter(toks));
            ranges::copy(arrows, std::back_inserter(toks));

            return toks;
        }
        return tl::unexpected<ParseError>(ParseError::InvalidDrawioFile);
    }
}