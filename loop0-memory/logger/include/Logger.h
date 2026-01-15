#pragma once

#include <string_view>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <mutex>


namespace logger{

enum class Level : std::uint8_t {
    Debug,
    Info,
    Warn,
    Error
};

struct Config {
    std::string path;
    std::size_t buffer_size = 4096;
    bool flush_on_each_write = false;
};


// Thread-safety:
// - log(), flush(), shutdown() are safe to call concurrently.
// - Logger must not be moved/destroyed while other threads may call its methods.
class Logger{
public:
    explicit Logger(Config cfg);
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
    bool alive() const noexcept { std::lock_guard<std::mutex> lock(m_); return alive_; }



private:
    mutable std::mutex m_;
    std::ofstream file_;
    std::unique_ptr<char[]> buffer_ = nullptr; 
    std::size_t cap_ = 0;
    std::size_t pos_ = 0;

    bool flush_on_each_write_ = false;
    bool alive_ = false;
    static std::string_view levelToString(Level level) noexcept;
    void flush_unlocked() noexcept;
    void shutdown_unlocked() noexcept;
};
} // namespace logger
