#pragma once
#include <chrono>
#include <cstddef>
#include <format>
#include <iosfwd>
#include <iostream>

namespace satp::util {
    class ProgressBar {
    public:
        explicit ProgressBar(std::size_t total,
                             std::ostream &os = std::cerr,
                             std::size_t width = 50,
                             std::size_t updateEvery = 1'000);

        void tick(std::size_t n = 1);

        void finish();

    private:
        void draw();

        const std::size_t total_, width_, updateEvery_;
        std::size_t count_ = 0;
        std::ostream &os_;
        std::chrono::steady_clock::time_point start_;
    };
} // namespace satp::util
