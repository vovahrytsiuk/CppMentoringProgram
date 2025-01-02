#include <gtest/gtest.h>
#include <CopyTool/ICopyTool.h>
#include <fstream>
#include <random>

TEST(CopyToolTestSuite, StlCopyToolCreatorTest)
{
    EXPECT_NO_THROW(CreateStlCopyTool());
}

TEST(CopyToolTestSuite, SingleThreadedCopyToolCreatorTest)
{
    EXPECT_NO_THROW(CreateSingleThreadedCopyTool(100));
}

TEST(CopyToolTestSuite, TwoThreadedCopyToolCreatorTest)
{
    EXPECT_NO_THROW(CreateTwoThreadedCopyTool(100));
}

TEST(CopyToolTestSuite, SharedMemoryCopyTool)
{
    EXPECT_NO_THROW(CreateSharedMemoryCopyTool("sm1"));
}

namespace
{
    bool CompareFiles(const std::filesystem::path &lhs, const std::filesystem::path &rhs)
    {
        std::ifstream file1(lhs, std::ios::binary);
        std::ifstream file2(rhs, std::ios::binary);

        if (file1.fail())
        {
            return false;
        }

        if (file2.fail())
        {
            return false;
        }

        std::vector<char> buffer1(0xFFFF), buffer2(0xFFFF);

        do
        {
            file1.read(buffer1.data(), buffer1.size());
            file2.read(buffer2.data(), buffer2.size());

            if (file1.gcount() != file2.gcount())
            {
                return false;
            }

            if (memcmp(buffer1.data(), buffer2.data(), file1.gcount()) != 0)
            {
                return false;
            }
        } while (file1 && file2);

        return true;
    }

    constexpr auto Kb = std::size_t{1024};
    constexpr auto Mb = std::size_t{1024 * Kb};
    constexpr auto Gb = std::size_t{1024 * Mb};

    void GenerateBinaryFile(const std::filesystem::path &filename, std::size_t fileSize, std::size_t bufferSize = Mb)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::ofstream outFile(filename, std::ios_base::out | std::ios_base::binary);

        std::size_t iterations = fileSize / bufferSize;
        std::size_t remaining = fileSize % bufferSize;

        std::vector<char> buffer(bufferSize);

        for (std::size_t i = 0; i < iterations; ++i)
        {
            for (auto &b : buffer)
            {
                b = static_cast<char>(dis(gen));
            }

            outFile.write(buffer.data(), buffer.size());
        }

        buffer.resize(remaining);

        for (auto &b : buffer)
        {
            b = static_cast<char>(dis(gen));
        }

        outFile.write(buffer.data(), buffer.size());
        outFile.close();
    }

    template <typename Function>
    auto measureExecutionTime(Function function)
    {
        auto start = std::chrono::high_resolution_clock::now();
        function();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    class FileGuard
    {
    public:
        FileGuard(std::filesystem::path path) : _path{std::move(path)}
        {
            if (std::filesystem::exists(_path))
            {
                std::filesystem::remove(_path);
            }
        }

        std::filesystem::path GetPath() const
        {
            return _path;
        }

        ~FileGuard()
        {
            std::filesystem::remove(_path);
        }

    private:
        std::filesystem::path _path;
    };

    struct TestParams
    {
        std::size_t _fileSize;
        std::vector<std::size_t> _bufferSizes;
    };

    class CopyToolTestFixture : public ::testing::TestWithParam<TestParams>
    {
    };
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(CopyToolTestSuite, CopyToolTestFixture, ::testing::Values(
    TestParams{Kb, {Kb / 4, Kb / 2, Kb, 10 * Kb}},
    TestParams{100 * Kb, {Kb, 10 * Kb, 100 * Kb, Mb}},
    TestParams{Mb, {Kb, 10 * Kb, 100 * Kb, Mb, 10 * Mb}},
    TestParams{10 * Mb, {Kb, 10 * Kb, 100 * Kb, Mb, 10 * Mb, 100 * Mb}},
    TestParams{100 * Mb, {Kb, 10 * Kb, 100 * Kb, Mb, 10 * Mb, 100 * Mb, Gb}},
    TestParams{Gb, {Kb, 10 * Kb, 100 * Kb, Mb, 10 * Mb, 100 * Mb, Gb, 2 * Gb}},
    TestParams{10 * Gb, {Kb, 10 * Kb, 100 * Kb, Mb, 10 * Mb, 100 * Mb, Gb, 2 * Gb, 4 * Gb}}
));
// clang-format on

TEST_P(CopyToolTestFixture, SingleThreadedCopyToolTest)
{
    auto source = FileGuard{"source"};
    auto destination = FileGuard{"destination"};
    GenerateBinaryFile(source.GetPath(), GetParam()._fileSize);
    for (auto bufferSize : GetParam()._bufferSizes)
    {
        auto copyTool = CreateSingleThreadedCopyTool(bufferSize);
        copyTool->CopyFile(source.GetPath(), destination.GetPath());
        CompareFiles(source.GetPath(), destination.GetPath());
    }
}

TEST_P(CopyToolTestFixture, TwoThreadedCopyToolTest)
{
    auto source = FileGuard{"source"};
    auto destination = FileGuard{"destination"};
    GenerateBinaryFile(source.GetPath(), GetParam()._fileSize);
    for (auto bufferSize : GetParam()._bufferSizes)
    {
        auto copyTool = CreateTwoThreadedCopyTool(bufferSize);
        copyTool->CopyFile(source.GetPath(), destination.GetPath());
        CompareFiles(source.GetPath(), destination.GetPath());
    }
}

TEST_P(CopyToolTestFixture, StlCopyToolTest)
{
    auto source = FileGuard{"source"};
    auto destination = FileGuard{"destination"};
    GenerateBinaryFile(source.GetPath(), GetParam()._fileSize);
    auto copyTool = CreateStlCopyTool();
    copyTool->CopyFile(source.GetPath(), destination.GetPath());
    CompareFiles(source.GetPath(), destination.GetPath());
}

TEST_P(CopyToolTestFixture, TimeComparisonTest)
{
    auto source = FileGuard{"source"};
    GenerateBinaryFile(source.GetPath(), GetParam()._fileSize);
    std::cout << "File size: " << GetParam()._fileSize << std::endl;
    for (auto bufferSize : GetParam()._bufferSizes)
    {
        {
            auto copyTool = CreateSingleThreadedCopyTool(bufferSize);
            auto destination = FileGuard{"destination"};
            auto time = measureExecutionTime([&]()
                                             { copyTool->CopyFile(source.GetPath(), destination.GetPath()); });
            std::cout << time
                      << " microseconds to copy file using single thread with buffer "
                      << std::to_string(bufferSize) << std::endl;
        }
        {
            auto copyTool = CreateTwoThreadedCopyTool(bufferSize);
            auto destination = FileGuard{"destination"};
            auto time = measureExecutionTime([&]()
                                             { copyTool->CopyFile(source.GetPath(), destination.GetPath()); });
            std::cout << time
                      << " microseconds to copy file using two threads with buffer "
                      << std::to_string(bufferSize) << std::endl;
        }
    }
    auto copyTool = CreateStlCopyTool();
    auto destination = FileGuard{"destination"};
    auto time = measureExecutionTime([&]()
                                     { copyTool->CopyFile(source.GetPath(), destination.GetPath()); });
    std::cout << time
              << " microseconds to copy file using stl" << std::endl;
}