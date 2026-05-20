#include "moonbeam.hpp"
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace moonbeam {

ConnectionTester::ConnectionTester(const std::string& host_ip, int http_port, int https_port)
    : ip(host_ip), port_http(http_port), port_https(https_port), cancelled(false), progress_cb(nullptr) {}

ConnectionTester::~ConnectionTester() {}

void ConnectionTester::setProgressCallback(std::function<void(float progress, const std::string& status)> cb) {
    progress_cb = cb;
}

void ConnectionTester::cancel() {
    cancelled = true;
}

bool ConnectionTester::isCancelled() const {
    return cancelled;
}

void ConnectionTester::updateProgress(float progress, const std::string& status) {
    if (progress_cb) {
        progress_cb(progress, status);
    }
}

size_t ConnectionTester::discardWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    (void)contents;
    (void)userp;
    return size * nmemb;
}

size_t ConnectionTester::speedWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    (void)contents;
    size_t total_size = size * nmemb;
    size_t* bytes_counter = static_cast<size_t*>(userp);
    if (bytes_counter) {
        *bytes_counter += total_size;
    }
    return total_size;
}

int ConnectionTester::progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    ConnectionTester* tester = static_cast<ConnectionTester*>(clientp);
    if (tester && tester->isCancelled()) {
        return 1; // Abort curl transfer
    }
    return 0;
}

bool ConnectionTester::runPingTest(float& min_ping, float& max_ping, float& avg_ping, float& jitter, float& loss_pct) {
    constexpr int total_pings = 30;
    std::vector<float> pings;
    int failed_count = 0;

    std::string url = "http://" + ip + ":" + std::to_string(port_http) + "/serverinfo";

    for (int i = 0; i < total_pings; ++i) {
        if (cancelled) return false;

        updateProgress((float)i / total_pings * 0.5f, "Measuring latency & jitter (Ping " + std::to_string(i + 1) + "/" + std::to_string(total_pings) + ")...");

        CURL* curl = curl_easy_init();
        if (!curl) {
            failed_count++;
            continue;
        }

        // Configure curl request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardWriteCallback);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 400L); // Quick timeout for local network pings
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);

        auto start = std::chrono::high_resolution_clock::now();
        CURLcode res = curl_easy_perform(curl);
        auto end = std::chrono::high_resolution_clock::now();

        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK && response_code == 200) {
            float duration = std::chrono::duration<float, std::milli>(end - start).count();
            pings.push_back(duration);
        } else {
            if (res != CURLE_ABORTED_BY_CALLBACK) {
                failed_count++;
            } else {
                return false; // User cancelled
            }
        }

        // Small delay between pings to get dynamic measurements
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    loss_pct = ((float)failed_count / total_pings) * 100.0f;

    if (pings.empty()) {
        return false;
    }

    min_ping = *std::min_element(pings.begin(), pings.end());
    max_ping = *std::max_element(pings.begin(), pings.end());
    avg_ping = std::accumulate(pings.begin(), pings.end(), 0.0f) / pings.size();

    // Calculate Jitter: average of absolute differences between consecutive pings
    if (pings.size() > 1) {
        float sum_diff = 0.0f;
        for (size_t i = 0; i < pings.size() - 1; ++i) {
            sum_diff += std::abs(pings[i] - pings[i + 1]);
        }
        jitter = sum_diff / (pings.size() - 1);
    } else {
        jitter = 0.0f;
    }

    return true;
}

bool ConnectionTester::runSpeedTest(float& speed_mbps) {
    std::string url = "https://" + ip + ":" + std::to_string(port_https) + "/images/sunshine.ico";

    size_t total_bytes = 0;
    auto test_start = std::chrono::high_resolution_clock::now();
    double elapsed_seconds = 0.0;
    constexpr double target_duration = 2.0; // 2 seconds speed test

    int iterations = 0;

    while (elapsed_seconds < target_duration) {
        if (cancelled) return false;

        updateProgress(0.5f + (float)(elapsed_seconds / target_duration) * 0.45f, "Measuring download bandwidth...");

        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // Disable SSL certificate verification as Sunshine uses self-signed certs
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, speedWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &total_bytes);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            if (res == CURLE_ABORTED_BY_CALLBACK) {
                return false; // User cancelled
            }
            // If the server doesn't respond or file is missing, we fail the test
            if (iterations == 0) {
                return false;
            }
            break; // If subsequent requests fail, break and calculate with what we have
        }

        iterations++;
        auto now = std::chrono::high_resolution_clock::now();
        elapsed_seconds = std::chrono::duration<float>(now - test_start).count();
    }

    if (total_bytes == 0 || elapsed_seconds == 0.0) {
        return false;
    }

    // speed = (bytes * 8 bits/byte) / (seconds * 1,000,000 bits/megabit)
    speed_mbps = (static_cast<double>(total_bytes) * 8.0) / (elapsed_seconds * 1000000.0);

    return true;
}

TestResult ConnectionTester::run() {
    TestResult result;
    cancelled = false;

    updateProgress(0.0f, "Initializing Connection Test...");

    // 1. Run Ping Test
    if (!runPingTest(result.min_ping_ms, result.max_ping_ms, result.avg_ping_ms, result.jitter_ms, result.packet_loss_pct)) {
        if (cancelled) {
            result.error_message = "Test cancelled by user.";
            return result;
        }
        result.error_message = "Failed to communicate with host. Verify host IP and status.";
        return result;
    }

    // 2. Run Speed Test
    if (!runSpeedTest(result.speed_mbps)) {
        if (cancelled) {
            result.error_message = "Test cancelled by user.";
            return result;
        }
        result.error_message = "Failed to download speed assets. Verify host HTTPS server.";
        return result;
    }

    updateProgress(0.95f, "Finalizing report...");

    result.success = true;

    // 3. Diagnose and Recommend
    // Criteria weighting
    if (result.packet_loss_pct > 2.0f || result.avg_ping_ms > 50.0f || result.speed_mbps < 5.0f) {
        result.rating = "Poor";
        result.recommendation = "Connection is unstable. High latency or packet loss will cause severe lagging and visual glitches. Switch to a 5GHz WiFi network, move closer to your router, or use an Ethernet connection.";
    } else if (result.packet_loss_pct > 0.5f || result.avg_ping_ms > 25.0f || result.jitter_ms > 5.0f || result.speed_mbps < 15.0f) {
        result.rating = "Fair";
        result.recommendation = "Connection is average. You might experience occasional micro-stutters. Recommended settings: Native resolution (e.g. 544p for PS Vita, 720p for Wii U) at 30 FPS, and set the bitrate between 5 to 10 Mbps. Enabling HEVC is highly advised.";
    } else if (result.avg_ping_ms > 12.0f || result.jitter_ms > 2.0f || result.speed_mbps < 30.0f) {
        result.rating = "Good";
        result.recommendation = "Connection is stable. Good for smooth gameplay. Recommended settings: Native resolution (544p/720p) at 60 FPS, with a bitrate between 10 to 18 Mbps. Experience should be fluid.";
    } else {
        result.rating = "Excellent";
        result.recommendation = "Outstanding connection. Perfect latency and bandwidth. Recommended settings: Stream at maximum native resolution, 60 FPS, with a bitrate of 20+ Mbps. Very close to native, lag-free gameplay.";
    }

    updateProgress(1.0f, "Completed!");
    return result;
}

} // namespace moonbeam
