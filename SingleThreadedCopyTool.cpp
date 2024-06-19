#include "ICopyTool.h"

class SingleThreadedCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        throw std::runtime_error("not implemented");
    }
};

ICopyToolPtrU CreateSingleThreadedCopyTool()
{
    return std::make_unique<SingleThreadedCopyTool>();
}