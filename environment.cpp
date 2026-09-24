#include "scene.h"
namespace abyss {
void Scene::buildVents() {
  ventCount=0;
  const float times[]={350,392,612,648};
  for(int field=0;field<4;++field) {
    Camera c=tour(times[field]); Vec3 center=c.position+c.forward*22+c.right*(field%2?8:-8);
    center.y=surface(center.x,center.z);
    uint32_t seed=100301+field*139;
    rock(center-Vec3{0,.3f,0},{2.7f,.9f,2.3f},seed,1);
    for(int chimney=0;chimney<3;++chimney) {
      Vec3 p=center+Vec3{float(chimney-1)*1.5f,0,std::sin(chimney*2.0f)};
      p.y=surface(p.x,p.z)+.2f;
      float height=2.6f+chimney*1.2f;
      Vec3 previous[6];material_=1;lighting_=.85f;
      for(int level=0;level<3;++level) {
        float y=height*level*.5f,r=(.68f-level*.17f)*(1+.08f*noise(seed+level));
        Vec3 ring[6];
        for(int k=0;k<6;++k) {
          float a=k*Pi/3;ring[k]=p+Vec3{std::cos(a)*r+.12f*level,y,std::sin(a)*r};
          if(level && k) quad(previous[k-1],previous[k],ring[k],ring[k-1],{77,83,72});
        }
        if(level) quad(previous[5],previous[0],ring[0],ring[5],{89,93,78});
        for(int k=0;k<6;++k) previous[k]=ring[k];
      }
      Vec3 mouth=p+Vec3{.24f,height,0};
      material_=0;
      for(int k=0;k<6;++k) add(previous[k],previous[(k+1)%6],mouth-Vec3{0,.35f,0},{23,28,25});
      if(chimney==2) vents[ventCount++]={mouth,seed};
    }
  }
  material_=0;lighting_=1;
}
bool Scene::clearView(Vec3 from,Vec3 to,float allowance) const {
 Vec3 direction=to-from;float distance=length(direction);
 if(distance<.01f) return true;
 float limit=std::max(0.0f,1-allowance/distance);
 Vec3 low{std::min(from.x,to.x),std::min(from.y,to.y),std::min(from.z,to.z)};
 Vec3 high{std::max(from.x,to.x),std::max(from.y,to.y),std::max(from.z,to.z)};
 auto blocks=[&](int index) {
   const auto& t=triangles[index]; Vec3 e1=t.b-t.a,e2=t.c-t.a,h=cross(direction,e2);
   float determinant=dot(e1,h);if(std::abs(determinant)<.000001f) return false;
   float inverse=1/determinant;Vec3 delta=from-t.a;
   float u=dot(delta,h)*inverse;if(u<0 || u>1) return false;
   Vec3 q=cross(delta,e1);float v=dot(direction,q)*inverse;
   if(v<0 || u+v>1) return false;
   float ray=dot(e2,q)*inverse;return ray>.001f && ray<limit;
 };
 if(!staticIndices) {for(int i=0;i<staticCount;++i) if(blocks(i)) return false;return true;}
 for(const auto& chunk:chunks) {
   if(low.x>chunk.maximum.x || high.x<chunk.minimum.x || low.y>chunk.maximum.y || high.y<chunk.minimum.y || low.z>chunk.maximum.z || high.z<chunk.minimum.z) continue;
   for(int j=0;j<chunk.count;++j) if(blocks(staticIndices[chunk.start+j])) return false;
 }
 return true;
}
}
