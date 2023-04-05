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
    namespace views = std::views;
    namespace ranges = std::ranges;

    // type aliases
    using TransitionTree = utility::binary_tree<FSMTransition>;
    using TransitionNode = utility::Node<FSMTransition>;
    using TransitionMatrix = std::vector<std::vector<std::optional<bool>>>;

    // helper functions
    namespace helpers
    {
        static auto to_bool(std::string_view str) -> tl::expected<bool, ParseError>
        {
            const static auto true_tokens  = {"1", "yes", "Yes", "True", "true"};
            const static auto false_tokens = {"0", "no", "No", "False", "false"};

            if (ranges::find(true_tokens, str) != true_tokens.end())
            {
                return true;
            }
            else if(ranges::find(false_tokens, str) != false_tokens.end())
            {
                return false;
            }   
            else
            {
                return tl::unexpected<ParseError>(ParseError::InvalidBooleanSpecifier);     
            } 
        }

        // checks if a given xmCell element is a given type based on its "style"
        static auto elem_is_type(XMLElement *element, std::string_view test_type) -> bool
        {
            if (auto style = element->Attribute("style"); style != nullptr)
            {
                auto res = std::string_view{style} | views::split(';') | ranges::views::filter([test_type](auto &&r)
                                                                                               {
                        auto actual_type = std::string_view(r.begin(), r.end());
                        return actual_type == test_type; });
                return !res.empty();
            }
            return false;
        };

        static auto sanitise(std::string_view s)
        {
            return std::regex_replace(std::string(s), std::regex("<[^>]*>"), "");
        }

        template <typename T>
        static auto has_error(const tl::expected<T, ParseError> &e)
        {
            return !e.has_value();
        }

        // specialisations of elem type checked lambda
        static auto is_predicate(XMLElement *element)
        {
            return elem_is_type(element, "rhombus");
        };

        static auto is_arrow(XMLElement *element)
        {
            return elem_is_type(element, "edgeStyle=orthogonalEdgeStyle");
        };

        static auto is_text(XMLElement *element)
        {
            return elem_is_type(element, "text");
        };

        static auto is_state(XMLElement *element)
        {
            return !is_predicate(element) & !is_arrow(element) & !is_text(element);
        };
    }

    auto extract_encoded_drawio(const std::filesystem::path &path) -> tl::expected<std::string, ParseError>
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

    static auto states_from_xml_elements(const std::vector<XMLElement *> &elements)
        -> tl::expected<std::vector<FSMState>, ParseError>
    {
        auto to_FSMState = [](XMLElement *el) -> tl::expected<FSMState, ParseError>
        {
            using namespace std::literals;
            /*
            Valid expressions are
                (1) $STATE=xyz;
                (2) $OUTPUTS={a,b,c,d};
                (3) {a,b,c,d}
                (4) $DEFAULT};e
                (5) __
            Which are ; delimited, in any order
            */

            // sanitise the input token of all font, size, and line breaks
            std::string value{helpers::sanitise(el->Attribute("value"))};
            auto toks = value 
                | views::split(';') 
                | views::transform([](auto r){ return std::string_view(r.begin(), r.end()); });

            // get the state name token (if it exists)
            std::string name;
            auto state_match = [](std::string_view tok) { 
                return std::regex_match(std::string(tok), std::regex("\\$STATE=[A-Za-z0-9_@./#&+-]+")); 
            };
            if (auto name_tok = ranges::find_if(toks, state_match); name_tok != toks.end())
            {
                std::string_view name_tok_v{*name_tok};
                name_tok_v.remove_prefix(std::min(name_tok_v.find_first_of("=") + 1, name_tok_v.size()));
                name = name_tok_v;
            }

            // get the outputs string (i.e. OutputA,OutputB,...) (if it exists)
            std::vector<std::string> outputs;
            auto outputs_match = [](std::string_view tok) { 
                return std::regex_match(std::string(tok), 
                    std::regex("\\$OUTPUTS=\\{[A-Za-z0-9,_@./#&+-]+\\}|\\{[A-Za-z0-9,_@./#&+-]+\\}")); 
            };
            if (auto outputs_tok = ranges::find_if(toks, outputs_match); outputs_tok != toks.end())
            {
                std::string_view outputs_tok_v{*outputs_tok};
                outputs_tok_v.remove_prefix(std::min(outputs_tok_v.find("{"sv) + 1, outputs_tok_v.size()));
                outputs_tok_v.remove_suffix(std::max(outputs_tok_v.size() - outputs_tok_v.find_first_of("}"), std::size_t(0)));
                outputs = outputs_tok_v 
                    | views::split(',') 
                    | views::transform([](auto r){ return std::string(r.begin(), r.end()); }) 
                    | utility::to<std::vector<std::string>>();
            }

            bool is_default_state = ranges::find(toks, "$DEFAULT") != toks.end();
            std::string id{helpers::sanitise(el->Attribute("id"))};

            if (outputs.empty())
            {
                if (name.empty())
                {
                    return FSMState(id, is_default_state);
                }
                else
                {
                    return FSMState(id, name, is_default_state);
                }
            }
            else
            {
                if (name.empty())
                {
                    return FSMState(id, outputs, is_default_state);
                }
                else
                {
                    return FSMState(id, name, outputs, is_default_state);
                }
            }
        };

        // construct the FSMStates and copy into output tokens
        auto states = elements | views::filter(helpers::is_state) | views::transform(to_FSMState);

        return utility::to_expected(states);
    }

    static auto predicates_from_xml_elements(const std::vector<XMLElement *> &elements)
        -> tl::expected<std::vector<FSMPredicate>, ParseError>
    {

        auto to_predicate = [](XMLElement *el) -> tl::expected<FSMPredicate, ParseError>
        {
            /*
            Predicates are of the form:
                (1) X                 // if X == 1 -> true path, else false path
                (2) X <comparator> 1  // if X <comparator> 1 -> true path, else false path
            where
            <comparator> \in {==, !=, <, <=, >, >=}
            */
            std::string value = helpers::sanitise(el->Attribute("value"));
            std::smatch pieces_match;
            if (std::regex_match(value, pieces_match,
                                 std::regex("[A-Za-z0-9_@./#&+-]+(==|!=|>|>=|<|<=)[A-Za-z0-9'_@./#&+-]+")))
            {
                // pieces match holds two things: <var><comparator><value>, <comparator>
                // we want to split value on the second, i.e. <comparator>
                std::string comparator{pieces_match[1].str()};
                auto toks = value 
                    | views::split(comparator) 
                    | views::transform([](auto r){ return std::string_view(r.begin(), r.end()); }) 
                    | utility::to<std::vector<std::string_view>>();

                return FSMPredicate(
                    helpers::sanitise(el->Attribute("id")),
                    toks[0],
                    comparator,
                    toks[1]
                );
            }
            else if (std::regex_match(value, std::regex("[A-Za-z0-9_@./#&+-]+")))
            {
                return FSMPredicate(
                    helpers::sanitise(el->Attribute("id")),
                    value
                );
            }
            else
            {
                return tl::unexpected<ParseError>(ParseError::IncorrectPredicateFormat);
            }
        };

        auto predicates = elements | views::filter(helpers::is_predicate) | views::transform(to_predicate);

        return utility::to_expected(predicates);
    }

    static auto arrows_from_xml_elements(const std::vector<XMLElement *> &elements)
        -> tl::expected<std::vector<FSMArrow>, ParseError>
    {
        auto to_arrow = [](XMLElement *el) -> tl::expected<FSMArrow, ParseError>
        {
            auto pSource = el->Attribute("source");
            if (!pSource)
            {
                return tl::unexpected<ParseError>(ParseError::MissingSourceArrow);          
            }
                
            auto pTarget = el->Attribute("target");
            if (!pTarget)
            {
                return tl::unexpected<ParseError>(ParseError::MissingTargetArrow);
            }

            FSMArrow arrow(
                helpers::sanitise(el->Attribute("id")),
                helpers::sanitise(pSource),
                helpers::sanitise(pTarget)
            );

            // if the arrow is relating toa decision block, it'll have a value
            auto pValue = el->Attribute("value");
            if (pValue)
            {
                auto b = helpers::to_bool(helpers::sanitise(pValue));
                if (!b)
                {
                    return tl::unexpected<ParseError>(b.error());
                }
                else
                {
                    arrow.m_value = b.value();
                }
            }
            return arrow;
        };

        auto arrows = elements | views::filter(helpers::is_arrow) | views::transform(to_arrow);

        return utility::to_expected(arrows);
    }

    auto drawio_to_tokens(std::string_view drawio_xml_str) -> tl::expected<TokenTuple, ParseError>
    {
        XMLDocument doc;
        doc.Parse(drawio_xml_str.data());
        if (doc.ErrorID() != XML_SUCCESS)
        {
            return tl::unexpected<ParseError>(ParseError::InvalidDecodedDrawioFile);
        }

        XMLElement *origin = doc.RootElement();
        if (origin)
        {
            XMLElement *pRoot = origin->FirstChildElement("root");
            if (pRoot)
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

                auto states = states_from_xml_elements(elements);
                if (!states)
                {
                    return tl::unexpected<ParseError>(states.error());
                }
                auto predicates = predicates_from_xml_elements(elements);
                if (!predicates)
                {
                    return tl::unexpected<ParseError>(predicates.error());
                }
                auto arrows = arrows_from_xml_elements(elements);
                if (!arrows)
                {
                    return tl::unexpected<ParseError>(arrows.error());
                }

                return std::make_tuple(states.value(), predicates.value(), arrows.value());
            }
        }
        return tl::unexpected<ParseError>(ParseError::InvalidDecodedDrawioFile);
    }

    // handle errors during parsing and token generation
    void HandleParseError(const ParseError err)
    {
        switch (err)
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
                "<INVALID DECODED DRAWIO FILE ERROR> : decoded Draw.IO");
            break;
        case ParseError::MissingSourceArrow:
            throw std::runtime_error(
                "<MISSING SOURCE ARROW> :  One of your arrows is not correctly connected to its source");
            break;
        case ParseError::MissingTargetArrow:
            throw std::runtime_error(
                "<MISSING TARGET ARROW> : One of your arrows is not correctly connected to its target");
            break;
        case ParseError::IncorrectPredicateFormat:
            throw std::runtime_error(
                "<INVALID PREDICATE FORMAT> : You provided an invalid predicate to one of the decision blocks");
            break;
        case ParseError::InvalidBooleanSpecifier:
            throw std::runtime_error(
                "<INVALID BOOLEAN SPECIFIER> : You provided an invalid boolean specified on a decision block arrow");
            break;
        default:
            throw std::runtime_error(
                "Something unexpected went wrong ... try again.");
            break;
        }
    }
}
