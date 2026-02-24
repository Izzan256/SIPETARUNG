#ifndef RADAR_H
#define RADAR_H

#include <Arduino.h>

class Radar {
  public:
    /**
     * @param pin Pin digital sensor
     * @param threshold Nilai akumulasi signal untuk trigger (default: 20)
     * @param alpha Koefisien smoothing (0.0 - 1.0), semakin kecil semakin lambat/halus
     */
    Radar(int pin, int threshold = 20, float alpha = 0.1);
    
    void init();
    
    // Harus dipanggil rutin di loop() untuk memproses signal
    void sig_process();

    // Mengembalikan true jika terdeteksi gerakan yang valid (melewati filter)
    bool obj_detected();

    // Mengembalikan nilai internal counter untuk keperluan debugging/kalibrasi
    int getSignalStrength();

  private:
    int _pin;
    int _threshold;
    float _alpha;
    float _filteredValue;
    bool _isDetected;
};

#endif

