#include "SessionFile.h"

#include <iostream>
#include <string>
#include <vector>

using namespace CoolEdit;

int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() != 2)
    {
        std::cerr << "Usage: " << args[0] << " <path/to/sesfile>\n";
        return 1;
    }
    auto file = load_session(args[1]);
    std::cout << file << '\n';
}
