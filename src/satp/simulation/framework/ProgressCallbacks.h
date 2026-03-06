#pragma once

#include <cstddef>
#include <functional>

using namespace std;

namespace satp::evaluation {
    struct ProgressCallbacks {
        function<void(size_t)> onStart;
        function<void(size_t)> onAdvance;
        function<void()> onFinish;
    };
} // namespace satp::evaluation
