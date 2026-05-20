#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <map>
#include <vector>

namespace moonbeam {

struct TestResult {
    bool success = false;
    float min_ping_ms = 0.0f;
    float max_ping_ms = 0.0f;
    float avg_ping_ms = 0.0f;
    float jitter_ms = 0.0f;
    float packet_loss_pct = 0.0f;
    float speed_mbps = 0.0f;
    std::string rating;          // "Excellent", "Good", "Fair", "Poor" (localized)
    std::string rating_raw;      // "Excellent", "Good", "Fair", "Poor" (English/raw for logic)
    std::string recommendation;  // Dynamic advice based on console/network
    std::string error_message;
    
    // Server metadata
    std::string host_name;
    std::string server_version;
    std::string server_state;
};

struct BitrateStepResult {
    int bitrate = 0;
    bool stable = false;
    float speed_kbps = 0.0f;
    float avg_latency_ms = 0.0f;
    float min_latency_ms = 0.0f;
    float max_latency_ms = 0.0f;
    float jitter_ms = 0.0f;
    int stutters = 0;
};

struct VideoBitrateResult {
    bool success = false;
    std::vector<BitrateStepResult> steps;
    int highest_stable_bitrate = 0;
    std::string error_message;
};

class ConnectionTester {
public:
    ConnectionTester(const std::string& host_ip, bool limit_to_2_4ghz = false, const std::string& device_name = "Device", const std::string& lang_code = "en", int http_port = 47989, int https_port = 47990);
    ~ConnectionTester();

    // Runs the diagnostic test.
    TestResult run();

    // Runs the video bitrate test
    VideoBitrateResult runVideoBitrateTest(const std::vector<int>& bitrates);

    // Dynamic translate lookup
    std::string translate(const std::string& key, const std::string& fallback = "");

    // Set callback to receive progress updates (progress: 0.0 to 1.0, status: description text)
    void setProgressCallback(std::function<void(float progress, const std::string& status)> cb);

    // Cancel the running test early
    void cancel();

    // Check if the test is cancelled
    bool isCancelled() const;

private:
    std::string ip;
    int port_http;
    int port_https;
    std::atomic<bool> cancelled;
    std::function<void(float, const std::string&)> progress_cb;

    bool limit_2_4ghz;
    std::string device;
    std::string lang;
    std::map<std::string, std::string> translations;

    // Server metadata parsed from /serverinfo
    std::string host_name;
    std::string server_version;
    std::string server_state;

    // Helper to get translated string with fallback
    std::string getTranslation(const std::string& key, const std::string& fallback);

    // Internal execution functions
    bool runPingTest(float& min_ping, float& max_ping, float& avg_ping, float& jitter, float& loss_pct);
    bool runSpeedTest(float& speed_mbps);
    void updateProgress(float progress, const std::string& status);

    // Libcurl helper callbacks
    static size_t discardWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t speedWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t stringWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
};

} // namespace moonbeam

