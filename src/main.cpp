#include <Arduino.h>
#include <Wire.h>
#include <RtcDS3231.h>          // Makuna/Rtc
#include <DFRobotDFPlayerMini.h>
#include "radar.h"

// ─── PIN CONFIGURATION ────────────────────────────────────────────────────────
// RTC DS3231  : SDA = D1 (GPIO5), SCL = D2 (GPIO4) — default I2C ESP8266
// DFPlayer    : Serial1 TX = GPIO2 (D4)
// Radar       : D3 (GPIO0) — bebas dari jalur I2C
#define MICROWAVE_SENSOR_PIN  D3

// ─── PARAMETER RADAR ──────────────────────────────────────────────────────────
#define RADAR_THRESHOLD       40
#define RADAR_ALPHA           0.1f

// ─── PARAMETER MP3 ────────────────────────────────────────────────────────────
#define MP3_VOLUME            25
#define MP3_TRACK_DETECTION   1
#define MP3_COOLDOWN_MS       5000UL

// ─── JAM OPERASIONAL ──────────────────────────────────────────────────────────
#define OPERATIONAL_HOUR_START  6    // 06:00
#define OPERATIONAL_HOUR_END   17    // 17:00

// ─── TIMING ───────────────────────────────────────────────────────────────────
#define RADAR_SAMPLE_INTERVAL_MS   20UL
#define STATUS_LOG_INTERVAL_MS   2000UL
#define RTC_CHECK_INTERVAL_MS   60000UL

// Deep sleep 10 menit (dalam mikrodetik). Max ESP8266 ~71 menit.
#define SLEEP_DURATION_US   (10UL * 60UL * 1000000UL)


// ─── OBJEK GLOBAL ─────────────────────────────────────────────────────────────
Radar                    radar(MICROWAVE_SENSOR_PIN, RADAR_THRESHOLD, RADAR_ALPHA);
DFRobotDFPlayerMini      mp3Player;
RtcDS3231<TwoWire>       rtc(Wire);   // Makuna: template dengan TwoWire

// ─── STATE ────────────────────────────────────────────────────────────────────
static unsigned long lastRadarSample = 0;
static unsigned long lastMp3Time     = 0;
static unsigned long lastStatusLog   = 0;
static unsigned long lastRtcCheck    = 0;
static uint16_t      totalDetections = 0;


// ─── HELPER: Cek jam operasional ─────────────────────────────────────────────
bool isOperationalHour() {
    RtcDateTime now = rtc.GetDateTime();
    uint8_t h = now.Hour();    // Makuna: .Hour() bukan .hour()
    return (h >= OPERATIONAL_HOUR_START && h < OPERATIONAL_HOUR_END);
}

// ─── HELPER: Log waktu RTC ke Serial ─────────────────────────────────────────
void printRtcTime() {
    RtcDateTime now = rtc.GetDateTime();
    Serial.print(F("[RTC]    Waktu: "));
    Serial.print(now.Hour());   Serial.print(':');
    if (now.Minute() < 10) Serial.print('0');
    Serial.print(now.Minute()); Serial.print(':');
    if (now.Second() < 10) Serial.print('0');
    Serial.println(now.Second());
}

// ─── HELPER: Deep sleep ───────────────────────────────────────────────────────
// CATATAN: D0 (GPIO16) harus disambung ke RST untuk auto wake-up
void enterDeepSleep() {
    Serial.print(F("[RTC]    Di luar jam operasional — deep sleep 10 menit..."));
    Serial.flush();
    ESP.deepSleep(SLEEP_DURATION_US);
}


// ─── SETUP ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);    // UART1 → DFPlayer TX (D4/GPIO2)
    Wire.begin();           // I2C → RTC DS3231

    delay(300);

    Serial.println(F("\n========================================"));
    Serial.println(F("   SIPETARUNG - OPTIMIZED"));
    Serial.println(F("========================================"));

    // ── Init RTC (Makuna) ──
    rtc.Begin();

    if (rtc.LastError() != 0) {
        Serial.print(F("[RTC]  ERROR: Wire error code "));
        Serial.println(rtc.LastError());
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(500);
            yield();
        }
    }

    // Jika RTC kehilangan daya / belum pernah di-set, fallback ke waktu kompilasi
    if (!rtc.IsDateTimeValid()) {
        Serial.println(F("[RTC]    Waktu tidak valid, set ke waktu kompilasi."));
        rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    }

    // Pastikan oscillator berjalan
    if (!rtc.GetIsRunning()) {
        Serial.println(F("[RTC]    Oscillator berhenti, memulai ulang..."));
        rtc.SetIsRunning(true);
    }

    printRtcTime();

    // ── Cek jam operasional sebelum init peripheral lain ──
    if (!isOperationalHour()) {
        enterDeepSleep();
        return;
    }

    // ── Init radar ──
    radar.init();
    Serial.print(F("[RADAR]  Threshold: "));
    Serial.print(RADAR_THRESHOLD);
    Serial.print(F(" | Alpha: "));
    Serial.println(RADAR_ALPHA, 2);

    // ── Init DFPlayer ──
    if (!mp3Player.begin(Serial1)) {
        Serial.println(F("[MP3]  ERROR: DFPlayer tidak ditemukan!"));
        pinMode(LED_BUILTIN, OUTPUT);
        for (uint8_t i = 0; i < 10; i++) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(200);
        }
    } else {
        mp3Player.volume(MP3_VOLUME);
        Serial.print(F("[MP3]    Volume: "));
        Serial.println(MP3_VOLUME);
    }

    Serial.println(F("\n[SISTEM] Siap mendeteksi burung...\n"));
}


// ─── LOOP ─────────────────────────────────────────────────────────────────────
void loop() {
    const unsigned long now = millis();

    // 1. Cek jam operasional setiap RTC_CHECK_INTERVAL_MS
    if (now - lastRtcCheck >= RTC_CHECK_INTERVAL_MS) {
        lastRtcCheck = now;
        if (!isOperationalHour()) {
            enterDeepSleep();
            return;
        }
    }

    // 2. Proses radar setiap RADAR_SAMPLE_INTERVAL_MS
    if (now - lastRadarSample >= RADAR_SAMPLE_INTERVAL_MS) {
        lastRadarSample = now;
        radar.sig_process();

        if (radar.obj_detected()) {
            totalDetections++;
            Serial.print(F("[DETEKSI] BURUNG! Strength: "));
            Serial.print(radar.getSignalStrength());
            Serial.print(F(" | Total: "));
            Serial.println(totalDetections);

            if (now - lastMp3Time >= MP3_COOLDOWN_MS) {
                lastMp3Time = now;
                mp3Player.play(MP3_TRACK_DETECTION);
                Serial.print(F("[MP3]    Memutar track "));
                Serial.println(MP3_TRACK_DETECTION);
            }
        } else {
            if (now - lastStatusLog >= STATUS_LOG_INTERVAL_MS) {
                lastStatusLog = now;
                Serial.print(F("[STATUS] Aman | Strength: "));
                Serial.println(radar.getSignalStrength());
            }
        }
    }

    yield();
}
