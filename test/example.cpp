#include <thread>

#include "doctest/doctest.h"
#include "riften/deque.hpp"

TEST_CASE("Examples") {
    // #include <thread>

    // #include "riften/deque.hpp"

    // Work-stealing deque of ints
    riften::Deque<int> deque;

    // One thread can push and pop items from one end (like a stack)
    std::thread owner([&]() {
        for (int i = 0; i < 10000; i = i + 1) {
            deque.emplace(i);
        }
        while (!deque.empty()) {
            [[maybe_unused]] std::optional item = deque.pop();
        }
    });

    // While multiple (any) threads can steal items from the other end
    std::thread thief([&]() {
        while (!deque.empty()) {
            [[maybe_unused]] std::optional item = deque.steal();
        }
    });

    owner.join();
    thief.join();

    REQUIRE(deque.empty());
}