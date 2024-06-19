#include "ICopyTool.h"

class TwoThreadedCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        throw std::runtime_error("not implemented");
    }
};

ICopyToolPtrU CreateTwoThreadedCopyTool()
{
    return std::make_unique<TwoThreadedCopyTool>();
}