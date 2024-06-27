#include "include/CopyTool/ICopyTool.h"
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

class TwoThreadedCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        auto sourceFile = std::ifstream(source, std::ios::binary);
        if (!sourceFile)
        {
            throw std::runtime_error("File " + source.generic_string() + " cannot be opened for reading");
        }
        auto destinationFile = std::ofstream(destination, std::ios::binary);
        if (!destinationFile)
        {
            throw std::runtime_error("File " + source.generic_string() + " cannot be opened for writing");
        }

        auto reader = std::thread(&TwoThreadedCopyTool::readData, this, std::ref(sourceFile));
        auto writer = std::thread(&TwoThreadedCopyTool::writeData, this, std::ref(destinationFile));

        reader.join();
        writer.join();

        sourceFile.close();
        destinationFile.close();
    }

private:
    void readData(std::ifstream &sourceFile)
    {
        constexpr std::size_t bufferSize{1024 * 1024};
        auto localBuffer = std::vector<char>(bufferSize);
        while (!sourceFile.eof())
        {
            sourceFile.read(localBuffer.data(), bufferSize);
            localBuffer.resize(sourceFile.gcount());
            std::unique_lock<std::mutex> lock(_mutex);
            _conditionalVariable.wait(lock, [this]()
                                      { return _buffer.empty(); });
            std::swap(_buffer, localBuffer);
            _conditionalVariable.notify_one();
        }
        std::unique_lock<std::mutex> lock(_mutex);
        _readingFinished = true;
        _conditionalVariable.notify_one();
    }
    void writeData(std::ofstream &destinationFile)
    {
        auto localBuffer = std::vector<char>();
        localBuffer.reserve(_bufferSize);
        while (true)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _conditionalVariable.wait(lock, [this]()
                                      { return !_buffer.empty(); });
            std::swap(localBuffer, _buffer);
            _conditionalVariable.notify_one();
            lock.unlock();
            if (!localBuffer.empty())
            {
                destinationFile.write(localBuffer.data(), localBuffer.size());
            }
            lock.lock();
            if (_readingFinished)
            {
                break;
            }
        }
    }

    std::vector<char> _buffer;
    bool _readingFinished = false;
    std::condition_variable _conditionalVariable;
    std::mutex _mutex;
};

ICopyToolPtrU CreateTwoThreadedCopyTool()
{
    return std::make_unique<TwoThreadedCopyTool>();
}