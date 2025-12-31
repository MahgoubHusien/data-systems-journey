#pragma once

#include <string_view>
#include <fstream>


namespace logger{

class Logger{
public:
    explicit Logger(const std::string_view file);
    // We delete copy construction and copy assignment so duplicating a resource-owning object is impossible at compile time.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // We enable move construction and assignment
    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    ~Logger() noexcept = default;
    void logLine(const std::string_view message);
    bool isOpen() const noexcept { return file_.is_open(); }


private:
    // A pointer to a runtime-managed object that knows how to talk to the OS for file operations.
    std::ofstream file_;
};
} // namespace logger
