#include "include/CopyTool/ICopyTool.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include <array>
#include <chrono>
#include <fstream>
#include <iostream>

using namespace boost::interprocess;
using namespace std::literals;

class ReadOperation
{
public:
private:
};

class WriteOperation
{
public:
private:
};

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

private:
    struct SharedData
    {
        std::chrono::steady_clock::time_point _readerStart;
        interprocess_mutex _mutex;
        interprocess_condition _cond;
        bool _dataReady;
        std::size_t _actualBufferSize;
        std::array<char, 1024 * 1024> _buffer; // add additional buffer
    };

    void Write(const std::filesystem::path &path)
    {
        auto writerStart = std::chrono::steady_clock::now();

        shared_memory_object shm_obj(open_only, _sharedMemoryName.data(), read_write);
        mapped_region region(shm_obj, read_write);
        void *address = region.get_address();
        SharedData *sharedData = static_cast<SharedData *>(address);

        std::filesystem::remove(path);
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for writing." << std::endl;
            return;
        }
        while (true)
        {
            scoped_lock<interprocess_mutex> lock(sharedData->_mutex);
            sharedData->_cond.wait(lock, [&]()
                                   { return sharedData->_dataReady; });
            if (sharedData->_actualBufferSize == 0)
            {
                break;
            }
            file.write(sharedData->_buffer.data(), sharedData->_actualBufferSize);
            sharedData->_dataReady = false;
            sharedData->_cond.notify_one();
        }

        auto writerFinish = std::chrono::steady_clock::now();
        std::cout << "Writer work time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         writerFinish - writerStart)
                         .count()
                  << " microseconds." << std::endl;
        std::cout << "General work time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         writerFinish - sharedData->_readerStart)
                         .count()
                  << " microseconds." << std::endl;
        shared_memory_object::remove(_sharedMemoryName.data()); // use remove in destructor or in final block
    }

    void Read(const std::filesystem::path &path)
    {
        auto readerStart = std::chrono::steady_clock::now();
        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (!file.is_open())
        {
            std::cout << "Error: Couldn't open file " << path << " for reading." << std::endl;
            return;
        }

        shared_memory_object shm_obj(create_only, _sharedMemoryName.data(), read_write);
        shm_obj.truncate(sizeof(SharedData));
        mapped_region region(shm_obj, read_write);

        void *address = region.get_address();

        SharedData *sharedData = new (address) SharedData;
        sharedData->_dataReady = false;
        sharedData->_readerStart = readerStart;

        while (file)
        {
            file.read(sharedData->_buffer.data(), sharedData->_buffer.size());
            sharedData->_actualBufferSize = file.gcount();
            if (sharedData->_actualBufferSize > 0)
            {
                scoped_lock<interprocess_mutex> lock(sharedData->_mutex);
                sharedData->_dataReady = true;
                sharedData->_cond.notify_one();
                sharedData->_cond.wait(lock, [&]()
                                       { return !sharedData->_dataReady; });
            }
        }

        scoped_lock<interprocess_mutex> lock(sharedData->_mutex);
        sharedData->_dataReady = true;
        sharedData->_actualBufferSize = 0;
        sharedData->_cond.notify_one();
        std::cout << "Reader work time: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now() -
                         readerStart)
                         .count()
                  << " microseconds." << std::endl;
    }

    std::string_view _sharedMemoryName;
    CopyToolMode _mode;
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

// process 1 f1, f2, sm1
// process 2 f1, f2, sm1
// process 3 f3, f4, sm1 = wait until first copying done
// process 4 f3, f4, sm1

// implementation rreqorked to work in parallel - two buffers in shared memory used for storing data from input files
// reqorked meshanism of communication with shared memory - now shared memory will be removed in destructor
// meshanism of stroign call history and monitoring state - in progress
// it is about monitoring calls with same shared memory name when second pair of processes shouild wait until first will finish
// and about monitoring of input and output file names - if one of that files matches currently files this pair of processes shouild wait unitl previous finishes
// and about calceling pair of processes if such pair of input and output files are the same  - processes should finish
// questions