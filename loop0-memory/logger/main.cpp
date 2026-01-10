#include "Logger.h"
#include <iostream>


int main() {
    logger::Config cfg;
    cfg.path = "test.log";
    cfg.buffer_size = 0;
    cfg.flush_on_each_write = false;

    logger::Logger log(cfg);

    log.log(logger::Level::Info,  "startup");
    log.log(logger::Level::Warn,  "disk 90% full");
    log.log(logger::Level::Error, "failed to write file");
    log.log(logger::Level::Debug, "retrying");
    log.flush();

    std::cout << "done\n";
}