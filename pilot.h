#pragma once
#include "scene.h"
namespace abyss {
struct PilotInput { float forward=0,strafe=0,rise=0,yaw=0,pitch=0;bool boost=false; };
class Pilot {
 public:
  bool manual=false;
  Camera camera;
  void enter(const Camera& view);
  void stop() { velocity_={}; }
  void update(const Scene& scene,const PilotInput& input,float seconds);
 private:
  Vec3 velocity_{};float yaw_=0,pitch_=0;
};
}
