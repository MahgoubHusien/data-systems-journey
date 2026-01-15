#include "Logger.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using logger::Level;
using logger::Logger;
using logger::Config;

// Simple helper: return true if `s` starts with `prefix`
static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

int main() {
    // ----- Test parameters -----
    const int kThreads = 16;            // 8–16 per spec
    const int kLinesPerThread = 5000;   // "thousands" per spec
    const std::string path = "logger_v5_stress.log";

    // Make the test deterministic-ish:
    // - use buffering (flush_on_each_write = false) to stress buffer draining
    // - keep buffer small-ish to force frequent drains
    Config cfg;
    cfg.path = path;
    cfg.buffer_size = 256;              // small to force flush_unlocked() often
    cfg.flush_on_each_write = false;

    // Truncate old file first (so line counts are deterministic).
    {
        std::ofstream trunc(path, std::ios::trunc);
        if (!trunc.is_open()) {
            std::cerr << "Failed to truncate file: " << path << "\n";
            return 1;
        }
    }

    Logger log(cfg);

    std::atomic<int> started{0};
    std::atomic<bool> go{false};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    // Each thread writes unique lines:
    // Format: "[INFO] T=<tid> I=<i> MSG=..."
    // This lets us validate structure and detect corruption.
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([t, &log, &started, &go]() {
            started.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {
                // spin until all threads ready
                std::this_thread::yield();
            }

            // Write many lines
            for (int i = 0; i < kLinesPerThread; ++i) {
                // Keep messages small-ish but varied
                std::string msg = "T=" + std::to_string(t) + " I=" + std::to_string(i) +
                                  " MSG=hello_logger_v5";
                log.log(Level::Info, msg);
            }
        });
    }

    // Wait until all threads are ready, then start together
    while (started.load(std::memory_order_relaxed) < kThreads) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);

    for (auto& th : threads) th.join();

    // Force any remaining buffer to be written
    log.flush();

    // Idempotent shutdown requirement: call twice
    log.shutdown();
    log.shutdown();

    // ----- Validate output file -----
    const long long expected_lines = 1LL * kThreads * kLinesPerThread;

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Failed to open output log file for validation: " << path << "\n";
        return 1;
    }

    long long line_count = 0;
    long long bad_lines = 0;

    std::string line;
    while (std::getline(in, line)) {
        ++line_count;

        // Must start with "[INFO] "
        if (!starts_with(line, "[INFO] ")) {
            ++bad_lines;
            continue;
        }

        // Must contain our expected fields
        // We don't parse deeply; we just ensure the line structure survived.
        if (line.find("T=") == std::string::npos ||
            line.find("I=") == std::string::npos ||
            line.find("MSG=") == std::string::npos) {
            ++bad_lines;
            continue;
        }

        // Must not contain newline characters (getline strips '\n', but corruption could embed '\r' etc.)
        // Optional: check non-empty
        if (line.empty()) {
            ++bad_lines;
            continue;
        }
    }

    std::cout << "Expected lines: " << expected_lines << "\n";
    std::cout << "Actual lines:   " << line_count << "\n";
    std::cout << "Bad lines:      " << bad_lines << "\n";

    if (line_count != expected_lines) {
        std::cerr << "FAIL: line count mismatch.\n";
        return 2;
    }
    if (bad_lines != 0) {
        std::cerr << "FAIL: found corrupted/malformed lines.\n";
        return 3;
    }

    std::cout << "PASS: stress test OK. No interleaving observed.\n";
    return 0;
}
