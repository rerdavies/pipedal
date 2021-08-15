#pragma once

#include <map>
#include <filesystem>


namespace pipedal {
    void WriteTemplateFile(
        const std::map<std::string,std::string> &map,
        std::filesystem::path inputFile,
        std::filesystem::path outputFile);
}