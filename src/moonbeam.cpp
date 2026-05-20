#include "moonbeam.hpp"
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

namespace moonbeam {

// Simple, non-dependent JSON line parser for flat key-value files
static std::map<std::string, std::string> parseSimpleJson(const std::string& filepath) {
    std::map<std::string, std::string> res;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return res;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Find key between first set of quotes
        size_t firstQuote = line.find('"');
        if (firstQuote == std::string::npos) continue;
        size_t secondQuote = line.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos) continue;
        
        std::string key = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        
        size_t colon = line.find(':', secondQuote + 1);
        if (colon == std::string::npos) continue;
        
        // Find value between quotes after the colon
        size_t thirdQuote = line.find('"', colon + 1);
        if (thirdQuote == std::string::npos) continue;
        size_t fourthQuote = line.find('"', thirdQuote + 1);
        if (fourthQuote == std::string::npos) continue;
        
        std::string value = line.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);
        res[key] = value;
    }
    return res;
}

// Simple helper to replace all occurrences of a placeholder in a string
static std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

// Simple XML tag extractor
static bool extractTag(const std::string& xml, const char* tag, std::string& out) {
    out.clear();
    const std::string open = std::string("<") + tag + ">";
    const std::string close = std::string("</") + tag + ">";
    size_t p1 = xml.find(open);
    if (p1 == std::string::npos) return false;
    p1 += open.size();
    size_t p2 = xml.find(close, p1);
    if (p2 == std::string::npos || p2 < p1) return false;
    out.assign(xml.data() + p1, p2 - p1);
    return true;
}

ConnectionTester::ConnectionTester(const std::string& host_ip, bool limit_to_2_4ghz, const std::string& device_name, const std::string& lang_code, int http_port, int https_port)
    : ip(host_ip), port_http(http_port), port_https(https_port), cancelled(false), progress_cb(nullptr),
      limit_2_4ghz(limit_to_2_4ghz), device(device_name) {
    if (lang_code.rfind("es", 0) == 0) {
        lang = "es";
    } else {
        lang = "en";
    }
}

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
    size_t* total_bytes = static_cast<size_t*>(userp);
    if (total_bytes) {
        *total_bytes += total_size;
    }
    return total_size;
}

size_t ConnectionTester::stringWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    if (str) {
        str->append(static_cast<const char*>(contents), total_size);
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

std::string ConnectionTester::getTranslation(const std::string& key, const std::string& fallback) {
    auto it = translations.find(key);
    if (it != translations.end()) {
        return it->second;
    }
    return fallback;
}

bool ConnectionTester::runPingTest(float& min_ping, float& max_ping, float& avg_ping, float& jitter, float& loss_pct) {
    constexpr int total_pings = 150;
    std::vector<float> pings;
    int failed_count = 0;
    bool is_es = (lang == "es");

    std::string url = "http://" + ip + ":" + std::to_string(port_http) + "/serverinfo";

    std::string xmlResponse;
    bool info_parsed = false;

    for (int i = 0; i < total_pings; ++i) {
        if (cancelled) return false;

        std::string statusMsg = is_es ? 
            "Midiendo latencia y jitter (Ping " + std::to_string(i + 1) + "/" + std::to_string(total_pings) + ")..." : 
            "Measuring latency & jitter (Ping " + std::to_string(i + 1) + "/" + std::to_string(total_pings) + ")...";
        updateProgress((float)i / total_pings * 0.33f, statusMsg);

        CURL* curl = curl_easy_init();
        if (!curl) {
            failed_count++;
            continue;
        }

        // Configure curl request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        if (!info_parsed) {
            xmlResponse.clear();
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xmlResponse);
        } else {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardWriteCallback);
        }
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

            if (!info_parsed && !xmlResponse.empty()) {
                extractTag(xmlResponse, "hostname", host_name);
                extractTag(xmlResponse, "appversion", server_version);
                extractTag(xmlResponse, "state", server_state);
                info_parsed = true;
            }
        } else {
            if (res != CURLE_ABORTED_BY_CALLBACK) {
                failed_count++;
            } else {
                return false; // User cancelled
            }
        }

        // Small delay between pings to get dynamic measurements
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
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
    bool is_es = (lang == "es");

    size_t total_bytes = 0;
    int iterations = 0;
    auto test_start = std::chrono::high_resolution_clock::now();
    double elapsed_seconds = 0.0;
    constexpr double target_duration = 15.0; // 15 seconds speed test

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

    while (elapsed_seconds < target_duration) {
        if (cancelled) {
            curl_easy_cleanup(curl);
            return false;
        }

        std::string statusMsg = is_es ? "Midiendo ancho de banda de descarga..." : "Measuring download bandwidth...";
        updateProgress(0.33f + (float)(elapsed_seconds / target_duration) * 0.62f, statusMsg);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            if (res == CURLE_ABORTED_BY_CALLBACK) {
                curl_easy_cleanup(curl);
                return false; // User cancelled
            }
            // If the server doesn't respond or file is missing, we fail the test
            if (iterations == 0) {
                curl_easy_cleanup(curl);
                return false;
            }
            break; // If subsequent requests fail, break and calculate with what we have
        }

        iterations++;
        auto now = std::chrono::high_resolution_clock::now();
        elapsed_seconds = std::chrono::duration<float>(now - test_start).count();
    }

    curl_easy_cleanup(curl);

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

    // Load translations using normalized lang ("es" or "en")
    std::string path1 = "resources/moonbeam/lang/" + lang + ".json";
    translations = parseSimpleJson(path1);
    if (translations.empty()) {
        std::string path2 = "third_party/moonbeam/lang/" + lang + ".json";
        translations = parseSimpleJson(path2);
    }
    
    // Built-in fallback mappings: resolve keys using loaded translations, or fallback to the provided default English string
    bool is_es = (lang == "es");
    auto get_fallback = [this](const std::string& key, const std::string& fallback_val) -> std::string {
        auto it = translations.find(key);
        if (it != translations.end()) {
            return it->second;
        }
        return fallback_val;
    };

    updateProgress(0.0f, is_es ? "Inicializando prueba de conexión..." : "Initializing Connection Test...");

    // 1. Run Ping Test
    if (!runPingTest(result.min_ping_ms, result.max_ping_ms, result.avg_ping_ms, result.jitter_ms, result.packet_loss_pct)) {
        if (cancelled) {
            result.error_message = is_es ? "Prueba cancelada por el usuario." : "Test cancelled by user.";
            return result;
        }
        result.error_message = is_es ? "Error de comunicación con el host. Verifica la IP." : "Failed to communicate with host. Verify host IP.";
        return result;
    }

    // 2. Run Speed Test
    if (!runSpeedTest(result.speed_mbps)) {
        if (cancelled) {
            result.error_message = is_es ? "Prueba cancelada por el usuario." : "Test cancelled by user.";
            return result;
        }
        result.error_message = is_es ? "Error al descargar recursos de velocidad. Verifica el servidor HTTPS." : "Failed to download speed assets. Verify host HTTPS server.";
        return result;
    }

    updateProgress(0.95f, is_es ? "Finalizando reporte..." : "Finalizing report...");

    result.success = true;
    result.host_name = host_name;
    result.server_version = server_version;
    result.server_state = server_state;

    // 3. Diagnose and Recommend
    std::string rec;

    if (result.packet_loss_pct > 2.0f || result.avg_ping_ms > 50.0f || result.speed_mbps < 3.0f) {
        result.rating = get_fallback("rating_poor", "Poor");
        rec = limit_2_4ghz ? get_fallback("rec_poor_2.4ghz", "Poor 2.4GHz") : get_fallback("rec_poor_5ghz", "Poor 5GHz");
    } else {
        // Evaluate based on bandwidth ranges
        if (result.speed_mbps < 5.0f) {
            result.rating = get_fallback("rating_fair", "Fair");
            rec = get_fallback("rec_fair_low", "Fair low");
        } else if (result.speed_mbps < 10.0f) {
            result.rating = get_fallback("rating_fair", "Fair");
            rec = get_fallback("rec_fair_mid", "Fair mid");
        } else if (result.speed_mbps < 15.0f) {
            result.rating = get_fallback("rating_good", "Good");
            rec = get_fallback("rec_good_low", "Good low");
        } else if (result.speed_mbps < 30.0f) {
            result.rating = get_fallback("rating_good", "Good");
            rec = get_fallback("rec_good_mid", "Good mid");
        } else {
            if (result.avg_ping_ms <= 12.0f && result.jitter_ms <= 2.0f && result.packet_loss_pct <= 0.1f) {
                result.rating = get_fallback("rating_excellent", "Excellent");
                rec = get_fallback("rec_excellent", "Excellent");
            } else {
                result.rating = get_fallback("rating_good", "Good");
                rec = get_fallback("rec_good_high", "Good high");
            }
        }

        // Append warnings for jitter/packet loss if not classified as Poor
        if (result.jitter_ms > 5.0f) {
            rec += " " + get_fallback("rec_high_jitter", "High Jitter");
        }
        if (result.packet_loss_pct > 0.5f) {
            rec += " " + get_fallback("rec_packet_loss", "Packet Loss");
        }
    }

    // Dynamic placeholder replacement for {device} name
    rec = replaceAll(rec, "{device}", device);

    result.recommendation = rec;

    updateProgress(1.0f, is_es ? "Completado!" : "Completed!");
    return result;
}

} // namespace moonbeam
