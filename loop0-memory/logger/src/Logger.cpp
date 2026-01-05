#include "Logger.h"
#include <stdexcept>
#include <cstring>


namespace logger{

Logger::Logger(std::string_view file) {
    buffer_ = new char[BUFFER_SIZE];
    capacity_ = BUFFER_SIZE;
    pos_ = 0;

    throw std::runtime_error("failpoint after alloc"); // <-- TEMP
    
    file_.open(std::string(file), std::ios::app);
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");

    ready_ = true;
}

Logger::Logger(Logger&& other) noexcept : file_(std::move(other.file_)) , buffer_(other.buffer_), capacity_(other.capacity_), pos_(other.pos_), ready_(other.ready_)
{
    other.buffer_ = nullptr;
    other.capacity_ = 0;
    other.pos_ = 0;
    other.ready_ = false;
}

Logger& Logger::operator=(Logger&& other) noexcept{
    if (this != &other) {
        flush(); // Flush the current buffer before moving the object
        delete[] buffer_;

        file_ = std::move(other.file_);
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        pos_ = other.pos_;
        ready_ = other.ready_;

        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.pos_ = 0;
        other.ready_ = false;
    }
    return *this;
}

void Logger::logLine(std::string_view message){
    if (!ready_) return; // moved-from or closed 

    const size_t len = message.size();
    const size_t needed = len + 1; // +1 for newline

    if (needed > capacity_){
        flush();
        file_.write(message.data(), static_cast<std::streamsize>(len));
        file_.write("\n", 1);
        file_.flush();
        return;
    }

    if (pos_ + needed > capacity_){
        flush();
    }

    std::memcpy(buffer_ + pos_, message.data(), len);
    pos_ += len;
    buffer_[pos_++] = '\n';
}

void Logger::flush(){
    if (!ready_ || pos_ == 0) return;

    file_.write(buffer_, static_cast<std::streamsize>(pos_));
    file_.flush();
    pos_ = 0;
}


} // namespace logger
