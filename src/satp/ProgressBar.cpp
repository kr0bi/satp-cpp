#include "./ProgressBar.h"
#include <iostream>

using namespace std;
using namespace satp::util;

ProgressBar::ProgressBar(size_t total,
                         ostream &os,
                         size_t width,
                         size_t updateEvery)
    : total_(total), width_(width), updateEvery_(updateEvery),
      os_(os), start_(chrono::steady_clock::now()) {
}

void ProgressBar::tick(size_t n) {
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
    const auto pos = static_cast<size_t>(width_ * progress);

    os_ << '\r' << '[';
    for (size_t i = 0; i < width_; ++i)
        os_ << (i < pos ? '=' : (i == pos ? '>' : ' '));
    os_ << "] " << format("{:3.0f}%", progress * 100);

    const auto elapsed = chrono::steady_clock::now() - start_;
    const auto elapsed_s = chrono::duration_cast<chrono::seconds>(elapsed).count();
    const auto eta_s = progress > 0 ? static_cast<long>(elapsed_s / progress - elapsed_s) : 0;

    os_ << " | elapsed: " << elapsed_s << " s"
            << " | ETA: " << eta_s << " s" << flush;
}
