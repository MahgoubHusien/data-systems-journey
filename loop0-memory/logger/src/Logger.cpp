#include "Logger.h"
#include <stdexcept>
#include <cstring>

namespace logger {

Logger::Logger(Config cfg) : pos_(0), flush_on_each_write_(cfg.flush_on_each_write) {
    if (cfg.buffer_size == 0) throw std::runtime_error("Buffer Size is 0");

    file_.open(cfg.path, std::ios::app);
    if (!file_.is_open()) throw std::runtime_error("Failed to open log file");

    buffer_ = std::make_unique<char[]>(cfg.buffer_size);
    cap_ = cfg.buffer_size;

    {
        std::lock_guard<std::mutex> lock(m_);
        alive_ = true;
        shutdown_ = false;
        q_cap_ = 1024; // choose policy: 1k entries
    }

    worker_ = std::thread(&Logger::worker_loop, this);
}

std::string_view Logger::levelToString(Level level) noexcept {
    switch (level) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::log(Level lvl, std::string_view message) noexcept {
    // format outside lock
    std::string entry;
    {
        const auto level = levelToString(lvl);
        entry.reserve(level.size() + message.size() + 4);
        entry.push_back('[');
        entry.append(level);
        entry.append("] ");
        entry.append(message);
        entry.push_back('\n');
    }

    {
        std::lock_guard<std::mutex> lock(m_);
        if (!alive_ || shutdown_) return;

        if (q_.size() == q_cap_) {
            q_.pop_front(); // DROP OLDEST
        }
        q_.push_back(std::move(entry));
    }
    cv_wakeup_.notify_one();
}

void Logger::worker_loop() noexcept {
    while (true) {
        std::deque<std::string> local;
        std::uint64_t local_flush_req = 0;

        {
            std::unique_lock<std::mutex> lock(m_);
            cv_wakeup_.wait(lock, [&] {
                return shutdown_ || !q_.empty() || flush_req_ != flush_ack_;
            });

            if (flush_req_ != flush_ack_) {
                local_flush_req = flush_req_;
            }

            local.swap(q_);

            if (shutdown_ && local.empty() && local_flush_req == 0) {
                break;
            }
        } // unlock

        // write batch (no mutex)
        while (!local.empty()) {
            write_one_(local.front());
            local.pop_front();
        }

        // push our internal buffer to the stream after each batch
        flush_unlocked();

        // strong flush: force stream flush + ack ticket
        if (local_flush_req != 0) {
            if (file_.is_open()) file_.flush();

            {
                std::lock_guard<std::mutex> lock(m_);
                flush_ack_ = std::max(flush_ack_, local_flush_req);
            }
            cv_flushed_.notify_all();
        }
    }

    // final drain
    flush_unlocked();
    if (file_.is_open()) file_.flush();

    // unblock any flush waiters
    {
        std::lock_guard<std::mutex> lock(m_);
        flush_ack_ = flush_req_;
    }
    cv_flushed_.notify_all();
}

void Logger::write_one_(std::string_view msg) noexcept {
    if (!file_.is_open() || !buffer_) return;

    const std::size_t needed = msg.size();

    // large message: flush buffer then write directly
    if (needed > cap_) {
        if (pos_ > 0) flush_unlocked();
        file_.write(msg.data(), static_cast<std::streamsize>(needed));
        if (flush_on_each_write_) file_.flush();
        return;
    }

    if (pos_ + needed > cap_) {
        flush_unlocked();
    }

    std::memcpy(buffer_.get() + pos_, msg.data(), needed);
    pos_ += needed;

    if (flush_on_each_write_) {
        flush_unlocked();
        file_.flush();
    }
}

void Logger::flush_unlocked() noexcept {
    if (!file_.is_open() || !buffer_ || pos_ == 0) return;
    file_.write(buffer_.get(), static_cast<std::streamsize>(pos_));
    pos_ = 0;
}

void Logger::flush() noexcept {
    std::uint64_t req = 0;
    {
        std::lock_guard<std::mutex> lock(m_);
        if (!alive_ || shutdown_) return;
        req = ++flush_req_;
    }
    cv_wakeup_.notify_one();

    std::unique_lock<std::mutex> lock(m_);
    cv_flushed_.wait(lock, [&] {
        return flush_ack_ >= req || shutdown_ || !alive_;
    });
}

void Logger::shutdown() noexcept {
    std::thread t;

    {
        std::lock_guard<std::mutex> lock(m_);
        if (!alive_) return;
        shutdown_ = true;
        t = std::move(worker_);
    }

    cv_wakeup_.notify_one();
    cv_flushed_.notify_all();

    if (t.joinable()) t.join();

    {
        std::lock_guard<std::mutex> lock(m_);
        alive_ = false;
    }
    cv_flushed_.notify_all();

    // cleanup (worker is gone)
    if (file_.is_open()) file_.close();
    buffer_.reset();
    cap_ = 0;
    pos_ = 0;
    flush_on_each_write_ = false;
}

} // namespace logger
