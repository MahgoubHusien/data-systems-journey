#include "Logger.h"
#include <stdexcept>
#include <cstring>
#include <string>


namespace logger{

Logger::Logger(std::string_view file) :  file_(std::string(file), std::ios::app), buffer_(std::make_unique<char[]>(BUFFER_SIZE)), pos_(0){
    
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");

}

void Logger::logLine(std::string_view message) noexcept{
    if (!file_.is_open() || !buffer_) return; // moved-from or closed 

    const size_t len = message.size();
    const size_t needed = len + 1; // +1 for newline

    if (needed > BUFFER_SIZE){
        if (pos_ > 0) flush();
        file_.write(message.data(), static_cast<std::streamsize>(len));
        file_.write("\n", 1);
        file_.flush();
        return;
    }

    if (pos_ + needed > BUFFER_SIZE){
        flush();
    }

    std::memcpy(buffer_.get() + pos_, message.data(), len);
    pos_ += len;
    buffer_[pos_++] = '\n';
}

void Logger::flush() noexcept{
    if (!file_.is_open() || !buffer_ || pos_ == 0) return;

    file_.write(buffer_.get(), static_cast<std::streamsize>(pos_));
    file_.flush();
    pos_ = 0;
}


} // namespace logger
