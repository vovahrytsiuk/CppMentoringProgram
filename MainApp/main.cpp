#include <stdio.h>
#include <CopyTool/ICopyTool.h>
#include <string>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "Usage: program <sourcefile> <destinationfile>\n";
        return 1;
    }

    auto copyTool = CreateTwoThreadedCopyTool();
    auto source = std::filesystem::path{std::string{argv[1]}};
    auto destination = std::filesystem::path{std::string{argv[2]}};
    std::filesystem::remove(destination);
    copyTool->CopyFile(source, destination);
    return 0;
}
