#include "Logger.h"
#include <stdexcept>
#include <cstring>
#include <string>


namespace logger{

Logger::Logger(Config cfg) : pos_(0), flush_on_each_write_(cfg.flush_on_each_write) {
    if (cfg.buffer_size == 0) throw std::runtime_error("Buffer Size is 0");

    file_.open(cfg.path, std::ios::app);
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");

    buffer_ = std::make_unique<char[]>(cfg.buffer_size);
    cap_ = cfg.buffer_size;
    alive_ = true;
}
    

Logger::Logger(Logger&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_);

    file_ = std::move(other.file_);
    buffer_ = std::move(other.buffer_);
    cap_ = other.cap_;
    pos_ = other.pos_;
    flush_on_each_write_ = other.flush_on_each_write_;
    alive_ = other.alive_;

    other.cap_ = 0;
    other.pos_ = 0;
    other.flush_on_each_write_ = false;
    other.alive_ = false;
}


Logger& Logger::operator=(Logger&& other) noexcept {
    if (this == &other) return *this;
  
    std::scoped_lock lock(m_, other.m_);
    shutdown_unlocked();
  
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
    std::lock_guard<std::mutex> lock(m_);
    if (!alive_ || !file_.is_open() || !buffer_) return; // moved-from or closed 

    std::string_view level = levelToString(sToLevel);

    const size_t levelLen = level.size();
    const size_t messageLen = message.size();
    const size_t needed = levelLen + messageLen + 1 + 1 + 1 + 1; // '[', ']', ' ', '\n' 

    if (needed > cap_){
        if (pos_ > 0) flush_unlocked();
        file_.write("[", 1);        
        file_.write(level.data(), static_cast<std::streamsize>(levelLen));
        file_.write("] ", 2);

        file_.write(message.data(), static_cast<std::streamsize>(messageLen));
        file_.write("\n", 1);

        if (flush_on_each_write_){
            file_.flush();
        }
        return;
    }

    if (pos_ + needed > cap_){
        flush_unlocked();
    }

    buffer_[pos_++] = '[';
    std::memcpy(buffer_.get() + pos_, level.data(), levelLen);
    pos_ += levelLen;

    buffer_[pos_++] = ']';
    buffer_[pos_++] = ' ';

    std::memcpy(buffer_.get() + pos_, message.data(), messageLen);
    pos_ += messageLen;
    buffer_[pos_++] = '\n';
    if (flush_on_each_write_){ 
        flush_unlocked();
        file_.flush();
    }
}

void Logger::flush_unlocked() noexcept{
    if (!alive_ || !file_.is_open() || !buffer_ || pos_ == 0) return;

    file_.write(buffer_.get(), static_cast<std::streamsize>(pos_));
    pos_ = 0;
}

void Logger::flush() noexcept{
    std::lock_guard<std::mutex> lock(m_);
    if (!alive_ || !file_.is_open()) return;
    flush_unlocked();
    file_.flush();
}

void Logger::shutdown_unlocked() noexcept{
    if (!alive_) return;

    if (buffer_ && pos_ > 0){
        flush_unlocked();
    }

    alive_ = false;

    if (file_.is_open()) file_.close();    
    cap_ = 0;
    pos_ = 0;
    flush_on_each_write_ = false;
    buffer_.reset();
}

void Logger::shutdown() noexcept{
    std::lock_guard<std::mutex> lock(m_);
    shutdown_unlocked();
}

} // namespace logger
