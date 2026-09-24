#if defined(__GNUC__)
#pragma GCC optimize ("O3", "fast-math")
#endif
#include "world_data.h"
#include "scene.h"
#include <limits>
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#ifdef quad
#undef quad
#endif
namespace abyss { namespace world {
namespace {
constexpr int ZoneCount=sizeof(Zones)/sizeof(Zones[0]);
constexpr int NodeCount=sizeof(Route)/sizeof(Route[0]);
void weights(float x,float z,float* w) {
  float distance[ZoneCount],nearest=1e20f;
  for(int i=0;i<ZoneCount;++i) {
    float dx=x-Zones[i].center.x,dz=z-Zones[i].center.z;
    distance[i]=std::sqrt(dx*dx+dz*dz);
    nearest=std::min(nearest,distance[i]);
  }
  float sum=0;
  for(int i=0;i<ZoneCount;++i) {
    float q=std::max(0.0f,1-(distance[i]-nearest)/34);
    w[i]=q*q; sum+=w[i];
  }
  for(int i=0;i<ZoneCount;++i) w[i]/=sum;
}
float height(int zone,float x,float z) {
  float dx=x-Zones[zone].center.x,dz=z-Zones[zone].center.z;
  switch(zone) {
    case 0: {
      float ripples=.6f*std::sin(x*.19f+z*.08f)+.45f*std::sin(z*.23f-x*.11f);
      float ridge=2.8f*std::sin(x*.047f)*std::cos(z*.058f);
      float canyon=12*std::exp(-((x-46)*(x-46)/150+(z-1)*(z-1)/950));
      return -25+ridge+ripples-canyon;
    }
    case 1: return -33+3.1f*std::sin(dx*.056f+dz*.018f)+
                            1.1f*std::sin(dz*.11f+std::sin(dx*.031f));
    case 2: return -48+3.5f*std::sin(dx*.085f)*std::cos(dz*.064f)+
                            2.2f*std::sin((dx+dz)*.13f);
    case 3: {
      float radius=std::sqrt(dx*dx+dz*dz);
      float q=radius/22, f=q-std::floor(q);
      float knee=clampf((f-.70f)/.30f,0,1);
      float terrace=(std::floor(q)+knee*knee*(3-2*knee))*2.2f;
      return -47+std::min(11.0f,terrace)+.4f*std::sin(dx*.08f+dz*.035f);
    }
    default: {
      float trench=dx+14*std::sin(dz*.028f);
      return -72-30*std::exp(-trench*trench/620)+
                  3.2f*std::sin(dz*.055f)+1.3f*std::cos(dx*.073f);
    }
  }
}
float segmentDistance(Vec3 p,Vec3 a,Vec3 b) {
  p.y=a.y=b.y=0; Vec3 v=b-a;
  float t=clampf(dot(p-a,v)/std::max(.001f,dot(v,v)),0,1);
  return length(p-(a+v*t));
}
}
float coralGarden(float x,float z) {
  float dx=(x-220)/43.f,dz=(z+132)/35.f;
  return clampf((1-dx*dx-dz*dz)*3.f,0,1);
}
Environment environment(float x,float z) {
  float w[ZoneCount]; weights(x,z,w);
  float r[4]={},g[4]={},b[4]={},light=0,visibility=0;
  int zone=0;
  for(int i=0;i<ZoneCount;++i) {
    if(w[i]>w[zone]) zone=i;
    Color colors[]={Zones[i].top,Zones[i].water,Zones[i].ground,Zones[i].rock};
    for(int j=0;j<4;++j) { r[j]+=colors[j].r*w[i]; g[j]+=colors[j].g*w[i]; b[j]+=colors[j].b*w[i]; }
    light+=Zones[i].light*w[i]; visibility+=Zones[i].visibility*w[i];
  }
  return {{uint8_t(r[0]),uint8_t(g[0]),uint8_t(b[0])},
          {uint8_t(r[1]),uint8_t(g[1]),uint8_t(b[1])},
          {uint8_t(r[2]),uint8_t(g[2]),uint8_t(b[2])},
          {uint8_t(r[3]),uint8_t(g[3]),uint8_t(b[3])},light,visibility,zone};
}
float floor(float x,float z) {
  float w[ZoneCount]; weights(x,z,w);
  float y=0;
  for(int i=0;i<ZoneCount;++i) if(w[i]>.0001f) y+=height(i,x,z)*w[i];
  return y;
}
Camera tour(float time) {
  float t=std::fmod(std::max(0.0f,time),TourSeconds)*NodeCount/TourSeconds;
  int index=int(t); t-=index;
  const auto& a=Route[(index+NodeCount-1)%NodeCount];
  const auto& b=Route[index];
  const auto& c=Route[(index+1)%NodeCount];
  const auto& d=Route[(index+2)%NodeCount];
  auto lifted=[](Vec3 v) { v.y+=floor(v.x,v.z); return v; };
  Vec3 p=spline(lifted(a.eye),lifted(b.eye),lifted(c.eye),lifted(d.eye),t);
  Vec3 target=spline(a.target,b.target,c.target,d.target,t);
  p.y=std::max(p.y,floor(p.x,p.z)+4.0f)+.10f*std::sin(time*.48f);
  target.y+=floor(target.x,target.z);
  Camera camera; camera.lookAt(p,target); return camera;
}
float zoneTourTime(int zone) {
  constexpr int node[]={0,10,22,31,40};
  return node[std::max(0,std::min(zone,4))]*TourSeconds/NodeCount;
}
bool reserved(float x,float z,float margin) {
  Vec3 p{x,0,z};
  for(const auto& s:Reservations) {
    float dx=x-s.center.x,dz=z-s.center.z;
    if(dx*dx+dz*dz<(s.radius+margin)*(s.radius+margin)) return true;
  }
  for(int i=0;i<NodeCount;++i)
    if(segmentDistance(p,Route[i].eye,Route[(i+1)%NodeCount].eye)<margin+10) return true;
  for(const auto& b:Boundaries)
    if(length(p-b.gate)<b.gap*.5f+margin+7) return true;
  return false;
}
const char* zoneName(int zone) { return Zones[std::max(0,std::min(zone,4))].name; }
} // world
void Scene::buildMap() {
  constexpr int Step=8;
  for(int z=WorldMin;z<WorldMin+WorldSize;z+=Step) {
    for(int x=WorldMin;x<WorldMin+WorldSize;x+=Step) {
      Vec3 a{float(x),floor(x,z),float(z)},b{float(x+Step),floor(x+Step,z),float(z)};
      Vec3 c{float(x+Step),floor(x+Step,z+Step),float(z+Step)},d{float(x),floor(x,z+Step),float(z+Step)};
      auto env=world::environment(x+Step*.5f,z+Step*.5f);
      lighting_=env.light;
      constexpr uint8_t materials[]={2,2,4,6,6};
      material_=materials[env.zone];
      Color color=scale(env.ground,.91f+noise((x-WorldMin)*7919+z-WorldMin)*.15f);
      if(((x-z)/Step)&1) { add(a,b,d,color); add(b,c,d,color); }
      else quad(a,b,c,d,color);
    }
#if defined(ARDUINO)
    delay(1);
#endif
  }
  // Continuous walls with real open passages; no invisible zone barriers.
  uint32_t seed=4300;
  for(const auto& boundary:world::Boundaries) {
    Vec3 direction=unit(boundary.b-boundary.a),normal={direction.z,0,-direction.x};
    float total=length(boundary.b-boundary.a);
    float gate=dot(boundary.gate-boundary.a,direction);
    auto wall=[&](float begin,float end) {
      if(end-begin<2) return;
      int steps=std::max(1,int(std::ceil((end-begin)/10)));
      Vec3 previous[7];
      const float offsets[]={-10,-6,-5,-3,2,6,10};
      const float heights[]={-.4f,1,18,27,26,8,-.4f};
      for(int i=0;i<=steps;++i) {
        Vec3 center=boundary.a+direction*(begin+(end-begin)*i/steps);
        auto env=world::environment(center.x,center.z);
        material_=1; lighting_=env.light;
        float high=1.0f+.18f*std::sin((begin+i*10)*.057f+seed);
        Vec3 current[7];
        for(int j=0;j<7;++j) {
          current[j]=center+normal*(offsets[j]+(noise(seed+i*19+j)-.5f)*1.5f);
          current[j].y=floor(current[j].x,current[j].z)+heights[j]*high;
          if(i && j) quad(previous[j?j-1:0],current[j?j-1:0],current[j],previous[j],env.rock);
        }
        // Close the ends so a gate shows the wall's thickness.
        if(i==0 || i==steps)
          for(int j=1;j<6;++j) add(current[0],current[j],current[j+1],env.rock);
        for(int j=0;j<7;++j) previous[j]=current[j];
      }
    };
    wall(0,std::max(0.0f,gate-boundary.gap*.5f));
    wall(std::min(total,gate+boundary.gap*.5f),total);
    seed+=137;
  }
  // Irregular silhouettes: boulders, broad shelves, strata, needles, overhangs and columns.
  for(int zone=0;zone<5;++zone) {
    int placed=0;
    for(int trial=0;trial<170 && placed<23;++trial) {
      uint32_t seed=16000+zone*1709+trial*23;
      float x=world::Zones[zone].center.x+(noise(seed)-.5f)*220;
      float z=world::Zones[zone].center.z+(noise(seed+1)-.5f)*220;
      if(x<WorldMin+20 || z<WorldMin+20 || x>300 || z>300) continue;
      if(zone==0 && x*x+z*z<75*75) continue; // preserve the established first-basin scenery
      if(world::environment(x,z).zone!=zone || world::reserved(x,z,10)) continue;
      float radius=2.5f+noise(seed+2)*9;
      float high=2+noise(seed+3)*19;
      int style=(placed+zone)%6;
      if(style==1) { radius*=1.4f; high*=.35f; }
      if(style==3) { radius*=.43f; high*=1.4f; }
      rock({x,floor(x,z)-.6f,z},{radius,high,radius*(.6f+noise(seed+4)*.9f)},seed,style);
      ++placed;
    }
  }
  struct Feature { Vec3 position,size; int style; };
  const Feature features[]={
    {{164,0,-196},{17,5,11},1},{{227,0,-118},{14,4,19},1},
    {{290,0,-127},{7,15,8},4},{{152,0,-66},{16,5,9},0},
    {{192,0,132},{6,25,7},5},{{265,0,181},{8,30,6},5},
    {{163,0,169},{12,19,7},2},{{177,0,243},{5,22,5},3},
    {{-165,0,235},{17,8,11},1},{{-95,0,174},{20,6,12},2},
    {{-224,0,182},{9,19,16},4},{{-37,0,251},{13,6,9},1},
    {{-291,0,-100},{6,39,7},3},{{-193,0,-144},{14,22,9},4},
    {{-186,0,-43},{8,29,12},2},{{-280,0,-212},{18,7,13},1}
  };
  int serial=0;
  for(const auto& feature:features) {
    Vec3 position=feature.position;
    position.y=floor(position.x,position.z)-.7f;
    rock(position,feature.size,44001+serial++*17,feature.style);
  }
  lighting_=1;
}

void Scene::seabedLife(Vec3 p,float size,uint32_t seed,int type) {
  material_=0;
  lighting_=world::environment(p.x,p.z).light;
  if(type>=6 && type<=9) {
    lighting_=std::max(1.28f,lighting_);
    reefCoral(p,size,seed,type);
  } else if(type==0) { // Three curved, tapered kelp ribbons: 18 triangles.
    Color color=blend({66,133,54},{131,147,64},noise(seed));
    for(int stem=0;stem<3;++stem) {
      float yaw=noise(seed+stem*19)*Pi*2;
      Vec3 side{std::cos(yaw),0,std::sin(yaw)};
      Vec3 base=p+side*(stem*.24f);
      float height=size*(.7f+.3f*noise(seed+stem+91));
      for(int j=0;j<3;++j) {
        float a=j/3.0f,b=(j+1)/3.0f;
        Vec3 low=base+Vec3{0,height*a,0}+side*(std::sin(a*3+stem)*a*size*.25f);
        Vec3 high=base+Vec3{0,height*b,0}+side*(std::sin(b*3+stem)*b*size*.25f);
        float wa=size*.13f*(1-a*.8f),wb=size*.13f*(1-b*.8f);
        quad(low-side*wa,low+side*wa,high+side*wb,high-side*wb,scale(color,.8f+j*.1f));
      }
    }
  } else if(type==1) { // Layered table corals, three irregular polygon plates.
    Color color=blend({183,108,96},{201,166,92},noise(seed));
    for(int level=0;level<3;++level) {
      Vec3 center=p+Vec3{level*.12f,size*(.2f+level*.23f),0};
      float radius=size*(.7f-level*.15f);
      for(int k=0;k<6;++k) {
        float a=k*Pi/3,b=(k+1)*Pi/3;
        add(center+Vec3{0,size*.1f,0},center+Vec3{std::cos(a)*radius,0,std::sin(a)*radius},
          center+Vec3{std::cos(b)*radius,0,std::sin(b)*radius},scale(color,.85f+level*.1f));
      }
    }
  } else if(type==2) { // Branched sea fans: crossed planes keep their silhouette from any direction.
    Color color=blend({192,107,112},{168,135,192},noise(seed));
    for(int plane=0;plane<2;++plane) {
      Vec3 side=plane?Vec3{0,0,1}:Vec3{1,0,0};
      for(int branch=-1;branch<=1;++branch) {
        Vec3 base=p+Vec3{0,size*.18f,0};
        Vec3 tip=p+Vec3{0,size*(branch==0?1.1f:.85f),0}+side*(branch*size*.55f);
        quad(base-side*(size*.055f),base+side*(size*.055f),tip+side*(size*.06f),tip-side*(size*.06f),color);
        add(tip-side*(size*.22f)-Vec3{0,size*.15f,0},tip+Vec3{0,size*.2f,0},tip+side*(size*.22f)-Vec3{0,size*.15f,0},color);
      }
    }
  } else if(type==4) { // Low, layered stromatolite-like mounds.
    for(int level=0;level<3;++level) for(int k=0;k<6;++k) {
      float a=k*Pi/3,b=(k+1)*Pi/3,r=size*(.65f-level*.15f);
      Vec3 center=p+Vec3{0,size*(.10f+level*.15f),0};
      add(center+Vec3{0,size*.16f,0},center+Vec3{std::cos(a)*r,0,std::sin(a)*r},
        center+Vec3{std::cos(b)*r,0,std::sin(b)*r},{133,151,97});
    }
  } else if(type==5) { // Forest kelp: tall, slender, many segments so it curves.
    Color color=blend({48,112,46},{92,138,58},noise(seed));
    for(int stem=0;stem<2;++stem) {
      float yaw=noise(seed+stem*23)*Pi*2;
      Vec3 side{std::cos(yaw),0,std::sin(yaw)};
      Vec3 base=p+side*(stem*.4f);
      float height=size*(.85f+.3f*noise(seed+stem+57));
      for(int j=0;j<6;++j) {
        float a=j/6.0f,b=(j+1)/6.0f;
        Vec3 low=base+Vec3{0,height*a,0}+side*(std::sin(a*4+stem)*a*height*.10f);
        Vec3 high=base+Vec3{0,height*b,0}+side*(std::sin(b*4+stem)*b*height*.10f);
        float wa=height*.038f*(1-a*.45f),wb=height*.038f*(1-b*.45f);
        quad(low-side*wa,low+side*wa,high+side*wb,high-side*wb,scale(color,.78f+j*.045f));
      }
    }
  } else { // Open cup sponges in the deep basin.
    Color color=blend({149,156,163},{174,135,124},noise(seed));
    for(int k=0;k<6;++k) {
      float a=k*Pi/3,b=(k+1)*Pi/3;
      Vec3 ra{std::cos(a),0,std::sin(a)},rb{std::cos(b),0,std::sin(b)};
      Vec3 top=p+Vec3{0,size,0};
      quad(p+ra*(size*.16f),p+rb*(size*.16f),top+rb*(size*.42f),top+ra*(size*.42f),color);
      add(top+ra*(size*.42f),top+rb*(size*.42f),p+Vec3{0,size*.4f,0},scale(color,.45f));
    }
  }
}
// The kelp forest stands at the back of the ivory dunes, away from the tour route.
static constexpr Vec3 KelpForest{250,0,-232};
void Scene::buildSeabedLife() {
  colonyCount=0;
  auto place=[&](float x,float z,uint32_t seed,int forceType=-1,float sizeScale=1) {
    if(x<WorldMin+8 || z<WorldMin+8 || x>WorldMin+WorldSize-8 || z>WorldMin+WorldSize-8) return;
    if(gateOpen && std::abs(x+12)<24 && std::abs(z-4)<24) return;
    // Anchor roots to the actual triangulated 8m terrain, rather than its smooth source function.
    int cellX=int((x-WorldMin)/8),cellZ=int((z-WorldMin)/8);
    int first=(cellZ*(WorldSize/8)+cellX)*2;
    float y=floor(x,z);
    for(int j=0;j<2;++j) {
      const auto& t=triangles[first+j]; Vec3 a=t.b-t.a,b=t.c-t.a;
      float den=a.x*b.z-a.z*b.x;
      float u=((x-t.a.x)*b.z-(z-t.a.z)*b.x)/den;
      float v=(a.x*(z-t.a.z)-a.z*(x-t.a.x))/den;
      if(u>=-.001f && v>=-.001f && u+v<=1.001f) { y=t.a.y+u*a.y+v*b.y; break; }
    }
    int zone=world::environment(x,z).zone;
    int type=forceType>=0?forceType:
      zone==3?(seed%2?3:4):zone==4?3:zone==1?0:zone==2?(seed%3==0?2:0):int(seed%3);
    float size=type==0?2.4f+noise(seed)*2.3f:1.5f+noise(seed)*1.8f;
    if(zone==1 && forceType<0) size*=.65f;
    size=forceType==5?sizeScale:size*sizeScale;   // forest kelp is given its height directly
    if(colonyCount<ColonyLimit) colonies[colonyCount++]={{x,y,z},size,uint8_t(type)};
    seabedLife({x,y-.08f,z},size,seed,type);
  };
  for(int i=0;i<120;++i) {
    Camera cam=tour(i*TourSeconds/120);
    int zone=world::environment(cam.position.x,cam.position.z).zone;
    int density=zone==0?3:(zone==1 || zone==4)?1:2;
    for(int j=0;j<density;++j) {
      uint32_t seed=54000+i*71+j*13;
      Vec3 p=cam.position+cam.forward*(20+noise(seed)*13)+cam.right*((j%2?-1:1)*(5+noise(seed+1)*9));
      place(p.x,p.z,seed);
    }
#if defined(ARDUINO)
    if(i%8==0) delay(1);
#endif
  }
  // Six irregular coral knolls separated by white sand channels. Reserve metadata
  // slots before the background vegetation so reef fish can find these real colonies.
  const Vec3 gardens[]={{207,0,-145},{232,0,-149},{244,0,-132},{232,0,-115},{209,0,-114},{194,0,-130}};
  for(int group=0;group<6;++group)for(int j=0;j<20;++j) {
    uint32_t seed=123001+group*277+j*31;
    float angle=j*2.399963f+noise(seed)*.6f,r=std::sqrt((j+.5f)/20)*9.0f;
    float x=gardens[group].x+std::cos(angle)*r,z=gardens[group].z+std::sin(angle)*r;
    if(world::environment(x,z).zone!=1 || world::reserved(x,z,2.5f))continue;
    int type=6+(j+group)%2;
    // Mixed sizes avoid the look of repeated identical props.
    float scale=.48f+noise(seed+3)*.48f;
    if(j==0)scale=1.30f;
    place(x,z,seed,type,scale);
#if defined(ARDUINO)
    if(j==19)delay(1);
#endif
  }
  // Scattered background colonies complement the denser route-side patches. Hand flying
  // leaves the tour, so the open basins need their own cover.
  for(int i=0;i<240;++i) {
    uint32_t seed=77000+i*29;
    float x=WorldMin+20+noise(seed)*(WorldSize-40),z=WorldMin+20+noise(seed+1)*(WorldSize-40);
    if(!world::reserved(x,z,2)) place(x,z,seed);
#if defined(ARDUINO)
    if(i%24==0) delay(1);
#endif
  }
  // A kelp forest at the back of the ivory dunes: tall stands, standing close together.
  for(int i=0;i<110;++i) {
    uint32_t seed=91000+i*37;
    float x=KelpForest.x+(noise(seed)-.5f)*70,z=KelpForest.z+(noise(seed+1)-.5f)*92;
    if(world::reserved(x,z,2)) continue;
    place(x,z,seed,5,11.0f+noise(seed+2)*6.0f);
#if defined(ARDUINO)
    if(i%24==0) delay(1);
#endif
  }
  material_=0; lighting_=1;
}

float Scene::surface(float x,float z,bool solids) const {
  if(x<WorldMin || z<WorldMin || x>=WorldMin+WorldSize || z>=WorldMin+WorldSize) return floor(x,z);
  int first=(int((z-WorldMin)/8)*(WorldSize/8)+int((x-WorldMin)/8))*2;
  auto height=[&](const Triangle& t,float& y) {
    Vec3 a=t.b-t.a,b=t.c-t.a;
    float den=a.x*b.z-a.z*b.x;
    if(std::abs(den)<.00001f) return false;
    float u=((x-t.a.x)*b.z-(z-t.a.z)*b.x)/den;
    float v=(a.x*(z-t.a.z)-a.z*(x-t.a.x))/den;
    if(u<-.0001f || v<-.0001f || u+v>1.0001f) return false;
    y=t.a.y+u*a.y+v*b.y; return true;
  };
  float result=-1000;
  for(int j=0;j<2;++j) { float y; if(height(triangles[first+j],y)) result=std::max(result,y); }
  if(solids && staticIndices) for(const auto& chunk:chunks) {
    if(x<chunk.minimum.x || x>chunk.maximum.x || z<chunk.minimum.z || z>chunk.maximum.z) continue;
    for(int j=0;j<chunk.count;++j) {
      int index=staticIndices[chunk.start+j];
      if(index<12800 || index>=solidCount) continue;
      float y; if(height(triangles[index],y)) result=std::max(result,y);
    }
  }
  return result;
}
void Scene::buildIndex(uint16_t* indices) {
  staticIndices=indices;
  for(auto& chunk:chunks) {
    chunk.minimum={1e20f,1e20f,1e20f}; chunk.maximum={-1e20f,-1e20f,-1e20f};
    chunk.start=0; chunk.count=0;
  }
  auto locate=[&](const Triangle& t) {
    Vec3 center=(t.a+t.b+t.c)*(1.0f/3);
    int x=std::max(0,std::min(ChunksAcross-1,int((center.x-WorldMin)/ChunkSize)));
    int z=std::max(0,std::min(ChunksAcross-1,int((center.z-WorldMin)/ChunkSize)));
    return z*ChunksAcross+x;
  };
  for(int i=0;i<staticCount;++i) {
    const auto& t=triangles[i]; Chunk& chunk=chunks[locate(t)]; ++chunk.count;
    for(Vec3 v:{t.a,t.b,t.c}) {
      chunk.minimum.x=std::min(chunk.minimum.x,v.x); chunk.minimum.y=std::min(chunk.minimum.y,v.y); chunk.minimum.z=std::min(chunk.minimum.z,v.z);
      chunk.maximum.x=std::max(chunk.maximum.x,v.x); chunk.maximum.y=std::max(chunk.maximum.y,v.y); chunk.maximum.z=std::max(chunk.maximum.z,v.z);
    }
  }
  uint16_t used[ChunkCount]={}; uint32_t start=0;
  for(auto& chunk:chunks) { chunk.start=start; start+=chunk.count; }
  for(int i=0;i<staticCount;++i) {
    int id=locate(triangles[i]);
    staticIndices[chunks[id].start+used[id]++]=uint16_t(i);
  }
}
} // abyss
