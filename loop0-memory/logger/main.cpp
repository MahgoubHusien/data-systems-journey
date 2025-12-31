#include "Logger.h"
#include <iostream>
#include <stdexcept>

using logger::Logger;

void earlyReturnTest() {
    logger::Logger logger("app.log");
    logger.logLine("start");
    return;
};

int main() {
    Logger a("a.log");
    Logger b = std::move(a);

    std::cout << "After move-construct b from a:\n";
    std::cout << "  a.isOpen() = " << a.isOpen() << "\n";
    std::cout << "  b.isOpen() = " << b.isOpen() << "\n";

    Logger c("c.log");
    c.logLine("c: before assigning from moved-from a");

    c = std::move(a); // a is already moved-from here

    std::cout << "After move-assign c from moved-from a:\n";
    std::cout << "  a.isOpen() = " << a.isOpen() << "\n";
    std::cout << "  c.isOpen() = " << c.isOpen() << "\n";

    c.logLine("c: after assigning from moved-from a (should NOT log)");
    b.logLine("b: should still log (owns a.log)");


    
    return 0;

}