#if defined(__GNUC__)
// These finite, bounded geometry loops benefit from inlining and reciprocal math.
#pragma GCC optimize ("O3", "fast-math")
#endif
#include "scene.h"
#include "vanity_data.h"
#include "journal.h"
namespace abyss {
namespace {
const Color Sand{148,163,139}, Rock{90,104,137}, Rust{133,91,62};
const Landmark Landmarks[]={
  {{-12,-15,6},"THE STONE GATE"},
  {{14,-20,25},"THE FORGOTTEN HULL"},
  {{40,-27,8},"THE BLUE DROP"},
  {{-33,-20,30},"CORAL GARDEN"},
  {{24,-12,-29},"BASALT CATHEDRAL"}
};
}
// The wobble keeps the terraces from being a perfect mirror of themselves; it is part of
// the curve, so both directions have to carry it or the buildings sit off their own row.
float Scene::cityQ(float x,float z) { return z+x*x/CityBowl+3*std::sin(x*.025f); }
float Scene::cityRowZ(float x,float q) { return q-x*x/CityBowl-3*std::sin(x*.025f); }
float Scene::floor(float x,float z) const {
  if(city) {
    float q=cityQ(x,z),y=-238;
    for(int tier=0;tier<5;++tier) {
      float t=clampf((q+16-tier*CityTier)/8,0,1);
      y+=(tier==4?24.f:18.f)*t*t*(3-2*t);
    }
    // Past the last of the built ground the terracing stops and the rock simply climbs,
    // which is what makes this a hollow in the cliffs rather than a flight of steps with
    // nothing at either end of it.
    float wall=clampf((std::abs(x)-CityEdge)/26,0,1);
    y+=CityWall*wall*wall*(3-2*wall);
    return y+.35f*std::sin(x*.09f)*std::cos(z*.08f);
  }
  float y=world::floor(x,z);
  if(gateOpen) { float r=std::max(std::abs(x+12)/20,std::abs(z-4)/20);y-=40*clampf((1-r)*2,0,1); }
  return y;
}

void Scene::add(Vec3 a,Vec3 b,Vec3 c,Color col,bool twoSided) {
  if(count>=MaxTriangles) { overflow=true; return; }
  Vec3 n=unit(cross(b-a,c-a));
  float sun=dot(n,unit({-.45f,.86f,-.28f}));
  if(twoSided) sun=std::abs(sun);
  // One step brighter overall: a higher ambient floor and less depth wash.
  float illumination=(.60f+.52f*std::max(0.0f,sun))*lighting_;
  float deep=clampf((-(a.y+b.y+c.y)/3-12)/40,0,.7f);
  if(material_!=7) col=blend(scale(col,illumination),{34,86,98},deep*.38f);
  Triangle& triangle=triangles[count++];
  triangle.a=a; triangle.b=b; triangle.c=c;
  triangle.color=col; triangle.material=uint8_t(material_|(twoSided?0:SolidFace));
  if(material_ && material_<=6) for(int i=0;i<8;++i) triangle.shades[i]=rgb565(scale(col,.62f+i*.105f));
}
void Scene::addSolid(Vec3 a,Vec3 b,Vec3 c,Color col,Vec3 outward) {
  if(dot(cross(b-a,c-a),outward)<0) add(a,c,b,col,false);
  else add(a,b,c,col,false);
}
// The terraced ruin in the Primordial Sea. The geometry is baked; all this does is
// stand it on the seabed at its site and hand each face over already wound outward,
// so the far side of it costs nothing to draw.
void Scene::vanity() {
  const Vec3 site{-44.0f,0,173.0f};
  auto env=world::environment(site.x,site.z);
  material_=1; lighting_=env.light;
  // Sit it on the lowest ground under its footprint, so no terrace floats.
  float bed=surface(site.x,site.z);
  for(int k=0;k<12;++k) {
    float a=k*Pi/6;
    bed=std::min(bed,surface(site.x+std::cos(a)*14,site.z+std::sin(a)*14));
  }
  for(int i=0;i<VanityFaceCount;++i) {
    const auto& f=VanityFaces[i];
    const auto& t=VanityTones[f.tone];
    Vec3 p[3];
    for(int k=0;k<3;++k)
      p[k]={site.x+f.v[k*3]*.01f,bed+f.v[k*3+1]*.01f,site.z+f.v[k*3+2]*.01f};
    add(p[0],p[1],p[2],{t[0],t[1],t[2]},false);
  }
  material_=0; lighting_=1;
}
void Scene::quadSolid(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col,Vec3 outward) {
  addSolid(a,b,c,col,outward); addSolid(a,c,d,col,outward);
}
void Scene::quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col) {
  add(a,b,c,col); add(a,c,d,col);
}
void Scene::rock(Vec3 p,Vec3 size,uint32_t seed,int style) {
  material_=1;
  auto env=world::environment(p.x,p.z); lighting_=env.light;
  const float profiles[6][5]={
    {.62f,.94f,1,.79f,.30f}, {1,1,.96f,.85f,.78f}, {.95f,.91f,.75f,.85f,.74f},
    {.70f,1,.56f,.35f,.07f}, {.45f,.47f,.97f,1,.66f}, {1,1,.95f,.91f,.88f}
  };
  const float levels[]={-.1f,.15f,.45f,.78f,1};
  int sides=style==5?6:8;
  Vec3 rings[5][8];
  float yaw=noise(seed)*Pi,lean=(noise(seed+5)-.5f)*size.x*.7f;
  for(int j=0;j<5;++j) for(int k=0;k<sides;++k) {
    float a=k*2*Pi/sides+yaw;
    float r=profiles[style%6][j]*(.88f+.20f*noise(seed+j*31+k));
    rings[j][k]=p+Vec3{std::cos(a)*size.x*r+lean*levels[j],
      size.y*(levels[j]+(j?.04f*noise(seed+130+j*13+k):0)),
      std::sin(a)*size.z*r};
  }
  // A rock is a closed lump standing on the seabed: its far side and its underside are
  // never seen, so the sides are wound radially outward and the cap upward, and the
  // renderer drops whichever half is facing away.
  // The cross-section is elliptical and each level is offset by the lean, so outward is
  // taken as the face centre minus the axis at that height rather than a plain radius.
  for(int j=0;j<4;++j) for(int k=0;k<sides;++k) {
    Color color=scale(env.rock,.88f+.16f*noise(seed+k*13+j*73));
    Vec3 axis=p+Vec3{lean*(levels[j]+levels[j+1])*.5f,size.y*(levels[j]+levels[j+1])*.5f,0};
    Vec3 mid=(rings[j][k]+rings[j][(k+1)%sides]+rings[j+1][(k+1)%sides]+rings[j+1][k])*.25f;
    quadSolid(rings[j][k],rings[j][(k+1)%sides],
              rings[j+1][(k+1)%sides],rings[j+1][k],color,mid-axis);
  }
  for(int k=1;k<sides-1;++k) addSolid(rings[4][0],rings[4][k],rings[4][k+1],env.rock,{0,1,0});
}

void Scene::beam(Vec3 a,Vec3 b,float width,Color col) {
  material_=3;
  Vec3 axis=unit(b-a);
  Vec3 u=unit(cross(axis,std::abs(axis.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0}))*width;
  Vec3 v=unit(cross(axis,u))*width;
  Vec3 r[4]={u+v,u-v,(u+v)*-1,v-u};
  for(int k=0;k<4;++k) quad(a+r[k],a+r[(k+1)%4],b+r[(k+1)%4],b+r[k],col);
  quad(b+r[0],b+r[1],b+r[2],b+r[3],col);
  // Both ends, not one. An open box is fine while every face is drawn from either side,
  // but it shows its inside the moment the ones facing away stop being drawn, and the
  // city wants them to stop.
  quad(a+r[3],a+r[2],a+r[1],a+r[0],col);
}
void Scene::cliff(Vec3 start,Vec3 end,float width,float height,uint32_t seed) {
  material_=1;
  Vec3 tangent=unit(end-start),normal={tangent.z,0,-tangent.x};
  Vec3 points[10][5];
  const float offsets[5]={-1,-.50f,-.16f,.08f,.55f};
  const float levels[5]={0,.14f,.77f,1,.04f};
  for(int i=0;i<10;++i) {
    Vec3 center=mix(start,end,i/9.0f);
    float elevation=height*(.84f+.22f*noise(seed+i));
    for(int j=0;j<5;++j) {
      Vec3 v=center+normal*(width*(offsets[j]+(noise(seed+i*31+j)-.5f)*.18f));
      v.y=floor(v.x,v.z)-.3f+levels[j]*elevation;
      points[i][j]=v;
    }
  }
  for(int i=0;i<9;++i) for(int j=0;j<4;++j) {
    Color col=blend({92,105,133},{121,113,116},noise(seed+i*11+j)*.50f);
    quad(points[i][j],points[i+1][j],points[i+1][j+1],points[i][j+1],col);
  }
  for(int j=1;j<4;++j) {
    add(points[0][0],points[0][j],points[0][j+1],Rock);
    add(points[9][0],points[9][j+1],points[9][j],Rock);
  }
}
void Scene::ruins() {
  const Color stone{141,151,138};
  Vec3 p{-6,floor(-6,39),39};
  for(int i=0;i<4;++i) {
    Vec3 base=p+Vec3{float(i*3),0,0};
    beam(base,base+Vec3{0,i==2?3.0f:6.0f,0},.45f,stone);
    beam(base+Vec3{-.65f,.25f,0},base+Vec3{.65f,.25f,0},.58f,stone);
    if(i<2) beam(base+Vec3{-.7f,6,0},base+Vec3{3.3f,6,0},.42f,stone);
  }
  beam(p+Vec3{3,.6f,2},p+Vec3{8,1.0f,5},.46f,stone);
}
void Scene::arch() {
  material_=1;
  Vec3 origin{-12,floor(-12,6)+2.0f,6};
  for(int i=0;i<11;++i) {
    float a=i*Pi/11,b=(i+1)*Pi/11;
    float ra=8.6f+.4f*std::sin(i*1.9f), rb=8.6f+.4f*std::sin((i+1)*1.9f);
    auto point=[&](float angle,float radius,float z) {
      return origin+Vec3{std::cos(angle)*radius,std::sin(angle)*radius*1.14f,z};
    };
    Vec3 af=point(a,5.6f,-2),bf=point(b,5.6f,-2);
    Vec3 ao=point(a,ra,-2.5f),bo=point(b,rb,-2.5f);
    Vec3 ab=point(a,5.6f,2),bb=point(b,5.6f,2);
    Vec3 aob=point(a,ra,2.5f),bob=point(b,rb,2.5f);
    Color c=blend({116,145,122},{85,120,109},noise(i+405));
    quad(af,bf,bo,ao,c); quad(bb,ab,aob,bob,c);
    quad(ab,bb,bf,af,scale(c,.72f)); quad(ao,bo,bob,aob,c);
    if(i==0) quad(ab,af,ao,aob,c);
    if(i==10) quad(bf,bb,bob,bo,c);
  }
  rock({-19.3f,floor(-19.3f,6),6},{3.2f,5.0f,4.5f},201);
  rock({-4.7f,floor(-4.7f,6),6},{3.2f,4.8f,4.5f},202);
}
void Scene::wreck() {
  material_=3;
  Vec3 origin{14,floor(14,25)-.4f,25};
  auto world=[&](Vec3 v) {
    // The hull lists to port and lies partly buried in the sand.
    float x=v.x*.983f-v.y*.184f,y=v.x*.184f+v.y*.983f;
    return origin+rotateY({x,y,v.z},-.48f);
  };
  auto plank=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col) {
    quad(world(a),world(b),world(c),world(d),col);
  };
  const float zs[]={-12,-8,-3,2,7,12,14};
  const float ws[]={1.6f,3.8f,4.2f,4.1f,3.4f,1.6f,0};
  for(int i=0;i<6;++i) {
    float za=zs[i],zb=zs[i+1],wa=ws[i],wb=ws[i+1];
    plank({-wa*.6f,.4f,za},{wa*.6f,.4f,za},{wb*.6f,.4f,zb},{-wb*.6f,.4f,zb},scale(Rust,.65f));
    for(int side=-1;side<=1;side+=2) {
      plank({side*wa*.6f,.4f,za},{side*wa,2.3f,za},
            {side*wb,2.3f,zb},{side*wb*.6f,.4f,zb},Rust);
      if((i==2 && side==-1)||i==3) continue;
      plank({side*wa,2.3f,za},{side*wa*.91f,4.6f,za},
            {side*wb*.91f,4.6f,zb},{side*wb,2.3f,zb},
            blend(Rust,{100,129,105},i*.08f));
    }
    if(i<2 || i>3)
      plank({-wa*.91f,4.55f,za},{wa*.91f,4.55f,za},
            {wb*.91f,4.55f,zb},{-wb*.91f,4.55f,zb},{104,110,87});
  }
  for(int i=1;i<6;++i) {
    float z=zs[i],w=ws[i];
    beam(world({-w,2,z}),world({-w*.9f,4.9f,z}),.11f,Rust);
    beam(world({w,2,z}),world({w*.9f,4.9f,z}),.11f,Rust);
    if(i==2||i==3) beam(world({-w*.85f,1,z}),world({w*.85f,1,z}),.12f,Rust);
  }
  // Roof on thin window pillars; open windows remain actual gaps.
  for(int side=-1;side<=1;side+=2) {
    for(int z=-9;z<=-5;z+=2)
      beam(world({side*2.1f,4.5f,float(z)}),world({side*2.1f,7.1f,float(z)}),.10f,Rust);
    plank({side*2.1f,4.5f,-9},{side*2.1f,5.5f,-9},
          {side*2.1f,5.5f,-5},{side*2.1f,4.5f,-5},Rust);
  }
  plank({-2.4f,7.2f,-9.5f},{2.4f,7.2f,-9.5f},{2.4f,7.2f,-4.6f},{-2.4f,7.2f,-4.6f},{99,127,107});
  beam(world({0,4.5f,7}),world({1.8f,13,6}),.16f,Rust);
  beam(world({-.4f,10,6.4f}),world({3.3f,10,6.4f}),.10f,Rust);
  for(int i=0;i<7;++i) {
    Vec3 a{6+noise(70+i)*5,0,-8+noise(100+i)*18};
    beam(world(a),world(a+Vec3{1,.2f,2+noise(200+i)*3}),.15f,{94,104,80});
  }
}
void Scene::coral(Vec3 p,float size,uint32_t seed) {
  Color c=blend({133,111,99},{168,136,96},noise(seed));
  beam(p,p+Vec3{0,size,0},.075f*size,c);
  for(int j=0;j<3;++j) {
    float yaw=noise(seed+13*j)*Pi*2;
    Vec3 a=p+Vec3{0,size*(.35f+.18f*j),0};
    Vec3 b=a+Vec3{std::cos(yaw)*size*.55f,size*.45f,std::sin(yaw)*size*.55f};
    beam(a,b,.045f*size,c);
    beam(b,b+Vec3{.15f*size,.40f*size,0},.026f*size,c);
  }
}
bool Scene::build(Triangle* storage,uint16_t* indices) {
  city=false; gateSink=0;
  triangles=storage; count=staticCount=0; overflow=false;
  if(!storage || !indices) return false;
  buildMap();
  lighting_=1;
  // Tall, deliberately spaced formations form a canyon without closing the tour.
  const Vec3 formations[]={
    {-29,10,-4},{-30,13,7},{-27,8,19},{2,8,0},{4,11,8},{6,6,16},
    {26,17,-27},{19,13,-25},{29,21,-33},{34,15,-30},{22,10,-34},
    {55,19,-15},{57,23,-2},{59,17,12},{52,11,25},
    {-49,12,22},{-49,18,33},{-42,11,42},{-20,10,42}
  };
  for(unsigned i=0;i<sizeof(formations)/sizeof(formations[0]);++i) {
    Vec3 v=formations[i];
    rock({v.x,floor(v.x,v.z)-1,v.z},{3+noise(i+7)*2,v.y,3+noise(i+55)*2},1000+i*91);
  }
  for(int i=0;i<11;++i) {
    float x=-37+noise(i+830)*15,z=23+noise(i+930)*12;
    rock({x,floor(x,z)-.4f,z},{1.5f+noise(i+132),1.2f+noise(i+14)*2,1.8f},2300+i);
    if(i<6) coral({x,floor(x,z)+1.0f,z},1.4f+noise(i+1)*1.0f,730+i);
  }
  gateStart=count;
  if(!gateOpen) arch();
  gateEnd=count;
  wreck(); ruins(); vanity();
  buildVents();
  buildMonuments();
  solidCount=count;
  buildSeabedLife();
  staticCount=count;
  buildIndex(indices);
  ecosystem.build(*this);
  return !overflow;
}
void Scene::fish(Vec3 p,float yaw,float size,float phase,Color col,bool whale) {
  material_=0;
  float sine=std::sin(yaw),cosine=std::cos(yaw);
  auto world=[&](Vec3 v) {
    return p+Vec3{cosine*v.x+sine*v.z,v.y,-sine*v.x+cosine*v.z}*size;
  };
  auto tri=[&](Vec3 a,Vec3 b,Vec3 c,Color color) { add(world(a),world(b),world(c),color); };
  const int N=6;
  Vec3 rings[3][N];
  float z[3]={-.65f,.05f,.65f};
  float w[3]={.12f,.34f,.23f};
  float h[3]={.16f,.43f,.30f};
  if(whale) { w[0]=.22f; w[1]=.46f; w[2]=.40f; h[0]=.20f; h[1]=.44f; h[2]=.35f; }
  for(int j=0;j<3;++j) for(int k=0;k<N;++k) {
    float a=k*2*Pi/N;
    rings[j][k]={std::cos(a)*w[j],std::sin(a)*h[j],z[j]};
  }
  for(int j=0;j<2;++j) for(int k=0;k<N;++k) {
    int n=(k+1)%N;
    Color c=k>=3?blend(col,{169,185,166},.48f):col;
    tri(rings[j][k],rings[j][n],rings[j+1][n],c);
    tri(rings[j][k],rings[j+1][n],rings[j+1][k],c);
  }
  for(int k=0;k<N;++k) tri(rings[2][k],rings[2][(k+1)%N],{0,.01f,.96f},col);
  float sw=std::sin(phase)*.20f;
  Vec3 tail{whale?0:sw,whale?sw:0,-1.20f};
  for(int k=0;k<N;++k) tri(rings[0][(k+1)%N],rings[0][k],tail,col);
  if(whale) {
    tri(tail,{-.73f,sw-.06f,-1.42f},{-.23f,sw+.03f,-.84f},col);
    tri(tail,{.23f,sw+.03f,-.84f},{.73f,sw-.06f,-1.42f},col);
    tri({-.35f,-.12f,.35f},{-1.0f,-.36f,-.20f},{-.28f,-.18f,-.16f},col);
    tri({.35f,-.12f,.35f},{.28f,-.18f,-.16f},{1.0f,-.36f,-.20f},col);
  } else {
    tri(tail,{sw,.44f,-1.44f},{sw,-.44f,-1.44f},scale(col,.85f));
    tri({-.20f,-.1f,.15f},{-.56f,-.15f,-.24f},{-.18f,-.12f,-.2f},col);
    tri({.20f,-.1f,.15f},{.18f,-.12f,-.2f},{.56f,-.15f,-.24f},col);
  }
  tri({0,.36f,.14f},{0,whale?.71f:.65f,-.31f},{0,.20f,-.5f},col);
}
void Scene::poseCreature(const ecology::Animal& animal,bool whale) {
  count=0; lighting_=1; viewer_=animal.position+Vec3{0,0,animal.length*1.4f};
  if(whale) fish(animal.position,animal.yaw,animal.length,animal.phase,{66,105,126},true);
  else animalMesh(animal);
}
void Scene::animate(float time) { animate(time,tour(time)); }
void Scene::animate(float time,const Camera& cam) {
  count=staticCount; lighting_=1; whalePresent=false; overflow=false; viewer_=cam.position;
  if(city) {animateCity(time,cam);return;}
  ecosystem.update(*this,cam,time);
  lighting_=std::max(.8f,world::environment(cam.position.x,cam.position.z).light);
  for(int i=0;i<ecosystem.count;++i) animalMesh(ecosystem.animals[i]);
  lighting_=1;
  Vec3 eye=cam.position;
  if(world::environment(eye.x,eye.z).zone!=0 || eye.x*eye.x+eye.z*eye.z>160*160) return;
  float a=time*.026f+.8f;
  Vec3 p{35+std::cos(a)*12,-11+std::sin(time*.039f)*2,-8+std::sin(a)*16};
  whaleLocation=p; whalePresent=true;
  fish(p,std::atan2(-12*std::sin(a),16*std::cos(a)),7.2f,time*1.3f,{66,105,126},true);
}
Camera Scene::tour(float time) const {
  if(city) return cityTour(time);
  Camera camera=world::tour(time);
  // Brief viewing windows prevent a nearby statue from stealing the opening gate view.
  // One entry per monument, indexed by id. It has to keep pace with Journal::catalog;
  // it did not, and reading past the end both corrupted the stack and gave the newest
  // monument a viewing window made of whatever happened to be there.
  constexpr float watchTime[Journal::MonumentCount]={0,0,0,244,270,396,420,845,127,722,692,536,198,317,469,624,776};
  static_assert(sizeof(watchTime)/sizeof(watchTime[0])==Journal::MonumentCount,
                "every monument needs a viewing time");
  float now=std::fmod(std::max(0.0f,time),TourSeconds),best=0;const Monument* nearest=nullptr;
  for(const auto& entry:Journal::catalog) {
    if(entry.id<3 || !entry.placed) continue;
    Vec3 d=entry.position-camera.position;d.y=0;
    float window=clampf((22-std::abs(now-watchTime[entry.id]))/12,0,1);
    float weight=clampf((64-length(d))/28,0,1)*window;
    if(weight>best) {best=weight;nearest=&entry;}
  }
  if(nearest) {
    Vec3 focus=nearest->position;focus.y+=floor(focus.x,focus.z);
    float weight=best*best*(3-2*best);
    Vec3 direction=mix(camera.forward,unit(focus-camera.position),weight);
    if(length(direction)>.01f) camera.lookAt(camera.position,camera.position+direction);
  }
  return camera;
}
const Landmark* Scene::nearby(Vec3 eye) const {
  if(city) {static const Landmark name{{0,-200,0},"UNDERSEA CITY"};return &name;}
  const Landmark* best=nullptr; float distance=24;
  for(const auto& l:Landmarks) {
    float d=length(l.position-eye);
    if(d<distance) { best=&l; distance=d; }
  }
  return best;
}
}
