#include "../include/app.hpp"

#include "../include/parser.hpp"
#include "../include/FSM_builder.hpp"
#include "../include/transition_matrix.hpp"

#include <fmt/format.h>

#include <fstream>

namespace app
{
    auto run(const std::filesystem::path &path, Options options) -> void
    {
        // turn the encoded XML into tokens
        auto token_tuple =
            parser::extract_encoded_drawio(path)
                .and_then(parser::base64_decode)
                .and_then(parser::inflate)
                .and_then(parser::url_decode)
                .and_then(parser::drawio_to_tokens)
                .or_else(parser::HandleParseError);

        // break down the tuple into (s)tates, (p)redicates, and (a)rrows
        auto &[s, p, a] = token_tuple.value();

        // get the decisions
        model::TransitionMatrix m(s, a, p);
        auto state_transition_map = model::build_transition_tree_map(s, p, m);

        // build the output string
        fsm::FSMBuilder builder(state_transition_map);

        // write the result        
        if (options.out_file.has_value())
        {
            std::ofstream output_file;
            output_file.open(options.out_file.value(), std::ios::out | std::ios::trunc);
            output_file << builder.write();
            output_file.close();
        }
        else 
        {
            fmt::print("{}", builder.write());
        }
    }
}