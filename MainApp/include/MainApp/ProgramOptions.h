#pragma once
#include <filesystem>
#include <optional>
#include <vector>

class ProgramOptions
{
public:
    enum Mode
    {
        Server,
        Client
    };

    static std::optional<ProgramOptions> ParseProgramOptions(std::vector<std::string> commandLine);

    ProgramOptions(Mode mode);

    Mode _mode;
};