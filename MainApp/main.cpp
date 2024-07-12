#include <MainApp/ProgramOptions.h>
#include <CopyTool/ICopyTool.h>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    auto programOptions = ProgramOptions::ParseProgramOptions(args);
    if (programOptions)
    {
        auto copyTool = CreateTwoThreadedCopyTool(100 * 1024);
        std::filesystem::remove(programOptions->_destination);
        copyTool->CopyFile(programOptions->_source, programOptions->_destination);
    }
    return 0;
}
