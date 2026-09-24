#include "scene.h"
#if defined(ARDUINO)
#include <Arduino.h>
#endif
namespace abyss { namespace ecology {
namespace {
float smooth(float t){t=clampf(t,0,1);return t*t*(3-2*t);}
}
float nautilusExtension(float distance) {
 // A continuous spatial response: fully hidden at 1.8 m, fully extended at 5.2 m.
 return smooth((distance-1.8f)/3.4f);
}
float gardenEelExtension(float distance,uint32_t id) {
 // Individual thresholds produce a ripple across a colony, without per-frame state.
 return smooth((distance-(1.7f+.5f*noise(id+19)))/2.6f);
}
Animal shelterCoelacanth(const CoelShelter& shelter,int index,float time) {
 float cycle=std::fmod(std::max(0.f,time)+noise(shelter.seed)*100.f,100.f);
 float travel=smooth((cycle-44)/12)-smooth((cycle-64)/12);
 float turn=Pi*(smooth((cycle-56)/8)+smooth((cycle-76)/8));
 bool awake=cycle>44 && cycle<84;
 float activity=awake?.12f+.75f*std::sin((cycle-44)*Pi/40):.06f;
 Vec3 p=shelter.home+shelter.outward*(travel*3.f);
 p.y+=std::sin(time*.6f+noise(shelter.seed+1)*6)*.012f;
 return {p,std::atan2(shelter.outward.x,shelter.outward.z)+turn,
   nominalLength(Species::Coelacanth)*(.85f+.10f*noise(shelter.seed+2)),
   time*.8f+noise(shelter.seed)*6,activity,Species::Coelacanth,uint32_t(70000+index)};
}
void Ecosystem::buildRelicShelters(const Scene& scene) {
 shelterCount=0;
 // Locate the calm side of real rock geometry, close enough to the primordial tour
 // to discover. Selection happens only at world build, never in the animation loop.
 for(int slot=0;slot<ShelterLimit;++slot) {
  Vec3 anchor=scene.tour(world::zoneTourTime(3)+slot*11.f).position;
  float best=60.f*60.f;CoelShelter chosen{};bool found=false;
  for(int i=12800;i<scene.solidCount;i+=2) {
   const auto& t=scene.triangles[i];
   if((t.material&SolidFace)==0 || (t.material&127)!=1)continue;
   Vec3 middle=(t.a+t.b+t.c)*(1.f/3),normal=unit(cross(t.b-t.a,t.c-t.a));
   if(std::abs(normal.y)>.38f)continue;
   Vec3 delta=middle-anchor;delta.y=0;float score=dot(delta,delta);
   if(score>=best || world::environment(middle.x,middle.z).zone!=3)continue;
   Vec3 outward=unit({normal.x,0,normal.z});Vec3 home=middle+outward*1.8f;
   float above=home.y-scene.surface(home.x,home.z);
   if(above<1.3f || above>5.f || world::reserved(home.x,home.z,1))continue;
   bool spaced=true;for(int j=0;j<shelterCount;++j)if(length(home-shelters[j].home)<18)spaced=false;
   if(!spaced || !scene.clearHull(home,1.05f))continue;
   // A wall behind the fish, and an unobstructed escape into open water, rule out
   // candidates accidentally selected inside a closed rock.
   if(scene.clearView(home,home-outward*3.f,.05f))continue;
   if(!scene.clearView(home,home+outward*7.f+Vec3{0,2,0}))continue;
   bool safe=true;
   for(int j=1;j<=6;++j){Vec3 p=home+outward*(j*.5f);if(!scene.clearHull(p,1.05f) || world::environment(p.x,p.z).zone!=3)safe=false;}
   if(!safe)continue;
   best=score;chosen={home,outward,hash(19001u+uint32_t(i)*31u)};found=true;
  }
  if(found)shelters[shelterCount++]=chosen;
#if defined(ARDUINO)
  delay(1);
#endif
 }
}
void Ecosystem::restingCoelacanths(const Scene& scene,const Camera& cam,float time) {
 if(scene.city || world::environment(cam.position.x,cam.position.z).zone!=3)return;
 for(int i=0;i<shelterCount && count<MaxAnimals;++i) {
  Animal a=shelterCoelacanth(shelters[i],i,time);Vec3 view=cam.view(a.position);
  if(view.z<.6f || view.z>45 || std::abs(view.x)>view.z*.68f+a.length || std::abs(view.y)>view.z*.53f+a.length)continue;
  animals[count++]=a;
 }
}
} }
