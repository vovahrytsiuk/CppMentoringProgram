#include "include/CopyTool/ICopyTool.h"
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

class TwoThreadedCopyTool : public ICopyTool
{
public:
    TwoThreadedCopyTool(std::size_t bufferSize) : _bufferSize{bufferSize}
    {
        _buffer.reserve(_bufferSize);
    }

    void Terminate() override
    {
        std::cout << "Custom termination function called" << std::endl;
        std::exit(-1);
    }

    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        auto sourceFile = std::ifstream(source, std::ios::binary);
        if (!sourceFile)
        {
            throw std::runtime_error("File " + source.generic_string() + " cannot be opened for reading");
        }
        std::filesystem::remove(destination);
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
        auto localBuffer = std::vector<char>();
        localBuffer.reserve(_bufferSize);
        while (!sourceFile.eof())
        {
            sourceFile.read(localBuffer.data(), _bufferSize);
            localBuffer.resize(sourceFile.gcount());
            std::unique_lock<std::mutex> lock(_mutex);
            _conditionalVariable.wait(lock, [this]()
                                      { return !_bufferReady; });
            std::swap(_buffer, localBuffer);
            _bufferReady = true;
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
                                      { return _bufferReady; });
            std::swap(localBuffer, _buffer);
            _bufferReady = false;
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
    std::size_t _bufferSize;
    bool _bufferReady = false;
    bool _readingFinished = false;
    std::condition_variable _conditionalVariable;
    std::mutex _mutex;
};

ICopyToolPtrU CreateTwoThreadedCopyTool(std::size_t bufferSize)
{
    return std::make_unique<TwoThreadedCopyTool>(bufferSize);
}