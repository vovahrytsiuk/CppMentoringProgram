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
#include <memory>

using namespace boost::interprocess;

class SharedMemory
{
public:
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
        std::atomic<bool> _readerTerminated = false;
    };

    explicit SharedMemory(std::string_view sharedMemoryName)
        : _sharedMemoryName{sharedMemoryName}
    {
        _shm_obj = shared_memory_object(open_or_create, _sharedMemoryName.data(), read_write);
        _shm_obj.truncate(sizeof(SharedData));
        _region = mapped_region(_shm_obj, read_write);
        _sharedData = static_cast<SharedData *>(_region.get_address());
        _sharedData->_copyToolNumber += 1;
        if (_sharedData->_copyToolNumber == 1)
        {
            _sharedData->_firstBufferReady = false;
            _sharedData->_secondBufferReady = false;
            _sharedData->_readingFinished = false;
        }
        std::cout << "Shared memory object constructed" << std::endl;
    }

    ~SharedMemory()
    {
        _sharedData->_copyToolNumber -= 1;
        if (_sharedData->_copyToolNumber == 0)
        {
            shared_memory_object::remove(_sharedMemoryName.data());
            std::cout << "Shared memory removed" << std::endl;
        }
        std::cout << "Shared memory object destructed" << std::endl;
    }

    SharedData &getData()
    {
        return *_sharedData;
    }

private:
    std::string_view _sharedMemoryName;
    shared_memory_object _shm_obj;
    mapped_region _region;
    SharedData *_sharedData;
};

class File
{
public:
    File(const std::filesystem::path &path, std::ios_base::openmode mode)
        : _file{path, mode}
    {
        if (!_file.is_open())
        {
            throw std::runtime_error("Error: Couldn't open file " + path.string());
        }
        std::cout << "File " << path << " constructed" << std::endl;
    }
    template <class BufferT>
    auto Read(BufferT &buffer, std::size_t size)
    {
        _file.read(buffer.data(), size);
        return _file.gcount();
    }

    template <class BufferT>
    void Write(BufferT &buffer, std::size_t size)
    {
        _file.write(buffer.data(), size);
    }

    bool Eof() const
    {
        return _file.eof();
    }

    explicit operator bool() const
    {
        return static_cast<bool>(_file);
    }

    ~File()
    {
        if (_file.is_open())
        {
            _file.close();
        }
        std::cout << "File destructed" << std::endl;
    }

private:
    std::fstream _file;
};

void ThrowFictiveException()
{
    throw std::invalid_argument("Fictive exception");
}

class SharedMemoryCopyTool : public ICopyTool
{
public:
    SharedMemoryCopyTool(std::string_view sharedMemoryName)
        : _sharedMemory{std::make_unique<SharedMemory>(sharedMemoryName)}
    {
        _mode = (_sharedMemory->getData()._copyToolNumber == 1) ? CopyToolMode::Reader : CopyToolMode::Writer;
        std::cout << "Shared memory copy tool constructed. Mode: "
                  << (_mode == CopyToolMode::Reader ? "Reader. " : "Writer. ")
                  << "Instance number: " << _sharedMemory->getData()._copyToolNumber << std::endl;
    }

    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination)
    {
        if (_sharedMemory->getData()._copyToolNumber > 2)
        {
            std::cout << "It is extra writer. Nothing to do" << std::endl;
        }
        else if (CopyToolMode::Reader == _mode)
        {
            _file = std::make_unique<File>(source, std::ios::binary | std::ios::in);
            Read();
        }
        else
        {
            std::filesystem::remove(destination);
            _file = std::make_unique<File>(destination, std::ios::binary | std::ios::out);
            Write();
        }
    }

    ~SharedMemoryCopyTool()
    {
        std::cout << "Shared memory copy tool destroed" << std::endl;
    }

    void Terminate() override
    {
        std::cout << "Custom termination function called" << std::endl;
        if (_sharedMemory)
        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedMemory->getData()._mutex1);
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedMemory->getData()._mutex2);
            _sharedMemory->getData()._readingFinished = true;
            _sharedMemory->getData()._firstBufferReady = false;
            _sharedMemory->getData()._secondBufferReady = false;
            _sharedMemory->getData()._cond.notify_one();
            _sharedMemory.reset();
        }
        if (_file)
        {
            _file.reset();
        }
        std::exit(-1);
    }

private:
    enum CopyToolMode
    {
        Reader,
        Writer
    };

    void Write()
    {
        try
        {
            auto writerStart = std::chrono::steady_clock::now();
            _sharedMemory->getData()._cond.notify_one();
            std::size_t processedDataLength = 0;
            while (true)
            {
                {
                    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedMemory->getData()._mutex1);
                    while (!_sharedMemory->getData()._firstBufferReady && !_sharedMemory->getData()._readingFinished)
                    {
                        _sharedMemory->getData()._cond.wait(lock1);
                    }

                    if (_sharedMemory->getData()._readingFinished && !_sharedMemory->getData()._firstBufferReady)
                    {
                        std::cout << "Reading finished" << std::endl;
                        break;
                    }

                    _file->Write(_sharedMemory->getData()._firstBuffer, _sharedMemory->getData()._actualFirstBufferSize);
                    processedDataLength += _sharedMemory->getData()._actualFirstBufferSize;
                    _sharedMemory->getData()._firstBufferReady = false;
                    _sharedMemory->getData()._cond.notify_one();
                    std::cout << "First buffer written" << std::endl;
                }
                {
                    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedMemory->getData()._mutex2);
                    while (!_sharedMemory->getData()._secondBufferReady && !_sharedMemory->getData()._readingFinished)
                    {
                        _sharedMemory->getData()._cond.wait(lock2);
                        std::cout << "1" << std::endl;
                    }
                    if (_sharedMemory->getData()._readingFinished && !_sharedMemory->getData()._secondBufferReady)
                    {
                        break;
                    }

                    _file->Write(_sharedMemory->getData()._secondBuffer, _sharedMemory->getData()._actualSecondBufferSize);
                    processedDataLength += _sharedMemory->getData()._actualSecondBufferSize;
                    _sharedMemory->getData()._secondBufferReady = false;
                    _sharedMemory->getData()._cond.notify_one();
                }
            }
            auto writerFinish = std::chrono::steady_clock::now();
            std::cout << "Expecting writer time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(
                             writerStart - _sharedMemory->getData()._readerStart)
                             .count()
                      << " nanoseconds." << std::endl;
            std::cout << "Writer work time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(
                             writerFinish - writerStart)
                             .count()
                      << " nanoseconds." << std::endl;
            std::cout << "General work time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(
                             writerFinish - _sharedMemory->getData()._readerStart)
                             .count()
                      << " nanoseconds." << std::endl;
            std::cout << "Processed data length: " << processedDataLength << std::endl;
        }
        catch (const std::runtime_error &err)
        {
            std::cout << err.what() << std::endl;
        }
    }

    void Read()
    {
        try
        {
            _sharedMemory->getData()._readerStart = std::chrono::steady_clock::now();
            std::cout << "Waiting for writer" << std::endl;
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(_sharedMemory->getData()._mutex1);
                if (!_sharedMemory->getData()._cond.wait_for(lock, std::chrono::seconds(5), [this]
                                                             { return _sharedMemory->getData()._copyToolNumber == 2; }))
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
            std::size_t processedDataLength = 0;
            while (*_file)
            {
                // ThrowFictiveException();
                // Handle the first buffer
                if (!_sharedMemory->getData()._firstBufferReady && !_file->Eof())
                {
                    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedMemory->getData()._mutex1);
                    if (!_sharedMemory->getData()._firstBufferReady && !_file->Eof())
                    { // Double-check locking
                        _sharedMemory->getData()._actualFirstBufferSize =
                            _file->Read(_sharedMemory->getData()._firstBuffer,
                                        _sharedMemory->getData()._firstBuffer.size());
                        processedDataLength += _sharedMemory->getData()._actualFirstBufferSize;
                        _sharedMemory->getData()._firstBufferReady = true;
                        _sharedMemory->getData()._cond.notify_one();
                    }
                }
                ThrowFictiveException();

                // Handle the second buffer
                if (!_sharedMemory->getData()._secondBufferReady && !_file->Eof())
                {
                    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedMemory->getData()._mutex2);
                    if (!_sharedMemory->getData()._secondBufferReady && !_file->Eof())
                    { // Double-check locking
                        _sharedMemory->getData()._actualSecondBufferSize =
                            _file->Read(_sharedMemory->getData()._secondBuffer,
                                        _sharedMemory->getData()._secondBuffer.size());
                        processedDataLength += _sharedMemory->getData()._actualSecondBufferSize;
                        _sharedMemory->getData()._secondBufferReady = true;
                        _sharedMemory->getData()._cond.notify_one();
                    }
                }
            }

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock1(_sharedMemory->getData()._mutex1);
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock2(_sharedMemory->getData()._mutex2);
                _sharedMemory->getData()._readingFinished = true;
                _sharedMemory->getData()._cond.notify_one();
            }

            std::cout << "Reader work time: "
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() -
                             _sharedMemory->getData()._readerStart)
                             .count()
                      << " nanoseconds." << std::endl;
            std::cout << "Processed data length: " << processedDataLength << std::endl;
        }
        catch (const std::runtime_error &err)
        {
            std::cout << "Exception: " << err.what() << std::endl;
        }
    }
    std::unique_ptr<SharedMemory> _sharedMemory;
    std::unique_ptr<File> _file;
    CopyToolMode _mode;
};

ICopyToolPtrU CreateSharedMemoryCopyTool(std::string_view sharedMemoryName)
{
    return std::make_unique<SharedMemoryCopyTool>(sharedMemoryName);
}
