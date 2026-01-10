#include "Logger.h"
#include <iostream>
#include <stdexcept>


int main() {
    logger::Logger log("test.log");

    log.log(logger::Level::Info,  "startup");
    log.log(logger::Level::Warn,  "disk 90% full");
    log.log(logger::Level::Error, "failed to write file");
    log.log(logger::Level::Debug, "retrying");

    log.flush();
}