#ifndef APP_H
#define APP_H

#include <filesystem>
#include <optional>
#include <string>

namespace app 
{
    struct Options
    {
        std::optional<std::filesystem::path> out_file; 
    };

    auto run(const std::filesystem::path& path, Options options) -> void;
}

#endif