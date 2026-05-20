#include <map>
#include <string>

namespace moonbeam {

std::map<std::string, std::string> get_lang_es() {
    return {
        {"rating_excellent", "Excelente"},
        {"rating_good", "Bueno"},
        {"rating_fair", "Regular"},
        {"rating_poor", "Deficiente"},
        {"rec_excellent", "¡Conexión excelente! Soporta hasta 1080p 60 FPS a calidad máxima (bitrate: 30+ Mbps)."},
        {"rec_good_high", "¡Buena conexión! Soporta 1080p 60 FPS. Bitrate recomendado: 15-20 Mbps."},
        {"rec_good_mid", "¡Buena conexión! Soporta 720p 60 FPS o 1080p 30 FPS. Bitrate recomendado: 10-15 Mbps."},
        {"rec_good_low", "Conexión decente. Soporta 720p 60 FPS o 1080p 30 FPS. Bitrate recomendado: 8-10 Mbps."},
        {"rec_fair_mid", "Conexión regular. Soporta 720p 30 FPS o 544p 60 FPS. Bitrate recomendado: 5-8 Mbps."},
        {"rec_fair_low", "Conexión limitada. Soporta 544p 30 FPS o 360p 60 FPS. Bitrate recomendado: 3-5 Mbps."},
        {"rec_poor_2.4ghz", "Conexión deficiente. Nota: La {device} solo soporta Wi-Fi de 2.4GHz (velocidad máxima baja y propensa a interferencias). Configura la resolución a 360p 30 FPS (bitrate < 3 Mbps). Desactiva el Bluetooth para reducir interferencias y acércate al router."},
        {"rec_poor_5ghz", "Conexión deficiente. Se recomienda conectar a una red Wi-Fi de 5GHz o usar cable Ethernet. Ajusta la resolución a 360p 30 FPS (bitrate < 3 Mbps)."},
        {"rec_high_jitter", "Se detectaron picos altos de jitter/latencia. Esto causará tirones constantes. Intenta acercarte al router o reducir la congestión en tu red."},
        {"rec_packet_loss", "Pérdida de paquetes detectada. Esto causará cortes de audio y pixelaciones. Reinicia el router o revisa posibles interferencias."},
        {"video_test_title", "Diagnóstico de Bitrate de Video"},
        {"video_test_confirm_msg", "Esta prueba evaluará la estabilidad de tu red en 5 tasas de bits diferentes (2000, 4000, 6000, 8000 y 10000 Kbps).\n\nCada bitrate se probará durante 10 segundos. La prueba completa tomará aproximadamente 50 segundos.\n\n¿Deseas comenzar?"},
        {"video_test_testing", "Probando {bitrate} Kbps... ({sec}s)"},
        {"video_test_cancelling", "Cancelando..."},
        {"video_test_cancel", "Cancelar"},
        {"video_test_report_title", "Reporte de Bitrate de Video"},
        {"video_test_stable", "Estable"},
        {"video_test_unstable", "Inestable"},
        {"video_test_achieved", "Alcanzado: {speed} Kbps"},
        {"video_test_rec_stable", "Recomendamos configurar tu bitrate de streaming a {speed} Kbps para una calidad óptima."},
        {"video_test_rec_unstable", "La conexión es inestable. Recomendamos usar una resolución más baja o mejorar la señal de Wi-Fi."}
    };
}

} // namespace moonbeam
