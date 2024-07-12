#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

class ProgramOptions
{
public:
    static std::optional<ProgramOptions> ParseProgramOptions(std::vector<std::string> commandLine);

    static constexpr auto HelpOption = "help";
    static constexpr auto SourceOption = "source";
    static constexpr auto DestinationOption = "destination";

    std::filesystem::path _source;
    std::filesystem::path _destination;

    ProgramOptions(std::filesystem::path source, std::filesystem::path destination);
};