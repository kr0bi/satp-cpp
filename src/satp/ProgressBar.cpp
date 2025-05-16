#include "./ProgressBar.h"
#include <iostream>

using namespace satp::util;

ProgressBar::ProgressBar(std::size_t total,
                         std::ostream &os,
                         std::size_t width,
                         std::size_t updateEvery)
    : total_(total), width_(width), updateEvery_(updateEvery),
      os_(os), start_(std::chrono::steady_clock::now()) {
}

void ProgressBar::tick(std::size_t n) {
    count_ += n;
    if (count_ % updateEvery_ == 0 || count_ == total_) draw();
}

void ProgressBar::finish() {
    count_ = total_;
    draw();
    os_ << '\n';
}

void ProgressBar::draw() {
    const double progress = static_cast<double>(count_) / total_;
    const auto pos = static_cast<std::size_t>(width_ * progress);

    os_ << '\r' << '[';
    for (std::size_t i = 0; i < width_; ++i)
        os_ << (i < pos ? '=' : (i == pos ? '>' : ' '));
    os_ << "] " << std::format("{:3.0f}%", progress * 100);

    const auto elapsed = std::chrono::steady_clock::now() - start_;
    const auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    const auto eta_s = progress > 0 ? static_cast<long>(elapsed_s / progress - elapsed_s) : 0;

    os_ << " | elapsed: " << elapsed_s << " s"
            << " | ETA: " << eta_s << " s" << std::flush;
}
