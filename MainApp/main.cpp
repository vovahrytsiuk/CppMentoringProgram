#include <MainApp/ProgramOptions.h>
#include <CopyTool/ICopyTool.h>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    auto programOptions = ProgramOptions::ParseProgramOptions(args);
    if (!programOptions)
    {
        return 0;
    }
    try
    {
        auto copyTool = CreateSharedMemoryCopyTool(programOptions->_sharedMemoryName);
        copyTool->CopyFile(programOptions->_source, programOptions->_destination);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
