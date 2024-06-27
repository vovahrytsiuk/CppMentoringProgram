#pragma once
#include <filesystem>
#include <string_view>
#include <vector>

class ProgramOptions
{
public:
    ProgramOption(std::vector<std::string> commandLine);

    static constexpr std::string_view HelpOption = "--help";
    static constexpr std::string_view SourceOption = "--source";
    static constexpr std::string_view DestinationOption = "--destination";

    std::filesystem::path _source;
    std::filesystem::path _destination;
};