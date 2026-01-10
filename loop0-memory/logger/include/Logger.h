#pragma once

#include <string_view>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <memory>


namespace logger{

enum class Level : std::uint8_t {
    Debug,
    Info,
    Warn,
    Error
};

class Logger{
public:
    explicit Logger(std::string_view file);
    Logger() = delete;
    // We delete copy construction and copy assignment so duplicating a resource-owning object is impossible at compile time.
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // We enable move construction and assignment
    Logger(Logger&&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    ~Logger() noexcept { shutdown(); }
    

    void log(Level level, std::string_view message) noexcept;

    void flush() noexcept;
    void shutdown() noexcept;
    bool alive() const noexcept { return alive_; }



private:
    std::ofstream file_;
    static constexpr size_t BUFFER_SIZE = 4096;
    std::unique_ptr<char[]> buffer_ = nullptr; // 4KB buffer for logging
    std::size_t pos_ = 0;
    bool alive_ = false;
    static std::string_view levelToString(Level level) noexcept;
};
} // namespace logger
