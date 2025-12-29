#include "Logger.h"
#include <iostream>
#include <stdexcept>

void earlyReturnTest() {
    logger::Logger logger("app.log");
    logger.logLine("start");
    return;
};

int main() {
    std::cout << "Program started\n" ;
    logger::Logger logger("app.log");
    logger.logLine("hello");
    std::cout << "PASS: basic logging (check app.log)\n";

    {
        logger::Logger logger("app.log");
        logger.logLine("nested scope, hello");
    }
    std::cout << "PASS: scope exit triggers destructor\n";

    earlyReturnTest();
    std::cout << "PASS: early return didnt leak\n" ;
  
    try {
        logger::Logger logger(".");
        logger.logLine("should throw");
        std::cout << "FAIL: expected constructor to throw\n";
        return 1;
    } catch (const std::runtime_error&) {
        std::cout << "PASS: exception caught\n" ;
 }
    std::cout << "Program ended\n";

    
    return 0;

}