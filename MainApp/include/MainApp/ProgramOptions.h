#pragma once
#include <filesystem>
#include <optional>
#include <vector>

class ProgramOptions
{
public:
    static std::optional<ProgramOptions> ParseProgramOptions(std::vector<std::string> commandLine);

    ProgramOptions(std::filesystem::path source, std::filesystem::path destination, std::string sharedMemoryName);

    std::filesystem::path _source;
    std::filesystem::path _destination;
    std::string _sharedMemoryName;
};