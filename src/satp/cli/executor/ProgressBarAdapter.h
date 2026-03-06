#pragma once

#include <cstddef>
#include <iostream>
#include <memory>

#include "satp/ProgressBar.h"
#include "satp/simulation/framework/ProgressCallbacks.h"

using namespace std;

namespace satp::cli::executor {
    class ProgressBarAdapter {
    public:
        [[nodiscard]] satp::evaluation::ProgressCallbacks callbacks() {
            return {
                [this](const size_t totalTicks) {
                    bar_ = make_unique<satp::util::ProgressBar>(totalTicks, cout, 50, 10'000);
                },
                [this](const size_t ticks) {
                    if (bar_) {
                        bar_->tick(ticks);
                    }
                },
                [this]() {
                    if (bar_) {
                        bar_->finish();
                        cout.flush();
                        bar_.reset();
                    }
                }
            };
        }

    private:
        unique_ptr<satp::util::ProgressBar> bar_;
    };
} // namespace satp::cli::executor
