# Moonbeam (libmoonbeam)

Moonbeam is a lightweight, cross-platform C++17 connection testing library designed for game streaming clients (specifically Moonlight ports on platforms like PS Vita, Wii U, and desktop systems). It evaluates the network connection quality between a client device and a host running Sunshine or GeForce Experience.

## Features

- **Ping & Jitter Diagnostic**: Measures average latency, minimum/maximum latency, packet loss, and jitter (standard deviation of pings) over a sequence of HTTP requests to the Sunshine unauthenticated `/serverinfo` endpoint.
- **Bandwidth & Speed Test**: Simulates streaming traffic by downloading the Sunshine host icon repeatedly over HTTPS for a fixed duration to calculate real-world throughput in Mbps.
- **Connection Rating & Recommendations**: Evaluates network statistics against predefined thresholds (Excellent, Good, Fair, Poor) and provides actionable optimization advice.
- **Asynchronous Execution & Cancellation**: Runs on background threads and can be safely cancelled mid-test.

## How it works

1. **Phase 1: Latency & Jitter Check**
   - Sends 30 rapid HTTP GET requests to `http://<host_ip>:47989/serverinfo` with 20ms delays.
   - Calculates the average, min, and max latency.
   - Computes jitter using standard deviation of the latency values.

2. **Phase 2: Speed / Throughput Check**
   - Downloads `https://<host_ip>:47990/images/sunshine.ico` repeatedly in a loop for 2 seconds (with SSL verification disabled to avoid certificate trust issues on homebrew devices).
   - Computes the average download speed in Megabits per second (Mbps).

3. **Phase 3: Connection Rating & Advice**
   - Combines the measured latency, jitter, packet loss, and throughput to classify the link.
   - Outputs recommendation strings tailored to the client's current link quality.

## Integration

To integrate Moonbeam into your CMake project:

```cmake
add_subdirectory(third_party/moonbeam)
target_link_libraries(your_target PRIVATE moonbeam)
```

And in your C++ code:

```cpp
#include <moonbeam.hpp>

// Create tester
moonbeam::ConnectionTester tester("192.168.1.100");

// Set progress callback
tester.setProgressCallback([](float progress, const std::string& status) {
    std::cout << "[" << (progress * 100) << "%] " << status << std::endl;
});

// Run diagnostics
auto result = tester.run();

if (result.success) {
    std::cout << "Rating: " << result.rating << std::endl;
    std::cout << "Recommendation: " << result.recommendation << std::endl;
} else {
    std::cerr << "Error: " << result.error_message << std::endl;
}
```

## License

Apache License 2.0.
