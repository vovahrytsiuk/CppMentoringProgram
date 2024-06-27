#include "include/CopyTool/ICopyTool.h"
#include <fstream>
#include <vector>

class SingleThreadedCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        auto sourceFile = std::ifstream(source, std::ios::binary);
        if (!sourceFile)
        {
            throw std::runtime_error("File " + source.generic_string() + " cannot be opened for reading");
        }
        auto destinationFile = std::ofstream(destination, std::ios::binary | std::ios::trunc); // c++ 23, noreplay
        if (!destinationFile)
        {
            throw std::runtime_error("File " + source.generic_string() + " cannot be opened for writing");
        }
        constexpr auto bufferSize = 1024 * 1024; // 1MB
        auto buffer = std::vector<char>(bufferSize);
        while (!sourceFile.eof())
        {
            sourceFile.read(buffer.data(), bufferSize);
            destinationFile.write(buffer.data(), bufferSize);
        }
        sourceFile.close();
        destinationFile.close();
    }
};

ICopyToolPtrU CreateSingleThreadedCopyTool()
{
    return std::make_unique<SingleThreadedCopyTool>();
}