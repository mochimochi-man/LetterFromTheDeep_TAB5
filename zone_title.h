#pragma once
#include "config.h"
#include "math3d.h"
namespace abyss {
// Shared by the area cards and the title card so both sit on the same baseline treatment.
void drawTitleMask(uint16_t* pixels,int width,int height,const uint8_t* mask,
                   int maskWidth,int maskHeight,int strength,int centreY);
class ZoneTitle {
 public:
  int zone=-1;
  float age=6;
  bool update(int next,float seconds);
  void draw(uint16_t* pixels,int width,int height) const;
 private:
  int pending_=-1;
  float stable_=0;
};
}
