#pragma once
#include "math3d.h"
namespace abyss {
class Finale {
 public:
  enum class Phase {Locked,Waiting,Announce,Open,Dive,City};
  enum class Event {None,GateOpens,OpenGate,BeginDive,EnterCity,ReturnSurface};
  Phase phase=Phase::Locked;
  float clock=0,cooldown=0;
  Camera start;
  void restore(bool complete,bool opened);
  // inCity is where the boat actually is. The finale is not always what put it
  // there, so it is told rather than left to assume.
  Event update(bool complete,bool inCity,const Camera& camera,float dt,bool manual);
  Camera view(const Camera& normal) const;
  // Announce is not a cinematic: the pilot keeps the boat, the picture just shakes.
  bool cinematic() const {return phase==Phase::Dive;}
  bool announcing() const {return phase==Phase::Announce;}
  float shake() const;
  void returned() {phase=Phase::Open;clock=0;cooldown=25;}
};
}
