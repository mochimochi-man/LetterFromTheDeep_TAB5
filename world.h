#pragma once
#include "config.h"
#include "math3d.h"
namespace abyss { namespace world {
struct Zone {
  const char* name; Vec3 center; float depth;
  Color top,water,ground,rock;
  float light,visibility;
};
struct Boundary { Vec3 a,b,gate; float gap; };
struct TourNode { Vec3 eye,target; }; // eye.y and target.y = altitude above seabed
struct Reservation { const char* name; Vec3 center; float radius; };
struct Environment {
  Color top,water,ground,rock;
  float light,visibility;
  int zone;
};
Environment environment(float x,float z);
float floor(float x,float z);
// Local coral garden within Ivory Dunes; zero outside its footprint.
float coralGarden(float x,float z);
Camera tour(float time);
float zoneTourTime(int zone);
bool reserved(float x,float z,float margin);
const char* zoneName(int zone);
} }
