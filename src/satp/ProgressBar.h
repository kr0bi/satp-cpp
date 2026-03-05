#pragma once
#include <chrono>
#include <cstddef>
#include <format>
#include <iosfwd>
#include <iostream>

using namespace std;

namespace satp::util {
    class ProgressBar {
    public:
        explicit ProgressBar(size_t total,
                             ostream &os = cerr,
                             size_t width = 50,
                             size_t updateEvery = 1'000);

        void tick(size_t n = 1);

        void finish();

    private:
        void draw();

        const size_t total_, width_, updateEvery_;
        size_t count_ = 0;
        ostream &os_;
        chrono::steady_clock::time_point start_;
    };
} // namespace satp::util
