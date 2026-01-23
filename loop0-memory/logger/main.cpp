// main.cpp — Logger v6 stress test (16 producers, slow consumer, verifies clean shutdown)
// Assumes your Logger supports: log(Level, string_view), flush(), shutdown(), alive()
// and writes to cfg.path.

#include "Logger.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using logger::Logger;
using logger::Level;
using logger::Config;

static std::size_t count_lines_in_file(const std::string& path) {
    std::ifstream in(path);
    std::size_t n = 0;
    std::string line;
    while (std::getline(in, line)) ++n;
    return n;
}

static bool validate_line_format(const std::string& line) {
    // Expected like: "[INFO] ..." (we won't validate message content beyond prefix)
    if (line.size() < 8) return false;
    if (line[0] != '[') return false;
    auto close = line.find(']');
    if (close == std::string::npos) return false;
    if (close + 1 >= line.size()) return false;
    if (line[close + 1] != ' ') return false;

    const std::string level = line.substr(1, close - 1);
    return level == "DEBUG" || level == "INFO" || level == "WARN" || level == "ERROR";
}

int main() {
    const std::string path = "logger_v6_stress.log";

    // Start fresh each run
    std::remove(path.c_str());

    Config cfg;
    cfg.path = path;
    cfg.buffer_size = 4096;
    cfg.flush_on_each_write = false;

    Logger log(cfg);

    // --- Stress parameters ---
    constexpr int kThreads = 16;
    constexpr int kPerThread = 50'000;           // 16 * 50k = 800k attempted logs
    constexpr int kTotalAttempted = kThreads * kPerThread;

    // We'll create bursts to overload the worker. If your worker is not slowed,
    // this still stresses the queue, but to really force backpressure/drops,
    // slow the worker in Logger::worker_loop (sleep after batch) as part of Week 2.
    //
    // If you already added sleep in worker_loop, great. If not, you'll still
    // test correctness + termination.

    std::atomic<int> ready{0};
    std::atomic<bool> start{false};

    // Capture any unexpected exceptions (shouldn't happen; log() is noexcept)
    std::atomic<bool> had_error{false};
    std::mutex err_mu;
    std::string err_msg;

    auto producer = [&](int tid) {
        ready.fetch_add(1, std::memory_order_relaxed);
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        try {
            for (int i = 0; i < kPerThread; ++i) {
                // Mix levels so formatting is exercised
                Level lvl = (i % 4 == 0) ? Level::Debug
                          : (i % 4 == 1) ? Level::Info
                          : (i % 4 == 2) ? Level::Warn
                                         : Level::Error;

                // Include identifiers so you can spot drops/out-of-order in manual inspection
                // Note: order across threads is not guaranteed in async logging (that's fine).
                std::string msg = "t=" + std::to_string(tid) + " i=" + std::to_string(i);
                log.log(lvl, msg);

                // Create producer pressure (burstier traffic)
                if ((i % 1024) == 0) {
                    // short pause to create bursts instead of perfectly even traffic
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                }
            }
        } catch (const std::exception& e) {
            had_error.store(true, std::memory_order_release);
            std::lock_guard<std::mutex> lk(err_mu);
            err_msg = e.what();
        } catch (...) {
            had_error.store(true, std::memory_order_release);
            std::lock_guard<std::mutex> lk(err_mu);
            err_msg = "unknown exception";
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) threads.emplace_back(producer, t);

    // Wait for all threads to be ready
    while (ready.load(std::memory_order_acquire) < kThreads) {
        std::this_thread::yield();
    }
    start.store(true, std::memory_order_release);

    for (auto& th : threads) th.join();

    // Force all queued logs to be processed
    log.flush();

    // Clean shutdown (joins worker)
    log.shutdown();

    if (had_error.load(std::memory_order_acquire)) {
        std::cerr << "FAIL: producer threw exception: " << err_msg << "\n";
        return 2;
    }

    // Validate output
    const std::size_t lines = count_lines_in_file(path);

    // With drop-oldest, you may drop under load, so lines <= attempted.
    // It should never exceed attempted.
    if (lines > static_cast<std::size_t>(kTotalAttempted)) {
        std::cerr << "FAIL: more lines than attempted logs\n";
        std::cerr << "Attempted: " << kTotalAttempted << "\n";
        std::cerr << "Actual:    " << lines << "\n";
        return 3;
    }

    // Basic format validation on first N lines (don’t scan gigantic logs)
    std::ifstream in(path);
    std::string line;
    std::size_t checked = 0;
    std::size_t bad = 0;
    const std::size_t kMaxCheck = 50'000;

    while (checked < kMaxCheck && std::getline(in, line)) {
        if (!validate_line_format(line)) ++bad;
        ++checked;
    }

    if (bad != 0) {
        std::cerr << "FAIL: bad line format detected\n";
        std::cerr << "Checked: " << checked << "  Bad: " << bad << "\n";
        return 4;
    }

    // Print summary
    std::cout << "Attempted logs: " << kTotalAttempted << "\n";
    std::cout << "Written lines : " << lines << "\n";
    if (lines < static_cast<std::size_t>(kTotalAttempted)) {
        std::cout << "Dropped lines : " << (kTotalAttempted - lines) << " (expected under load)\n";
    } else {
        std::cout << "Dropped lines : 0\n";
    }

    std::cout << "PASS: stress test completed, program terminated cleanly.\n";
    return 0;
}
