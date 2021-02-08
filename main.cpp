#include <memory>
#include "System.hpp"
#include "Console.hpp"

int main(int argc, char *argv[]) {
    std::shared_ptr<System> system = std::make_shared<System>(argc > 1 ? argv[1] : "fs.dat");
    std::shared_ptr<Console> console = std::make_shared<Console>(system);
    console->run();
    return 0;
}
