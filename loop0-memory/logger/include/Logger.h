#pragma once

#include <string_view>
#include <fstream>
#include <cstddef>
#include <iostream>



namespace logger{

class Logger{
public:
    explicit Logger(std::string_view file);
    // We delete copy construction and copy assignment so duplicating a resource-owning object is impossible at compile time.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // We enable move construction and assignment
    Logger(Logger&&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    ~Logger() noexcept{
        std::cerr << "~Logger called\n";
        if (ready_) flush();
        delete[] buffer_;
    }

    void logLine(std::string_view message);
    void flush();


private:
    // A pointer to a runtime-managed object that knows how to talk to the OS for file operations.
    std::ofstream file_;
    static constexpr size_t BUFFER_SIZE = 4096;
    char* buffer_ = nullptr; // 4KB buffer for logging
    size_t capacity_ = 0;
    size_t pos_ = 0;
    bool ready_ = false;

};
} // namespace logger
