#include "Logger.h"
#include <stdexcept>
#include <iostream>

namespace logger{

Logger::Logger(const std::string& file) 
    : file_(fopen(file.c_str(), "a")) {
    if (!file_) {
        throw std::runtime_error("Failed to open log file");
    }
}

Logger::~Logger() noexcept{
    if (file_) {
        std::cerr << "~Logger closing file\n";
        fclose(file_);
        file_ = nullptr;
    }
}

void Logger::logLine(const std::string& message){
    fprintf(file_, "%s\n", message.c_str());
    fflush(file_);
}


} // namespace logger
