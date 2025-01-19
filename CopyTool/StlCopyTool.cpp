#include "include/CopyTool/ICopyTool.h"
#include <iostream>

class StlCopyTool : public ICopyTool
{
public:
    void CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination) override
    {
        std::filesystem::remove(destination);
        std::filesystem::copy_file(source, destination);
    }

    void Terminate() override
    {
        std::cout << "Custom termination function called" << std::endl;
        std::exit(-1);
    }
};

ICopyToolPtrU CreateStlCopyTool()
{
    return std::make_unique<StlCopyTool>();
}