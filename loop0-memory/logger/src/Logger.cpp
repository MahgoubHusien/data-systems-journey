#include "Logger.h"
#include <stdexcept>
#include <fstream>


namespace logger{

Logger::Logger(std::string_view file) 
    : file_(std::string(file), std::ios::app) {
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }
}

void Logger::logLine(std::string_view message){
    if (!file_.is_open()) return; // moved-from or closed 
    file_ << message << "\n";
    file_.flush();
}


} // namespace logger
