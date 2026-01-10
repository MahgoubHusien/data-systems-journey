#include "Logger.h"
#include <stdexcept>
#include <cstring>
#include <string>


namespace logger{

Logger::Logger(Config cfg) :  file_(cfg.path, std::ios::app), buffer_(std::make_unique<char[]>(cfg.buffer_size)), pos_(0), flush_on_each_write_(cfg.flush_on_each_write){
    
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");
    if (cfg.buffer_size == 0) throw std::runtime_error("Buffer Size is 0");
    cap_ = cfg.buffer_size;
    alive_ = true;

}

Logger::Logger(Logger&& other) noexcept
  : file_(std::move(other.file_)),
    buffer_(std::move(other.buffer_)),
    cap_(other.cap_),
    pos_(other.pos_),
    flush_on_each_write_(other.flush_on_each_write_),
    alive_(other.alive_)
{
  // Source is now moved-from: make it dead-by-contract.
  other.cap_ = 0;
  other.pos_ = 0;
  other.flush_on_each_write_ = false;
  other.alive_ = false;
}

Logger& Logger::operator=(Logger&& other) noexcept {
    if (this == &other) return *this;
  
    // Put *this into a safe dead state (and flush/close if needed)
    shutdown();
  
    file_   = std::move(other.file_);
    buffer_ = std::move(other.buffer_);
    cap_    = other.cap_;
    pos_    = other.pos_;
    flush_on_each_write_ = other.flush_on_each_write_;
    alive_  = other.alive_;
  
    other.cap_ = 0;
    other.pos_ = 0;
    other.flush_on_each_write_ = false;
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

    if (needed > cap_){
        if (pos_ > 0) flush();
        file_.write("[", 1);        
        file_.write(level.data(), static_cast<std::streamsize>(levelLen));
        file_.write("] ", 2);

        file_.write(message.data(), static_cast<std::streamsize>(messageLen));
        file_.write("\n", 1);
        file_.flush();
        return;
    }

    if (pos_ + needed > cap_){
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
    if (flush_on_each_write_) flush();
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
    cap_ = 0;
    pos_ = 0;
    flush_on_each_write_ = false;
    buffer_.reset();
}

} // namespace logger
