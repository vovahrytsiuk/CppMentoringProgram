#include "include/CopyTool/ICopyTool.h"
#include <fstream>
#include <vector>

class SingleThreadedCopyTool : public ICopyTool
{
public:
    SingleThreadedCopyTool(std::size_t bufferSize) : _bufferSize{bufferSize} {}

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
        auto buffer = std::vector<char>(_bufferSize);
        while (!sourceFile.eof())
        {
            sourceFile.read(buffer.data(), _bufferSize);
            destinationFile.write(buffer.data(), sourceFile.gcount());
        }
        sourceFile.close();
        destinationFile.close();
    }

private:
    std::size_t _bufferSize;
};

ICopyToolPtrU CreateSingleThreadedCopyTool(std::size_t bufferSize)
{
    return std::make_unique<SingleThreadedCopyTool>(bufferSize);
}