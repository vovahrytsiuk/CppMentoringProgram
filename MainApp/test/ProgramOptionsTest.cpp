#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MainApp/ProgramOptions.h"

using namespace std::literals;

namespace
{
    struct TestParams
    {
        std::vector<std::string> _commandLine;
        std::optional<ProgramOptions> _programOptions;
        std::string _expectedMessage;
    };

    class ProgramOptionsTestFixture : public ::testing::TestWithParam<TestParams>
    {
    };

    constexpr auto HelpScreen = "Usage: copyTool --source \"source path\" --destination \"destination path\""sv;
    constexpr auto HelpOption = "--help"sv;
    constexpr auto SourceOption = "--source"sv;
    constexpr auto DestinationOption = "--destination"sv;
    constexpr auto SourceFilePath = "source.txt"sv;
    constexpr auto DestinationFilePath = "dest.txt"sv;
    constexpr auto SharedMemoryOption = "--shared_memory"sv;
    constexpr auto SharedMemoryName = "SharedMemory"sv;
}

bool operator==(const ProgramOptions &lhs, const ProgramOptions &rhs)
{
    return std::tie(lhs._source, lhs._destination, lhs._sharedMemoryName) ==
           std::tie(rhs._source, rhs._destination, rhs._sharedMemoryName);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(ProgramOptionsTestSuite, ProgramOptionsTestFixture, ::testing::Values(
    TestParams{{}, std::nullopt, "the option '--destination' is required but missing"},
    TestParams{{DestinationOption.data()}, std::nullopt, "the required argument for option '--destination' is missing"},
    TestParams{{SourceOption.data()}, std::nullopt, "the required argument for option '--source' is missing"},
    TestParams{{HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{DestinationOption.data(), SourceOption.data()}, std::nullopt, "the option '--shared_memory' is required but missing"},
    TestParams{{SourceOption.data(), HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{DestinationOption.data(), HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{DestinationOption.data(), SourceOption.data(), HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{SourceOption.data(), SourceFilePath.data()}, std::nullopt, "the option '--destination' is required but missing"},
    TestParams{{SourceOption.data(), SourceFilePath.data(), HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{SourceOption.data(), SourceFilePath.data(), DestinationOption.data(), DestinationFilePath.data(), HelpOption.data()}, std::nullopt, HelpScreen.data()},
    TestParams{{SourceOption.data(), SourceFilePath.data(), DestinationOption.data(), DestinationFilePath.data(), SharedMemoryOption.data(), SharedMemoryName.data()}, ProgramOptions{SourceFilePath, DestinationFilePath, SharedMemoryName.data()}, ""}
));
// clang-format on

TEST_P(ProgramOptionsTestFixture, ProgramOptionsTest)
{
    ::testing::internal::CaptureStdout();
    auto programOptionsOpt = ProgramOptions::ParseProgramOptions(GetParam()._commandLine);
    auto output = ::testing::internal::GetCapturedStdout();
    EXPECT_THAT(output, ::testing::HasSubstr(GetParam()._expectedMessage));
    ASSERT_EQ(programOptionsOpt.has_value(), GetParam()._programOptions.has_value());
    if (programOptionsOpt.has_value())
    {
        EXPECT_EQ(*programOptionsOpt, *GetParam()._programOptions);
    }
}