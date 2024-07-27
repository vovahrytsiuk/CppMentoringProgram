#include <MainApp/ProgramOptions.h>
#include <CopyTool/ICopyTool.h>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    auto programOptions = ProgramOptions::ParseProgramOptions(args);
    if (!programOptions)
    {
        return 0;
    }
    try
    {
        auto copyTool = CreateSharedMemoryCopyTool("sharedMemory");
        copyTool->CopyFile(programOptions->_source, programOptions->_destination);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
