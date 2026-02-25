#include <Arduino.h>
#include <Servo.h>
#include "radar.h"  


#define MICROWAVE_SENSOR_PIN  D2   
#define BUZZER_PIN            D8   
#define SERVO_PIN             D4   


#define RADAR_THRESHOLD   40    
#define RADAR_ALPHA       0.1f  

// PARAMETER SERVO (Gerakan Radar)
#define SERVO_MIN_ANGLE   0     
#define SERVO_MAX_ANGLE   180   
#define SERVO_STEP        2     
#define SERVO_INTERVAL_MS 15    

// PARAMETER BUZZER ULTRASONIK
// Frekuensi tidak terdengar manusia, mengganggu burung
#define BUZZER_FREQ_HZ        25000   
#define BUZZER_DURATION_MS    3000    
#define BUZZER_COOLDOWN_MS    5000    


Radar radar(MICROWAVE_SENSOR_PIN, RADAR_THRESHOLD, RADAR_ALPHA);
Servo radarServo;

// State servo
int   servoAngle        = 0;
int   servoDirection    = 1;   
unsigned long lastServoMove = 0;

// State buzzer
bool  buzzerActive           = false;
unsigned long lastBuzzerTime = 0;

// Statistik
int totalDetections = 0;


void activateUltrasonicBuzzer(unsigned long durationMs) {
    const unsigned int halfPeriodUs = 1000000UL / BUZZER_FREQ_HZ / 2; 
    unsigned long startTime = millis();

    Serial.printf("[BUZZER] ON - %dHz selama %lums\n", BUZZER_FREQ_HZ, durationMs);

    while (millis() - startTime < durationMs) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriodUs);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriodUs);

        yield(); 
    }

    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("[BUZZER] OFF");
}


void updateServoSweep() {
    unsigned long now = millis();
    if (now - lastServoMove < SERVO_INTERVAL_MS) return;

    servoAngle += servoDirection * SERVO_STEP;

    if (servoAngle >= SERVO_MAX_ANGLE) {
        servoAngle     = SERVO_MAX_ANGLE;
        servoDirection = -1;
    } else if (servoAngle <= SERVO_MIN_ANGLE) {
        servoAngle     = SERVO_MIN_ANGLE;
        servoDirection = 1;
    }

    radarServo.write(servoAngle);
    lastServoMove = now;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n========================================"));
    Serial.println(F("   SIPETARUNG"));
    Serial.println(F("========================================"));

    // Init radar
    radar.init();
    Serial.printf("[RADAR]  Pin: D2 | Threshold: %d | Alpha: %.2f\n",
                  RADAR_THRESHOLD, RADAR_ALPHA);

    // Init buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.printf("[BUZZER] Pin: D8 | Frekuensi: %dHz\n", BUZZER_FREQ_HZ);
    radarServo.attach(SERVO_PIN);
    radarServo.write(90);
    Serial.printf("[SERVO]  Pin: D4 | Sweep: %d deg - %d deg\n",
                  SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

    delay(1000); 

    Serial.println(F("\n[SISTEM] Siap mendeteksi burung...\n"));
}

void loop() {
    radar.sig_process();
    updateServoSweep();
    unsigned long now = millis();

    if (radar.obj_detected()) {
        totalDetections++;

        Serial.printf("[DETEKSI] BURUNG! Sudut: %3d deg | Strength: %2d | Total: %d\n",
                      servoAngle,
                      radar.getSignalStrength(),
                      totalDetections);

        if (now - lastBuzzerTime >= BUZZER_COOLDOWN_MS) {
            lastBuzzerTime = now;
            buzzerActive   = true;
            activateUltrasonicBuzzer(BUZZER_DURATION_MS);
            buzzerActive = false;
        }

    } else {
        static unsigned long lastStatusLog = 0;
        if (now - lastStatusLog >= 2000) {
            Serial.printf("[STATUS] Aman | Sudut: %3d deg | Strength: %2d\n",
                          servoAngle,
                          radar.getSignalStrength());
            lastStatusLog = now;
        }
    }


    delay(10);
}
