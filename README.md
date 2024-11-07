# AsyncQueue

A header-only C++ library implementing a thread-safe asynchronous queue with timeout support.

## Features
- Thread-safe operations
- Configurable capacity
- Timeout support for push/pop operations
- Move semantics support
- Extension support through virtual hooks
- Header-only implementation

## Integration

### Method 1: CMake FetchContent
```cmake
include(FetchContent)

FetchContent_Declare(
    async_queue
    GIT_REPOSITORY https://github.com/HarryPehkonen/AsyncQueue.git
    GIT_TAG v0.1.2
)

FetchContent_MakeAvailable(async_queue)

# Link against the library
target_link_libraries(your_target PRIVATE async_queue::async_queue)
```

### Method 2: Manual Installation
```bash
git clone https://github.com/HarryPehkonen/AsyncQueue.git
cd async_queue
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

Then in your project's CMakeLists.txt:
```cmake
find_package(async_queue REQUIRED)
target_link_libraries(your_target PRIVATE async_queue::async_queue)
```

## Usage Example
```cpp
#include <async_queue/async_queue.hpp>

async_queue::AsyncQueue<int> queue(100);  // Queue with capacity 100

// Producer
queue.push(42);
queue.try_push(43, std::chrono::milliseconds(100));

// Consumer
auto value = queue.pop();
if (value) {
    std::cout << "Received: " << *value << std::endl;
}
```

### push vs try push:

1. `push` is a blocking call - it will wait indefinitely until:
   - There is space in the queue (if at capacity)
   - OR the queue is closed
2. `try_push` has a timeout parameter and will only wait up to that duration before:
   - Successfully adding the item if space becomes available
   - OR returning false if the timeout expires
   - OR returning false if the queue is closed

### pop vs try_pop:

1. `pop` is a blocking call - it will wait indefinitely until:
   - There is an item to retrieve
   - OR the queue is closed
2. `try_pop` has a timeout parameter and will only wait up to that duration before:
   - Successfully retrieving an item if one becomes available
   - OR returning nullopt if the timeout expires
   - OR returning nullopt if the queue is closed and empty


### The following tests demonstrate the key differences

PushBlocking, TryPushNonBlocking, PopBlocking, TryPopNonBlocking, TryPushSuccessful, and TryPopSuccessful

1. `push`/`pop` will wait indefinitely:
   - Good when you know the operation must eventually succeed
   - Useful for producer-consumer scenarios where blocking is acceptable

2. `try_push`/`try_pop` provide timeout control:
   - Good for handling timeouts gracefully
   - Useful when you need to avoid deadlocks
   - Better for scenarios where waiting indefinitely is not acceptable

3. The try_* versions are particularly useful when:
   - You need to implement timeouts for reliability
   - You want to avoid potential deadlocks
   - You need to handle failure cases explicitly
   - You're implementing cancelable operations

## Building Tests
```bash
mkdir build && cd build
cmake -DASYNC_QUEUE_BUILD_TESTS=ON ..
cmake --build .
ctest
```
## License

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
