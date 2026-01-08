#pragma once

#include <string_view>
#include <fstream>
#include <cstddef>
#include <iostream>

#include <memory>


namespace logger{

class Logger{
public:
    explicit Logger(std::string_view file);
    // We delete copy construction and copy assignment so duplicating a resource-owning object is impossible at compile time.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // We enable move construction and assignment
    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    ~Logger() noexcept = default;
    

    void logLine(std::string_view message) noexcept;
    void flush() noexcept;



private:
    // A pointer to a runtime-managed object that knows how to talk to the OS for file operations.
    std::ofstream file_;
    static constexpr size_t BUFFER_SIZE = 4096;
    std::unique_ptr<char[]> buffer_ = nullptr; // 4KB buffer for logging
    std::size_t pos_ = 0;

};
} // namespace logger
