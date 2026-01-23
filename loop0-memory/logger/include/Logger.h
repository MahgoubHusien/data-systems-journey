#pragma once

#include <string_view>
#include <fstream>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <thread>
#include <algorithm>

namespace logger {

enum class Level : std::uint8_t {
    Debug,
    Info,
    Warn,
    Error
};

struct Config {
    std::string path;
    std::size_t buffer_size = 4096;       // file write buffer (bytes)
    bool flush_on_each_write = false;     // forces immediate flush per log
};

// Thread-safety:
// - log(), flush(), shutdown() are safe to call concurrently.
// - Logger must not be destroyed while other threads may call its methods.
class Logger {
public:
    explicit Logger(Config cfg);
    Logger() = delete;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // v6: owning a thread => non-movable (keep it simple + correct)
    Logger(Logger&&) noexcept = delete;
    Logger& operator=(Logger&&) noexcept = delete;

    ~Logger() noexcept { shutdown(); }

    void log(Level level, std::string_view message) noexcept;
    void flush() noexcept;
    void shutdown() noexcept;

    bool alive() const noexcept {
        std::lock_guard<std::mutex> lock(m_);
        return alive_;
    }

private:
    static std::string_view levelToString(Level level) noexcept;

    // worker
    void worker_loop() noexcept;
    void write_one_(std::string_view msg) noexcept;
    void flush_unlocked() noexcept; // worker-only (writes buffer_ -> file_)

private:
    // shared state
    mutable std::mutex m_;
    std::condition_variable cv_wakeup_;
    std::condition_variable cv_flushed_;

    std::deque<std::string> q_;
    std::size_t q_cap_ = 1024; // bounded entries (drop-oldest)

    std::thread worker_;
    bool shutdown_ = false;
    bool alive_ = false;

    // strong flush handshake
    std::uint64_t flush_req_ = 0;
    std::uint64_t flush_ack_ = 0;

    // file + buffer (worker-only during normal operation)
    std::ofstream file_;
    std::unique_ptr<char[]> buffer_ = nullptr;
    std::size_t cap_ = 0;
    std::size_t pos_ = 0;
    bool flush_on_each_write_ = false;
};

} // namespace logger
