#include "include/MainApp/ProgramOptions.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <string_view>

using namespace std::literals;

namespace
{
    constexpr auto HelpOption = "help"sv;
    constexpr auto ServerOption = "server"sv;
    constexpr auto ClientOption = "client"sv;
}

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(
    ProgramOptions::Mode mode)
    : _mode{mode}
{
}

std::optional<ProgramOptions> ProgramOptions::ParseProgramOptions(std::vector<std::string> commandLine)
{
    po::options_description commonOptions("Common options");
    commonOptions.add_options()(HelpOption.data(), "Print this copy tool help message");
    po::options_description options("Copy tool options");
    // clang-format off
    options.add_options()
    (ServerOption.data(), "Server mode of application to act as a server")
    (ClientOption.data(), "Client mode of application to actas a client");
    // clang-format on
    auto printHelpMessage = [&]()
    {
        std::cout << "Usage: copyTool --client/--server\n";
        std::cout << commonOptions << options;
    };
    try
    {
        po::variables_map vm;
        po::store(po::command_line_parser(commandLine).options(commonOptions).allow_unregistered().run(), vm);
        po::notify(vm);
        if (vm.contains(HelpOption.data()))
        {
            printHelpMessage();
            return std::nullopt;
        }
        po::store(po::command_line_parser(commandLine).options(options).run(), vm);
        po::notify(vm);
        bool isServer = vm.contains(ServerOption.data());
        bool isClient = vm.contains(ClientOption.data());
        if (isServer && isClient)
        {
            std::cout << "ERROR: Only one of --server/--client should be provided";
            return std::nullopt;
        }
        else if (isServer || isClient)
        {
            return ProgramOptions(isServer ? ProgramOptions::Mode::Server : ProgramOptions::Mode::Client);
        }
        else
        {
            std::cout << "ERROR: One of --server/--client should be provided";
            return std::nullopt;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        printHelpMessage();
    }

    return std::nullopt;
}