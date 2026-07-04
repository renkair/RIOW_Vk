//
// Created by Renkai on 03/07/2026.
//

#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>


std::string readTextFile(const std::string &filePath)
{
    std::ifstream infile(filePath);
    if (infile.is_open())
    {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        const std::string output = buffer.str();
        infile.close();
        return output;
    }
    return std::string();
}
