#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "/dev/sensor0";
    std::ifstream device(path);
    if (!device) {
        std::cerr << "Cannot open " << path << " (load driver and check permissions)\n";
        return 1;
    }
    std::string sample;
    if (!std::getline(device, sample)) {
        std::cerr << "No sample read from " << path << "\n";
        return 1;
    }
    std::cout << "Kernel sensor sample: " << sample << '\n';
    return 0;
}
