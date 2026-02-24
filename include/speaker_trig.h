#ifndef SPEAKER_TRIG_H
#define SPEAKER_TRIG_H

#include <Arduino.h>

class speakerTrig {
  public:
    speakerTrig::Trig(int speakerPin, int radarPin);
    void init();
    
  private:
      int _speaker_pin;
      int _radar_pin;

}
#endif
