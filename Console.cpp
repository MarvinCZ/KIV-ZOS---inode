#include "Console.hpp"
#include "unistd.h"
#include <fstream>
#include <utility>

Console::Console(std::shared_ptr<System> system) {
    this->system = std::move(system);
}

void Console::run() {
    std::string line;
    std::cout << "/$ ";
    while(std::getline(std::cin, line)) {
        this->readLine(line);
        std::cout << this->system->pwd << "$ ";
    }
}

void Console::readLine(const std::string& line) {
    auto firstSpace = line.find(' ');
    std::string command = line.substr(0, firstSpace);
    auto secondSpace = line.find(' ', firstSpace+1);
    std::string firstArg = firstSpace != std::string::npos ? line.substr(firstSpace + 1, secondSpace - (firstSpace + 1)) : "";
    std::string secondArg = secondSpace != std::string::npos ? line.substr(secondSpace + 1) : "";

    if (command == "cp") {
        this->system->copyFile(firstArg, secondArg);
    } else if (command == "ln") {
        this->system->hardLink(firstArg, secondArg);
    } else if (command == "mv") {
        this->system->moveFile(firstArg, secondArg);
    } else if (command == "rm") {
        this->system->removeFile(firstArg);
    } else if (command == "mkdir") {
        this->system->createDirectory(firstArg);
    } else if (command == "rmdir") {
        this->system->removeDirectory(firstArg);
    } else if (command == "ls") {
        this->system->listDirectory(firstArg);
    } else if (command == "cat") {
        this->system->printFile(firstArg);
    } else if (command == "cd") {
        this->system->cd(firstArg);
    } else if (command == "pws") {
        this->system->printPwd();
    } else if (command == "info") {
        this->system->info(firstArg);
    } else if (command == "incp") {
        this->system->copyFromOutside(firstArg, secondArg);
    } else if (command == "outcp") {
        this->system->copyToOutside(secondArg, firstArg);
    } else if (command == "load") {
        this->loadFile(firstArg);
    } else if (command == "format") {
        this->system->format(std::stoul(firstArg, nullptr, 0));
    }
}

void Console::loadFile(const std::string& file) {
    if (access(file.c_str(), R_OK) != 0) {
        std::cerr << "FILE NOT FOUND" << std::endl;
        return;
    }

    std::ifstream infile(file);
    std::string line;
    while (std::getline(infile, line))
    {
        this->readLine(line);
    }
}
