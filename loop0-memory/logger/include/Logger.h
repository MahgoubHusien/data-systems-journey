#pragma once

#include <string>
#include <cstdio>

namespace logger{

class Logger{
public:
    explicit Logger(const std::string& file);
    // We delete copy construction and copy assignment so duplicating a resource-owning object is impossible at compile time.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger() noexcept;
    void logLine(const std::string& message);

private:
    // A pointer to a runtime-managed object that knows how to talk to the OS for file operations.
    FILE* file_ = nullptr;

};
} // namespace logger
