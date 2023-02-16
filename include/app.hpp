#ifndef APP_H
#define APP_H

#include <filesystem>

namespace app 
{
    auto run(const std::filesystem::path& path) -> void;
}

#endif