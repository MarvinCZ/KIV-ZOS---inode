#pragma once
#include <memory>
#include "System.hpp"

class Console {
public:
    explicit Console(std::shared_ptr<System> system);
    void run();
    void readLine(const std::string& line);
    void loadFile(const std::string& file);

private:
    std::shared_ptr<System> system;

};



