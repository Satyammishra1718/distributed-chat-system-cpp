#include "../../include/core/threading/ThreadPool.h"
#include <iostream>
#include <cassert>

using namespace DistributedChat;

int main() {
    std::cout << "[ThreadPoolTest] Initializing thread pool with 2 threads...\n";
    ThreadPool pool(2);

    auto f1 = pool.Enqueue([]() {
        return 42;
    });

    auto f2 = pool.Enqueue([](int x) {
        return x + 10;
    }, 20);

    assert(f1.get() == 42);
    assert(f2.get() == 30);

    std::cout << "[ThreadPoolTest] ThreadPool tests completed successfully!\n";
    return 0;
}