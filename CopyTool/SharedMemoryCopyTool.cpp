#include "include/CopyTool/ICopyTool.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include <array>
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

        if (_mode == CopyToolMode::Reader)
        {
            _shm_obj = shared_memory_object(create_only, _sharedMemoryName.data(), read_write);
            _shm_obj.truncate(sizeof(SharedData));
        }
        else
        {
            _shm_obj = shared_memory_object(open_only, _sharedMemoryName.data(), read_write);
        }
        _region = mapped_region(_shm_obj, read_write);
        _sharedData = static_cast<SharedData *>(_region.get_address());
    }

    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination)
    {
        if (CopyToolMode::Reader == _mode)
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
        if (_mode == CopyToolMode::Writer)
        {
            shared_memory_object::remove(_sharedMemoryName.data());
        }
    }

private:
    struct SharedData
    {
        std::chrono::steady_clock::time_point _readerStart;
        interprocess_mutex _mutex1;
        interprocess_mutex _mutex2;
        interprocess_condition _cond1;
        interprocess_condition _cond2;
        bool _firstBufferReady = false;
        bool _secondBufferReady = false;
        std::size_t _actualFirstBufferSize;
        std::size_t _actualSecondBufferSize;
        std::array<char, 1024> _firstBuffer;
        std::array<char, 1024> _secondBuffer;
        bool _readingFinished = false;
    };

    void Write(const std::filesystem::path &path)
    {
        auto writerStart = std::chrono::steady_clock::now();
        std::filesystem::remove(path);
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for writing." << std::endl;
            return;
        }

        while (true)
        {
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
                while (!_sharedData->_firstBufferReady && !_sharedData->_readingFinished)
                {
                    _sharedData->_cond1.wait(lock1);
                }
                if (_sharedData->_readingFinished && !_sharedData->_firstBufferReady)
                {
                    break;
                }

                std::cout << "Writing from first buffer: " << _sharedData->_actualFirstBufferSize << std::endl;

                file.write(_sharedData->_firstBuffer.data(), _sharedData->_actualFirstBufferSize);
                _sharedData->_firstBufferReady = false;

                _sharedData->_cond1.notify_one();
                std::cout << "Writing from first buffer finished" << std::endl;
            }

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
                while (!_sharedData->_secondBufferReady && !_sharedData->_readingFinished)
                {
                    _sharedData->_cond2.wait(lock2);
                }
                if (_sharedData->_readingFinished && !_sharedData->_secondBufferReady)
                {
                    break;
                }

                std::cout << "Writing from second buffer: " << _sharedData->_actualSecondBufferSize << std::endl;

                file.write(_sharedData->_secondBuffer.data(), _sharedData->_actualSecondBufferSize);
                _sharedData->_secondBufferReady = false;

                _sharedData->_cond2.notify_one();
                std::cout << "Writing from second buffer finished" << std::endl;
            }
        }

        auto writerFinish = std::chrono::steady_clock::now();
        std::cout << "Expecting writer time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         writerStart - _sharedData->_readerStart)
                         .count()
                  << " milliseconds." << std::endl;
        std::cout << "Writer work time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         writerFinish - writerStart)
                         .count()
                  << " milliseconds." << std::endl;
        std::cout << "General work time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         writerFinish - _sharedData->_readerStart)
                         .count()
                  << " milliseconds." << std::endl;
    }

    void Read(const std::filesystem::path &path)
    {
        _sharedData->_readerStart = std::chrono::steady_clock::now();
        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for reading." << std::endl;
            return;
        }

        while (file)
        {
            // Handle the first buffer
            if (!_sharedData->_firstBufferReady)
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
                if (!_sharedData->_firstBufferReady)
                { // Double-check locking
                    std::cout << "Reading to first buffer" << std::endl;
                    file.read(_sharedData->_firstBuffer.data(), _sharedData->_firstBuffer.size());
                    _sharedData->_actualFirstBufferSize = file.gcount();
                    _sharedData->_firstBufferReady = true;
                    _sharedData->_cond1.notify_one();
                    std::cout << "Reading to first buffer finished: " << _sharedData->_actualFirstBufferSize << std::endl;
                }
            }

            // Handle the second buffer
            if (!_sharedData->_secondBufferReady && !file.eof())
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
                if (!_sharedData->_secondBufferReady)
                { // Double-check locking
                    std::cout << "Reading to second buffer" << std::endl;
                    file.read(_sharedData->_secondBuffer.data(), _sharedData->_secondBuffer.size());
                    _sharedData->_actualSecondBufferSize = file.gcount();
                    _sharedData->_secondBufferReady = true;
                    _sharedData->_cond2.notify_one();
                    std::cout << "Reading to second buffer finished: " << _sharedData->_actualSecondBufferSize << std::endl;
                }
            }
        }

        file.close();
        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedData->_mutex1);
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedData->_mutex2);
            _sharedData->_readingFinished = true;
            _sharedData->_cond1.notify_one();
            _sharedData->_cond2.notify_one();
        }

        std::cout << "Reader work time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() -
                         _sharedData->_readerStart)
                         .count()
                  << " milliseconds." << std::endl;
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
            shared_memory_object shm(open_only, sharedMemoryName.data(), read_write);
            return true;
        }
        catch (const interprocess_exception &)
        {
            return false;
        }
    }();
    auto mode = sharedMemoryExists ? SharedMemoryCopyTool::CopyToolMode::Writer
                                   : SharedMemoryCopyTool::CopyToolMode::Reader;
    std::cout << "Mode: " << (mode == SharedMemoryCopyTool::CopyToolMode::Reader ? "reader" : "writer") << std::endl;
    return std::make_unique<SharedMemoryCopyTool>(sharedMemoryName, mode);
}
