#pragma once
#include <filesystem>
#include <memory>

class ICopyTool
{
public:
    virtual void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) = 0;
    virtual ~ICopyTool() = default;
    virtual void Terminate() = 0;
};

using ICopyToolPtrU = std::unique_ptr<ICopyTool>;

ICopyToolPtrU CreateStlCopyTool();

ICopyToolPtrU CreateSingleThreadedCopyTool(std::size_t bufferSize);

ICopyToolPtrU CreateTwoThreadedCopyTool(std::size_t bufferSize);

ICopyToolPtrU CreateSharedMemoryCopyTool(std::string_view sharedMemoryName);