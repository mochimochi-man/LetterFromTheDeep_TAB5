#include "finale.h"
namespace abyss {
constexpr float AnnounceSeconds=4.5f;
void Finale::restore(bool complete,bool opened) {phase=complete?(opened?Phase::Open:Phase::Waiting):Phase::Locked;clock=0;cooldown=0;}
Finale::Event Finale::update(bool complete,bool inCity,const Camera& c,float dt,bool manual) {
 if(!std::isfinite(dt)) return Event::None;
 dt=clampf(dt,0,.2f);cooldown=std::max(0.f,cooldown-dt);
 if(!complete) {phase=Phase::Locked;clock=0;return Event::None;}
 // The pilot can be in the city, or out of it, without this having been what took them
 // there. A save loaded inside it comes back as Open, because all restore is told is that
 // the gate was once opened; travelling to it from the list of discoveries changes the
 // region and says nothing to the finale at all. Left alone, the phase then reads Open
 // while the boat is two hundred metres down inside the city - and since everything down
 // there is below the shaft's mouth, crossing the middle of the city passes the test for
 // being at that mouth, the descent starts over, and a quarter of a minute later the
 // pilot is told they have arrived somewhere they already are and put back at the start
 // of it. Where the boat is wins; the phase is corrected to match.
 if(inCity && phase!=Phase::City && phase!=Phase::Dive) { phase=Phase::City; clock=0; }
 else if(!inCity && phase==Phase::City) { phase=Phase::Open; clock=0; cooldown=25; }
 // Finding the last monument is the trigger, wherever the boat happens to be. The
 // gate sinks on its own and the pilot is told about it; nothing takes the controls.
 if(phase==Phase::Locked) { phase=Phase::Announce;clock=0;start=c;return Event::GateOpens; }
 clock+=dt;
 Vec3 gate{-12,-20,4};float distance=length(c.position-gate);
 if(phase==Phase::Waiting) phase=Phase::Announce;
 if(phase==Phase::Announce && clock>=AnnounceSeconds) {phase=Phase::Open;clock=0;cooldown=5;return Event::OpenGate;}
 if(phase==Phase::Open && cooldown<=0) {
  // The open shaft pulls the boat in. Come within reach of the mouth, by hand or on the
  // cruise, and the descent takes the camera from there; nobody has to fly down a hole.
  bool atMouth=std::hypot(c.position.x+12,c.position.z-4)<16 && c.position.y<-5;
  if(atMouth || (!manual && distance<45)) {phase=Phase::Dive;clock=0;start=c;return Event::BeginDive;}
 }
 if(phase==Phase::Dive && clock>=15) {phase=Phase::City;clock=0;return Event::EnterCity;}
 if(phase==Phase::City && manual && std::abs(c.position.x+12)<7 && std::abs(c.position.z+86)<7 && c.position.y>-174) {
  returned();return Event::ReturnSurface;
 }
 return Event::None;
}
// Hardest at the moment it opens, gone by the end of the announcement.
float Finale::shake() const {
 if(phase!=Phase::Announce) return 0;
 return clampf(1-clock/AnnounceSeconds,0,1);
}
Camera Finale::view(const Camera& normal) const {
 if(phase==Phase::Announce) {
  // The boat stays exactly where the pilot left it; only the view is rattled.
  float a=shake()*.16f;
  Camera camera=normal;
  Vec3 jolt{std::sin(clock*47)*a,std::sin(clock*61+1.7f)*a*.8f,std::sin(clock*39+.4f)*a*.5f};
  camera.lookAt(normal.position+jolt,normal.position+jolt+normal.forward);
  return camera;
 }
 if(!cinematic()) return normal;
 Camera camera;
 {
  float t=clampf(clock/15,0,1);
  float approach=clampf(clock/5,0,1);approach=approach*approach*(3-2*approach);
  float dive=clampf((clock-5)/10,0,1);dive=dive*dive*(3-2*dive);
  Vec3 p=clock<5?mix(start.position,{-12,-14,4},approach):mix({-12,-14,4},{-12,-57,4},dive);
  camera.lookAt(p,p+mix(start.forward,{0,-.75f,.66f},t));
 }
 return camera;
}
}
