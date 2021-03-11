// The contents of this file are from: https://github.com/taskflow/work-stealing-queue

#include <atomic>
#include <deque>
#include <queue>
#include <random>
#include <set>
#include <thread>

#include "doctest/doctest.h"
#include "riften/deque.hpp"

// ============================================================================
// riften::Deque tests
// ============================================================================

// Procedure: wsq_test_owner
void wsq_test_owner() {
    int64_t cap = 2;

    riften::Deque<int> queue(cap);
    std::deque<int> gold;

    REQUIRE(queue.capacity() == 2);
    REQUIRE(queue.empty());

    for (int i = 2; i <= (1 << 16); i <<= 1) {
        REQUIRE(queue.empty());

        for (int j = 0; j < i; ++j) {
            queue.emplace(j);
        }

        for (int j = 0; j < i; ++j) {
            auto item = queue.pop();
            REQUIRE((item && *item == i - j - 1));
        }
        REQUIRE(!queue.pop());

        REQUIRE(queue.empty());
        for (int j = 0; j < i; ++j) {
            queue.emplace(j);
        }

        for (int j = 0; j < i; ++j) {
            auto item = queue.steal();
            REQUIRE((item && *item == j));
        }
        REQUIRE(!queue.pop());

        REQUIRE(queue.empty());

        for (int j = 0; j < i; ++j) {
            // enqueue
            if (auto dice = ::rand() % 3; dice == 0) {
                queue.emplace(j);
                gold.push_back(j);
            }
            // pop back
            else if (dice == 1) {
                auto item = queue.pop();
                if (gold.empty()) {
                    REQUIRE(!item);
                } else {
                    REQUIRE(*item == gold.back());
                    gold.pop_back();
                }
            }
            // pop front
            else {
                auto item = queue.steal();
                if (gold.empty()) {
                    REQUIRE(!item);
                } else {
                    REQUIRE(*item == gold.front());
                    gold.pop_front();
                }
            }

            REQUIRE(queue.size() == (int)gold.size());
        }

        while (!queue.empty()) {
            auto item = queue.pop();
            REQUIRE((item && *item == gold.back()));
            gold.pop_back();
        }

        REQUIRE(gold.empty());

        REQUIRE(queue.capacity() == i);
    }
}

// Procedure: wsq_test_n_thieves
void wsq_test_n_thieves(int N) {
    int64_t cap = 2;

    riften::Deque<int> queue(cap);

    REQUIRE(queue.capacity() == 2);
    REQUIRE(queue.empty());

    for (int i = 2; i <= (1 << 16); i <<= 1) {
        REQUIRE(queue.empty());

        int p = 0;

        std::vector<std::deque<int>> cdeqs(N);
        std::vector<std::thread> consumers;
        std::deque<int> pdeq;

        auto num_stolen = [&]() {
            int total = 0;
            for (const auto& cdeq : cdeqs) {
                total += static_cast<int>(cdeq.size());
            }
            return total;
        };

        for (int n = 0; n < N; n++) {
            consumers.emplace_back([&, n]() {
                while (num_stolen() + (int)pdeq.size() != i) {
                    if (auto dice = ::rand() % 4; dice == 0) {
                        if (auto item = queue.steal(); item) {
                            cdeqs[n].push_back(*item);
                        }
                    }
                }
            });
        }

        std::thread producer([&]() {
            while (p < i) {
                if (auto dice = ::rand() % 4; dice == 0) {
                    queue.emplace(p++);
                } else if (dice == 1) {
                    if (auto item = queue.pop(); item) {
                        pdeq.push_back(*item);
                    }
                }
            }
        });

        producer.join();

        for (auto& c : consumers) {
            c.join();
        }

        REQUIRE(queue.empty());
        REQUIRE(queue.capacity() <= i);

        std::set<int> set;

        for (const auto& cdeq : cdeqs) {
            for (auto k : cdeq) {
                set.insert(k);
            }
        }

        for (auto k : pdeq) {
            set.insert(k);
        }

        for (int j = 0; j < i; ++j) {
            REQUIRE(set.find(j) != set.end());
        }

        REQUIRE((int)set.size() == i);
    }
}

// ----------------------------------------------------------------------------
// Testcase: WSQTest.Owner
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.Owner" * doctest::timeout(300)) { wsq_test_owner(); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.1Thief
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.1Thief" * doctest::timeout(300)) { wsq_test_n_thieves(1); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.2Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.2Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(2); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.3Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.3Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(3); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.4Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.4Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(4); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.5Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.5Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(5); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.6Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.6Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(6); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.7Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.7Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(7); }

// ----------------------------------------------------------------------------
// Testcase: WSQTest.8Thieves
// ----------------------------------------------------------------------------
TEST_CASE("WSQ.8Thieves" * doctest::timeout(300)) { wsq_test_n_thieves(8); }