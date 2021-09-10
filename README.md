# `riften::Deque` 


A bleeding-edge lock-free, single-producer multi-consumer, Chase-Lev work stealing deque as presented in the paper "Dynamic Circular Work-Stealing Deque" and further improved in the follow up paper: "Correct and Efficient Work-Stealing for Weak Memory Models". 

This implementation is based on:
- https://github.com/taskflow/work-stealing-queue
- https://github.com/ssbl/concurrent-deque

`riften::Deque` places very few constraints on the types which can be placed in the deque (they must be trivially destructible and have nothrow move constructor/assignment operators) and has no memory overhead associated with buffer recycling. 

## Usage

```C++
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
            std::optional item = deque.pop();
        }
    });

    // While multiple (any) threads can steal items from the other end
    std::thread thief([&]() {
        while (!deque.empty()) {
            std::optional item = deque.steal();
        }
    });

    owner.join();
    thief.join();
```

## CMake

This is a single-header library. Additionally, the library can be installed:
```zsh
mkdir build && cd build
cmake ..
make && make install
```
and then imported into your CMake project by including:
```CMake
find_package(RiftenDeque REQUIRED)
```
in your `CMakeLists.txt` file.

### CPM.cmake

The recommended way to consume this library is through [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake), just add:

```CMake
CPMAddPackage("gh:ConorWilliams/ConcurrentDeque#v1.1.0")
```
to your `CMakeLists.txt` and you're good to go!

## Tests

To compile and run the tests:
```zsh
mkdir build && cd build
cmake ../test
make && make test
```

