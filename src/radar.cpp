#include "radar.h"

Radar::Radar(int pin, int threshold, float alpha) 
    : _pin(pin), _threshold(threshold), _alpha(alpha){
    _filteredValue = 0.0;
    _isDetected = false;

}

void Radar::init(){
    pinMode(_pin, INPUT);
}

void Radar::sig_process(){
    // Mengambil sinyal mentah dari sensor
    float raw_sig = digitalRead(_pin) ? 100.0 : 0.0;

    // Filter menggunakan algoritma Exponential Moving Average (EMA)
    // Rumus: S_t = a * Y_t + (1 - a) * S_{t-1}
    _filteredValue = (_alpha * raw_sig) + ((1.0 - _alpha) * _filteredValue);

    if(_filteredValue >= (float)_threshold) {
        _isDetected = true;

    } else if (_filteredValue < ((float)_threshold * 0.5)){
        _isDetected = false;

    }
}

bool Radar::obj_detected(){
    return _isDetected;
}

int Radar::getSignalStrength(){
    return (int)_filteredValue;
}