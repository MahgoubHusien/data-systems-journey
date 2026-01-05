#include "Logger.h"
#include <iostream>
#include <stdexcept>


int main() {
    try {
        logger::Logger log("x.log");
    } catch (const std::exception& e) {
        std::cerr << "Caught: " << e.what() << "\n";
    }
    return 0;
}