#include "include/MainApp/ProgramOptions.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <string_view>

using namespace std::literals;

namespace
{
    constexpr auto HelpOption = "help"sv;
    constexpr auto SourceOption = "source"sv;
    constexpr auto DestinationOption = "destination"sv;
}

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(std::filesystem::path source, std::filesystem::path destination)
    : _source{std::move(source)}, _destination{std::move(destination)}
{
}

std::optional<ProgramOptions> ProgramOptions::ParseProgramOptions(std::vector<std::string> commandLine)
{
    po::options_description commonOptions("Common options");
    commonOptions.add_options()(HelpOption.data(), "Print this copy tool help message");
    po::options_description options("Copy tool options");
    // clang-format off
    options.add_options()
    (SourceOption.data(), po::value<std::filesystem::path>()->required(), "Source file path")
    (DestinationOption.data(), po::value<std::filesystem::path>()->required(), "Destination file path");
    // clang-format on
    auto printHelpMessage = [&]()
    {
        std::cout << "Usage: copyTool --source \"source path\" --destination \"destination path\"\n";
        std::cout << commonOptions << options;
    };
    try
    {
        po::variables_map vm;
        po::store(po::command_line_parser(commandLine).options(commonOptions).allow_unregistered().run(), vm);
        po::notify(vm);
        if (vm.count(HelpOption.data()))
        {
            printHelpMessage();
            return std::nullopt;
        }
        po::store(po::command_line_parser(commandLine).options(options).run(), vm);
        po::notify(vm);
        return ProgramOptions(vm[SourceOption.data()].as<std::filesystem::path>(), vm[DestinationOption.data()].as<std::filesystem::path>());
    }
    catch (std::exception &e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        printHelpMessage();
    }

    return std::nullopt;
}