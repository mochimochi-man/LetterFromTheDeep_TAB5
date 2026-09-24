#include "scene.h"
namespace abyss { namespace ecology {
void Ecosystem::largeGroups(const Scene& scene,const Camera& cam,float time) {
 struct Group {Species species;int zone,population;Vec3 center;float rx,rz,speed;};
 // Exactly one persistent population per kind, independent of local habitat generation.
 static const Group groups[]={
  {Species::Dolphin,0,6,{0,0,-5},25,20,.048f},
  {Species::Manta,1,3,{264,0,-90},17,24,.020f},
  {Species::WhaleShark,1,1,{211,0,-175},25,20,.014f},
  {Species::Hammerhead,4,5,{-238,0,-116},19,28,.028f},
  {Species::WhiteShark,2,2,{202,0,148},26,20,.018f},
  {Species::Orca,5,3,{0,-195,-48},48,17,.024f}
 };
 const int zone=scene.environment(cam.position).zone;
 for(int g=0;g<6;++g) {
  const auto& group=groups[g];if(group.zone!=zone)continue;
  for(int j=0;j<group.population && count<MaxAnimals;++j) {
   float angle=time*group.speed+g*.9f-j*(group.species==Species::WhiteShark?.95f:.18f);
   float lane=(j%3-1)*2.8f;
   Vec3 p=group.center+Vec3{std::cos(angle)*(group.rx+lane),0,std::sin(angle)*(group.rz+lane)};
   if(!scene.city && world::environment(p.x,p.z).zone!=group.zone)continue;
   float body=nominalLength(group.species)*(j==0?1.f:j==group.population-1?.76f:.90f);
   float heading=std::atan2(-(group.rx+lane)*std::sin(angle),(group.rz+lane)*std::cos(angle));
   if(!scene.city) {
    // Sample the whole silhouette against terrain/monuments, not just its centre.
    float bed=scene.surface(p.x,p.z),ceiling=bed;
    for(int k=0;k<8;++k){float a=k*Pi/4;ceiling=std::max(ceiling,scene.surface(p.x+std::cos(a)*body*.6f,p.z+std::sin(a)*body*.6f,true));}
    p.y=std::max(bed+12+std::sin(angle*.8f+j)*1.3f,ceiling+body*.4f+1);
    if(p.y>-body*.35f-1)continue;
   } else p.y+=j*2+std::sin(angle)*1.5f;
   Vec3 view=cam.view(p);
   if(view.z < -body || view.z>78 || std::abs(view.x)>std::max(0.f,view.z)*.75f+body || std::abs(view.y)>std::max(0.f,view.z)*.60f+body)continue;
   float rate=group.species==Species::Dolphin?2.6f:group.species==Species::Manta?.9f:group.species==Species::WhaleShark?.65f:1.25f;
   animals[count++]={p,heading,body,time*rate+j*.8f,1,group.species,uint32_t(50000+g*100+j)};
  }
 }
}
} }
