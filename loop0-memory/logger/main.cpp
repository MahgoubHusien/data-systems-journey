#include "Logger.h"
#include <iostream>
#include <stdexcept>


int main() {
    logger::Logger log("test.log");
    log.logLine("a");
    log.logLine("b");
    log.flush();
}