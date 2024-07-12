#include "include/MainApp/ProgramOptions.h"

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(std::filesystem::path source, std::filesystem::path destination)
    : _source{std::move(source)}, _destination{std::move(destination)}
{
}

std::optional<ProgramOptions> ProgramOptions::ParseProgramOptions(std::vector<std::string> commandLine)
{
    try
    {
        po::options_description commonOptions;
        commonOptions.add_options()(HelpOption, "Print this copy tool help message");
        po::options_description options("Copy tool options");
        options.add_options()(SourceOption, po::value<std::filesystem::path>()->required(), "Source file path")(DestinationOption, po::value<std::filesystem::path>()->required(), "Destination file path");
        po::variables_map vm;
        po::store(po::command_line_parser(commandLine).options(commonOptions).allow_unregistered().run(), vm);
        po::notify(vm);
        if (vm.count(HelpOption))
        {
            std::cout << "Usage: copyTool --source \"source path\" --destination \"destination path\"\n";
            std::cout << options;
            return std::nullopt;
        }
        po::store(po::command_line_parser(commandLine).options(options).run(), vm);
        po::notify(vm);
        return ProgramOptions(vm[SourceOption].as<std::filesystem::path>(), vm[DestinationOption].as<std::filesystem::path>());
    }
    catch (std::exception &e)
    {
        std::cout << "ERROR: " << e.what() << std::endl
                  << std::endl;
    }

    return std::nullopt;
}