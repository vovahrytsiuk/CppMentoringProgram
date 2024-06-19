#include <stdio.h>
#include "ICopyTool.h"

int main(int, char**){
    printf("Hello, from copyTool!\n");
    auto copyTool = CreateTwoThreadedCopyTool();
    auto source = std::filesystem::path{"../example.txt"};
    auto destination = std::filesystem::path{"../result.txt"};
    std::filesystem::remove(destination);
    copyTool->CopyFile(source, destination);
}
