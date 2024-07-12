#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MainApp/ProgramOptions.h"

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

    constexpr auto HelpScreen = "Usage: copyTool --source \"source path\" --destination \"destination path\"";
}

bool operator==(const ProgramOptions &lhs, const ProgramOptions &rhs)
{
    return std::tie(lhs._source, lhs._destination) == std::tie(rhs._source, rhs._destination);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(ProgramOptionsTestSuite, ProgramOptionsTestFixture, ::testing::Values(
    TestParams{{}, std::nullopt, "the option '--destination' is required but missing"},
    TestParams{{"--destination"}, std::nullopt, "the required argument for option '--destination' is missing"},
    TestParams{{"--source"}, std::nullopt, "the required argument for option '--source' is missing"},
    TestParams{{"--help"}, std::nullopt, HelpScreen},
    TestParams{{"--destination", "--source"}, std::nullopt, "the option '--source' is required but missing"},
    TestParams{{"--source", "--help"}, std::nullopt, HelpScreen},
    TestParams{{"--destination", "--help"}, std::nullopt, HelpScreen},
    TestParams{{"--destination", "--source", "--help"}, std::nullopt, HelpScreen},
    TestParams{{"--source", "source.txt"}, std::nullopt, "the option '--destination' is required but missing"},
    TestParams{{"--source", "source.txt", "--help"}, std::nullopt, HelpScreen},
    TestParams{{"--source", "source.txt", "--destination", "dest.txt", "--help"}, std::nullopt, HelpScreen},
    TestParams{{"--source", "source.txt", "--destination", "dest.txt"}, ProgramOptions{"source.txt", "dest.txt"}, ""}
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