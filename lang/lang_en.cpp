#include <map>
#include <string>

namespace moonbeam {

std::map<std::string, std::string> get_lang_en() {
    return {
        {"rating_excellent", "Excellent"},
        {"rating_good", "Good"},
        {"rating_fair", "Fair"},
        {"rating_poor", "Poor"},
        {"rec_excellent", "Excellent connection! Supports up to 1080p 60 FPS at maximum quality (bitrate: 30+ Mbps)."},
        {"rec_good_high", "Good connection! Supports 1080p 60 FPS. Recommended bitrate: 15-20 Mbps."},
        {"rec_good_mid", "Good connection! Supports 720p 60 FPS or 1080p 30 FPS. Recommended bitrate: 10-15 Mbps."},
        {"rec_good_low", "Decent connection. Supports 720p 60 FPS or 1080p 30 FPS. Recommended bitrate: 8-10 Mbps."},
        {"rec_fair_mid", "Fair connection. Supports 720p 30 FPS or 544p 60 FPS. Recommended bitrate: 5-8 Mbps."},
        {"rec_fair_low", "Limited connection. Supports 544p 30 FPS or 360p 60 FPS. Recommended bitrate: 3-5 Mbps."},
        {"rec_poor_2.4ghz", "Poor connection. Note: The {device} only supports 2.4GHz Wi-Fi (max link speed is low and prone to interference). Keep resolution at 360p 30 FPS (bitrate < 3 Mbps). Try disabling Bluetooth to reduce interference and move closer to the router."},
        {"rec_poor_5ghz", "Poor connection. Recommended to switch to a 5GHz Wi-Fi network or use an Ethernet cable. Keep resolution at 360p 30 FPS (bitrate < 3 Mbps)."},
        {"rec_high_jitter", "High jitter/latency spikes detected. This will cause frequent stuttering. Try moving closer to the router or reducing network congestion."},
        {"rec_packet_loss", "Packet loss detected. This will cause audio cuts and visual blockiness. Restart your router or check for signal interference."},
        {"video_test_title", "Video Bitrate Diagnostic"},
        {"video_test_confirm_msg", "This test will evaluate your network stability across 5 different bitrates (2000, 4000, 6000, 8000, and 10000 Kbps).\n\nEach bitrate will be tested for 10 seconds. The entire test will take approximately 50 seconds.\n\nDo you want to start?"},
        {"video_test_testing", "Testing {bitrate} Kbps... ({sec}s)"},
        {"video_test_cancelling", "Cancelling..."},
        {"video_test_cancel", "Cancel"},
        {"video_test_report_title", "Video Bitrate Report"},
        {"video_test_stable", "Stable"},
        {"video_test_unstable", "Unstable"},
        {"video_test_achieved", "Achieved: {speed} Kbps"},
        {"video_test_rec_stable", "We recommend setting your streaming bitrate to {speed} Kbps for optimal quality."},
        {"video_test_rec_unstable", "Connection is unstable. We recommend using a lower resolution or improving your Wi-Fi signal."}
    };
}

} // namespace moonbeam
