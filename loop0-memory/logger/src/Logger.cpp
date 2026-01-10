#include "Logger.h"
#include <stdexcept>
#include <cstring>
#include <string>


namespace logger{

Logger::Logger(std::string_view file) :  file_(std::string(file), std::ios::app), buffer_(std::make_unique<char[]>(BUFFER_SIZE)), pos_(0){
    
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");

    alive_ = true;

}

Logger::Logger(Logger&& other) noexcept
  : file_(std::move(other.file_)),
    buffer_(std::move(other.buffer_)),
    pos_(other.pos_),
    alive_(other.alive_)
{
  // Source is now moved-from: make it dead-by-contract.
  other.pos_ = 0;
  other.alive_ = false;
}

Logger& Logger::operator=(Logger&& other) noexcept {
    if (this == &other) return *this;
  
    // Put *this into a safe dead state (and flush/close if needed)
    shutdown();
  
    file_   = std::move(other.file_);
    buffer_ = std::move(other.buffer_);
    pos_    = other.pos_;
    alive_  = other.alive_;
  
    other.pos_ = 0;
    other.alive_ = false;
  
    return *this;
  }

std::string_view Logger::levelToString(Level level) noexcept{
    switch(level){
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}
  

void Logger::log(Level sToLevel, std::string_view message) noexcept{
    if (!alive_ || !file_.is_open() || !buffer_) return; // moved-from or closed 

    std::string_view level = levelToString(sToLevel);

    const size_t levelLen = level.size();
    const size_t messageLen = message.size();
    const size_t needed = levelLen + messageLen + 1 + 1 + 1 + 1; // '[', ']', ' ', '\n' 

    if (needed > BUFFER_SIZE){
        if (pos_ > 0) flush();
        file_.write("[", 1);        
        file_.write(level.data(), static_cast<std::streamsize>(levelLen));
        file_.write("] ", 2);

        file_.write(message.data(), static_cast<std::streamsize>(messageLen));
        file_.write("\n", 1);
        file_.flush();
        return;
    }

    if (pos_ + needed > BUFFER_SIZE){
        flush();
    }

    buffer_[pos_++] = '[';
    std::memcpy(buffer_.get() + pos_, level.data(), levelLen);
    pos_ += levelLen;

    buffer_[pos_++] = ']';
    buffer_[pos_++] = ' ';

    std::memcpy(buffer_.get() + pos_, message.data(), messageLen);
    pos_ += messageLen;
    buffer_[pos_++] = '\n';
}

void Logger::flush() noexcept{
    if (!alive_ || !file_.is_open() || !buffer_ || pos_ == 0) return;

    file_.write(buffer_.get(), static_cast<std::streamsize>(pos_));
    file_.flush();
    pos_ = 0;
}

void Logger::shutdown() noexcept{
    if (!alive_) return;

    if (buffer_ && pos_ > 0){
        flush();
    }

    alive_ = false;

    if (file_.is_open()) file_.close();    
    pos_ = 0;
    buffer_.reset();
}

} // namespace logger
