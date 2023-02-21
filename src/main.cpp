#include "../include/app.hpp"

#include <argparse/argparse.hpp>

auto main(const int argc, char const * const * const argv) -> int
{
    argparse::ArgumentParser program("FSM.io");

    program.add_argument("-d", "--diagram")
        .required()
        .default_value("../../resources/test.drawio")
        .help("Specify the draw.io file you wish to convert.");
    program.add_argument("-o", "--outfile")
        .help("Specify the file you wish to write the output of the conversion too (optional)");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    // set up the optional arguments
    app::Options options;
    if (auto o = program.present("-o")) 
    {
        options.out_file = std::filesystem::path{*o};
    }

    // run with the options and the required arguments
    const std::filesystem::path infile{program.get("-d")};
    app::run(infile, options);
    return 0;
}
