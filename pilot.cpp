#include "pilot.h"
namespace abyss {
namespace {
Vec3 closestSegment(Vec3 p,Vec3 a,Vec3 b) {
 Vec3 d=b-a;return a+d*clampf(dot(p-a,d)/std::max(.000001f,dot(d,d)),0,1);
}
float distanceSquared(Vec3 p,const Triangle& t) {
 Vec3 ab=t.b-t.a,ac=t.c-t.a,n=cross(ab,ac);float nn=dot(n,n);
 if(nn>.000001f) {
  float h=dot(p-t.a,n);Vec3 q=p-n*(h/nn),v=q-t.a;
  float aa=dot(ab,ab),bb=dot(ab,ac),cc=dot(ac,ac),d=aa*cc-bb*bb;
  if(d>.000001f) {float u=(cc*dot(v,ab)-bb*dot(v,ac))/d,w=(aa*dot(v,ac)-bb*dot(v,ab))/d;
   if(u>=0 && w>=0 && u+w<=1) return h*h/nn;}
 }
 float best=1e20f;
 for(int i=0;i<3;++i) {Vec3 v[]={t.a,t.b,t.c};Vec3 d=p-closestSegment(p,v[i],v[(i+1)%3]);best=std::min(best,dot(d,d));}
 return best;
}
}
bool Scene::clearHull(Vec3 p,float radius) const {
 if(city && p.y>-110-radius) return false;
 if(p.x<WorldMin+radius || p.z<WorldMin+radius || p.x>WorldMin+WorldSize-radius || p.z>WorldMin+WorldSize-radius || p.y>-radius) return false;
 auto blocks=[&](int i) {
  if(i>=solidCount) return false;
  const auto& t=triangles[i];
  if(p.x+radius<std::min(t.a.x,std::min(t.b.x,t.c.x)) || p.x-radius>std::max(t.a.x,std::max(t.b.x,t.c.x)) ||
     p.y+radius<std::min(t.a.y,std::min(t.b.y,t.c.y)) || p.y-radius>std::max(t.a.y,std::max(t.b.y,t.c.y)) ||
     p.z+radius<std::min(t.a.z,std::min(t.b.z,t.c.z)) || p.z-radius>std::max(t.a.z,std::max(t.b.z,t.c.z))) return false;
  return distanceSquared(p,t)<radius*radius;
 };
 if(!staticIndices) {for(int i=0;i<solidCount;++i) if(blocks(i)) return false;return true;}
 if(p.y<surface(p.x,p.z)+radius) return false;
 for(const auto& chunk:chunks) {
  if(p.x+radius<chunk.minimum.x || p.x-radius>chunk.maximum.x || p.y+radius<chunk.minimum.y || p.y-radius>chunk.maximum.y || p.z+radius<chunk.minimum.z || p.z-radius>chunk.maximum.z) continue;
  for(int j=0;j<chunk.count;++j) if(blocks(staticIndices[chunk.start+j])) return false;
 }
 return true;
}
void Pilot::enter(const Camera& view) {
 camera=view;velocity_={};yaw_=std::atan2(view.forward.x,view.forward.z);
 pitch_=std::asin(clampf(view.forward.y,-1,1));manual=true;
}
// Where the boat can get to from where it is buried.
//
// A saved position is a point in a world that can be rebuilt under it: the undersea city
// has been reshaped more than once, and a spot that was open water over one terrace can
// end up inside the next one up. Every axis is then blocked, so the boat turns on the
// spot and goes nowhere, which looks exactly like the controls having died. Straight up
// is the way out of a floor; a ring of directions handles being inside a wall or a tower.
static bool Rescue(const Scene& scene,Vec3 from,float radius,Vec3& out) {
 for(float lift=1;lift<=130;lift+=1) {
  const Vec3 up=from+Vec3{0,lift,0};
  if(scene.clearHull(up,radius)) { out=up; return true; }
 }
 for(float reach=2;reach<=48;reach+=2) for(int i=0;i<8;++i) {
  const float a=i*Pi/4;
  for(float lift:{0.f,6.f,-6.f}) {
   const Vec3 aside=from+Vec3{std::sin(a)*reach,lift,std::cos(a)*reach};
   if(scene.clearHull(aside,radius)) { out=aside; return true; }
  }
 }
 return false;
}
void Pilot::update(const Scene& scene,const PilotInput& input,float seconds) {
 if(!manual || !std::isfinite(seconds)) return;
 float dt=clampf(seconds,0,.2f);
 // Buried, by a world that changed shape since this position was written down. Ease out
 // of the rock rather than sit in it; control comes back the moment the hull is clear.
 if(!scene.clearHull(camera.position,.7f)) {
  Vec3 open;
  if(!Rescue(scene,camera.position,.7f,open)) {
   // Nothing within reach is open water. Rather than leave the boat sealed in, put it
   // back where this region begins, which is somewhere the game itself can always stand.
   enter(scene.city?scene.cityTour(0):scene.tour(0));
   return;
  }
  const Vec3 away=open-camera.position;const float far=length(away);
  const Vec3 eye=camera.position+away*(std::min(far,8.f*dt)/std::max(.0001f,far));
  camera.lookAt(eye,eye+camera.forward);    // the same heading, a little further out
  velocity_={};
  return;
 }
 yaw_+=clampf(input.yaw,-1,1)*1.05f*dt;
 yaw_=std::remainder(yaw_,2*Pi);
 pitch_=clampf(pitch_+clampf(input.pitch,-1,1)*.75f*dt,-1.1f,1.1f);
 Vec3 forward{std::sin(yaw_)*std::cos(pitch_),std::sin(pitch_),std::cos(yaw_)};
 Vec3 right{std::cos(yaw_),0,-std::sin(yaw_)};
 Vec3 desired=forward*clampf(input.forward,-1,1)+right*clampf(input.strafe,-1,1)+Vec3{0,clampf(input.rise,-1,1),0};
 float size=length(desired);if(size>1) desired=desired*(1/size);
 // Cruising speed is what the boost used to be; the boost climbs from there.
 desired=desired*(input.boost?10.f:6.f);
 Vec3 change=desired-velocity_;float delta=length(change),limit=10*dt;
 velocity_=velocity_+change*(delta>limit?limit/delta:1);
 Vec3 travel=velocity_*dt;int steps=std::max(1,int(std::ceil(length(travel)/.25f)));Vec3 step=travel*(1.f/steps),p=camera.position;
 for(int i=0;i<steps;++i) {
  if(scene.clearHull(p+step,.7f)) {p=p+step;continue;}
  // Independent axes allow sliding along a wall while retaining a finite hull radius.
  Vec3 axes[]={{step.x,0,0},{0,step.y,0},{0,0,step.z}};
  for(int axis=0;axis<3;++axis) {
   if(scene.clearHull(p+axes[axis],.7f)) p=p+axes[axis];
   else {if(axis==0) velocity_.x=0;if(axis==1) velocity_.y=0;if(axis==2) velocity_.z=0;}
  }
 }
 camera.lookAt(p,p+forward);
}
}
