#include "../include/app.hpp"

#include <argparse/argparse.hpp>

auto main(const int argc, char const * const * const argv) -> int
{
    argparse::ArgumentParser program("FSM.io");

    program.add_argument("-d", "--diagram")
        .required()
        .help("specify the draw.io file you wish to convert.");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    if (auto d = program.present("-d")) 
    {
        std::filesystem::path f{*d};
        app::run(f);
    }
    return 0;
}
