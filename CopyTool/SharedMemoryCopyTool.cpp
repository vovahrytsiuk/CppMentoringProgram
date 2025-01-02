#include "include/CopyTool/ICopyTool.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>

using namespace boost::interprocess;

class SharedMemoryCopyTool : public ICopyTool
{
public:
    enum CopyToolMode
    {
        Reader,
        Writer
    };

    SharedMemoryCopyTool(std::string_view sharedMemoryName, CopyToolMode mode)
        : _sharedMemoryName{sharedMemoryName}, _mode{mode}
    {
        std::cout << "Copy toll mode: "
                  << (_mode == CopyToolMode::Reader ? "Reader" : "Writer")
                  << std::endl;

        _shm_obj = shared_memory_object(open_only, _sharedMemoryName.data(), read_write);
        _shm_obj.truncate(sizeof(SharedData));
        _region = mapped_region(_shm_obj, read_write);
        _sharedData = static_cast<SharedData *>(_region.get_address());
        _sharedData->_copyToolNumber += 1;
        std::cout << "Copy tool number: " << _sharedData->_copyToolNumber << std::endl;
    }

    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination)
    {
        if (_sharedData->_copyToolNumber > 2)
        {
            std::cout << "It is extra writer. Nothing to do" << std::endl;
        }
        else if (CopyToolMode::Reader == _mode)
        {
            Read(source);
        }
        else
        {
            Write(destination);
        }
    }

    ~SharedMemoryCopyTool()
    {
        _sharedData->_copyToolNumber -= 1;
        if (_sharedData->_copyToolNumber == 0)
        {
            shared_memory_object::remove(_sharedMemoryName.data());
        }
    }

private:
    struct SharedData
    {
        std::chrono::steady_clock::time_point _readerStart;
        interprocess_mutex _mutex;
        interprocess_mutex _mutex1;
        interprocess_mutex _mutex2;
        interprocess_condition _cond;
        std::atomic<bool> _firstBufferReady = false;
        std::atomic<bool> _secondBufferReady = false;
        std::atomic<std::size_t> _actualFirstBufferSize;
        std::atomic<std::size_t> _actualSecondBufferSize;
        std::array<char, 1024> _firstBuffer;
        std::array<char, 1024> _secondBuffer;
        std::atomic<bool> _readingFinished = false;
        std::atomic<std::size_t> _copyToolNumber = 0;
    };

    void Write(const std::filesystem::path &path)
    {
        auto writerStart = std::chrono::steady_clock::now();
        _sharedData->_cond.notify_one();
        std::filesystem::remove(path);
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for writing." << std::endl;
            return;
        }
        std::size_t processedDataLength = 0;
        while (true)
        {
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
                while (!_sharedData->_firstBufferReady && !_sharedData->_readingFinished)
                {
                    _sharedData->_cond.wait(lock1);
                }
                if (_sharedData->_readingFinished && !_sharedData->_firstBufferReady)
                {
                    break;
                }

                file.write(_sharedData->_firstBuffer.data(), _sharedData->_actualFirstBufferSize);
                processedDataLength += _sharedData->_actualFirstBufferSize;
                _sharedData->_firstBufferReady = false;
                _sharedData->_cond.notify_one();
            }

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
                while (!_sharedData->_secondBufferReady && !_sharedData->_readingFinished)
                {
                    _sharedData->_cond.wait(lock2);
                }
                if (_sharedData->_readingFinished && !_sharedData->_secondBufferReady)
                {
                    break;
                }

                file.write(_sharedData->_secondBuffer.data(), _sharedData->_actualSecondBufferSize);
                processedDataLength += _sharedData->_actualSecondBufferSize;
                _sharedData->_secondBufferReady = false;
                _sharedData->_cond.notify_one();
            }
        }

        auto writerFinish = std::chrono::steady_clock::now();
        std::cout << "Expecting writer time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(
                         writerStart - _sharedData->_readerStart)
                         .count()
                  << " nanoseconds." << std::endl;
        std::cout << "Writer work time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(
                         writerFinish - writerStart)
                         .count()
                  << " nanoseconds." << std::endl;
        std::cout << "General work time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(
                         writerFinish - _sharedData->_readerStart)
                         .count()
                  << " nanoseconds." << std::endl;
        std::cout << "Processed data length: " << processedDataLength << std::endl;
    }

    void Read(const std::filesystem::path &path)
    {
        _sharedData->_readerStart = std::chrono::steady_clock::now();
        std::cout << "Waiting for writer" << std::endl;
        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_sharedData->_mutex1);
            if (!_sharedData->_cond.wait_for(lock, std::chrono::seconds(5), [this]
                                             { return _sharedData->_copyToolNumber == 2; }))
            {
                // If wait_for returns false, the writer did not start within 5 seconds
                std::cout << "Reader timed out waiting for the writer to start. Nothing to do." << std::endl;
                return;
            }
            else
            {
                std::cout << "Reader starts processing as writer has started." << std::endl;
            }
        }
        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for reading." << std::endl;
            return;
        }

        std::size_t processedDataLength = 0;
        while (file)
        {
            // Handle the first buffer
            if (!_sharedData->_firstBufferReady && !file.eof())
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
                if (!_sharedData->_firstBufferReady && !file.eof())
                { // Double-check locking
                    file.read(_sharedData->_firstBuffer.data(), _sharedData->_firstBuffer.size());
                    _sharedData->_actualFirstBufferSize = file.gcount();
                    processedDataLength += _sharedData->_actualFirstBufferSize;
                    _sharedData->_firstBufferReady = true;
                    _sharedData->_cond.notify_one();
                }
            }

            // Handle the second buffer
            if (!_sharedData->_secondBufferReady && !file.eof())
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
                if (!_sharedData->_secondBufferReady && !file.eof())
                { // Double-check locking
                    file.read(_sharedData->_secondBuffer.data(), _sharedData->_secondBuffer.size());
                    _sharedData->_actualSecondBufferSize = file.gcount();
                    processedDataLength += _sharedData->_actualSecondBufferSize;
                    _sharedData->_secondBufferReady = true;
                    _sharedData->_cond.notify_one();
                }
            }
        }

        file.close();
        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
            _sharedData->_readingFinished = true;
            _sharedData->_cond.notify_one();
        }

        std::cout << "Reader work time: "
                  << std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now() -
                         _sharedData->_readerStart)
                         .count()
                  << " nanoseconds." << std::endl;
        std::cout << "Processed data length: " << processedDataLength << std::endl;
    }

    std::string_view _sharedMemoryName;
    CopyToolMode _mode;
    shared_memory_object _shm_obj;
    mapped_region _region;
    SharedData *_sharedData;
};

ICopyToolPtrU CreateSharedMemoryCopyTool(std::string_view sharedMemoryName)
{
    auto sharedMemoryExists = [&]()
    {
        try
        {
            shared_memory_object shm(create_only, sharedMemoryName.data(), read_write);
            return false;
        }
        catch (const interprocess_exception &)
        {
            return true;
        }
    }();
    auto mode = sharedMemoryExists ? SharedMemoryCopyTool::CopyToolMode::Writer
                                   : SharedMemoryCopyTool::CopyToolMode::Reader;
    return std::make_unique<SharedMemoryCopyTool>(sharedMemoryName, mode);
}
