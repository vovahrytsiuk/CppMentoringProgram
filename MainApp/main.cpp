#include <MainApp/ProgramOptions.h>
#include <CopyTool/ICopyTool.h>
#include <string>
#include <iostream>

ICopyToolPtrU copyTool = nullptr;

int main(int argc, char **argv)
{
    set_terminate([]()
                  {std::cout << "Custom termination function called" << std::endl;if(copyTool){
                    copyTool.reset();
        
    } });
    std::vector<std::string> args(argv, argv + argc);
    auto programOptions = ProgramOptions::ParseProgramOptions(args);
    if (!programOptions)
    {
        return 0;
    }
    copyTool = CreateSharedMemoryCopyTool(programOptions->_sharedMemoryName);
    copyTool->CopyFile(programOptions->_source, programOptions->_destination);
    return 0;
}
