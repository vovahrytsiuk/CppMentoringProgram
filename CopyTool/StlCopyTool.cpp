#include "include/CopyTool/ICopyTool.h"

class StlCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        std::filesystem::remove(destination);
        std::filesystem::copy_file(source, destination);
    }
};

ICopyToolPtrU CreateStlCopyTool()
{
    return std::make_unique<StlCopyTool>();
}