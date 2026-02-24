#include "speaker_trig.h"


speakerTrig::speakerTrig(int speakerPin, int radarPin){
  _speaker_pin = speakerPin;
  _radar_pin = radarPin;
}

void speakerTrig:init(_speaker_pin, _radar_pin) {
  pinMode(_speaker_pin, OUTPUT);
  pinMode(_radar_pin, INPUT);

}

void speakerTrig::usirBurung(){
  int radar_threshold;
  bool radar_status = digitalRead(_radar_pin)
  if (
}
