#include "scene.h"
#include "journal.h"
namespace abyss {
namespace {
// Small reusable polygon solids. All dimensions are metres in local monument space.
struct Mason {
 Scene& scene; Vec3 origin; float sine,cosine;
 Mason(Scene& s,Vec3 p,float yaw):scene(s),origin(p),sine(std::sin(yaw)),cosine(std::cos(yaw)) {}
 Vec3 at(Vec3 p) const { return origin+Vec3{p.x*cosine+p.z*sine,p.y,-p.x*sine+p.z*cosine}; }
 void face(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color color) {
  scene.add(at(a),at(b),at(c),color);scene.add(at(a),at(c),at(d),color);
 }
 void box(Vec3 p,Vec3 half,Color color) {
  Vec3 v[8];for(int i=0;i<8;++i) v[i]=p+Vec3{(i&1)?half.x:-half.x,(i&2)?half.y:-half.y,(i&4)?half.z:-half.z};
  const int sides[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
  for(const auto& q:sides) face(v[q[0]],v[q[1]],v[q[2]],v[q[3]],color);
 }
 void taper(Vec3 p,float y,float bottom,float top,Color color,int n=8,float stretch=1) {
  for(int i=0;i<n;++i) {
   float a=2*Pi*i/n,b=2*Pi*(i+1)/n;
   Vec3 aa{std::cos(a)*bottom,0,std::sin(a)*bottom*stretch},bb{std::cos(b)*bottom,0,std::sin(b)*bottom*stretch};
   Vec3 cc{std::cos(b)*top,y,std::sin(b)*top*stretch},dd{std::cos(a)*top,y,std::sin(a)*top*stretch};
   face(p+aa,p+bb,p+cc,p+dd,color);
   scene.add(at(p+Vec3{0,y,0}),at(p+dd),at(p+cc),color);
  }
 }
 void ellipsoid(Vec3 p,Vec3 size,Color color) {
  for(int j=0;j<4;++j) for(int k=0;k<8;++k) {
   auto point=[&](int ring,int side) {float a=-Pi*.5f+Pi*ring/4,b=2*Pi*side/8;
    return p+Vec3{std::cos(a)*std::cos(b)*size.x,std::sin(a)*size.y,std::cos(a)*std::sin(b)*size.z};};
   face(point(j,k),point(j,k+1),point(j+1,k+1),point(j+1,k),color);
  }
 }
 void bar(Vec3 a,Vec3 b,float radius,Color color) {
  Vec3 axis=unit(b-a),u=unit(cross(axis,std::abs(axis.y)<.9f?Vec3{0,1,0}:Vec3{1,0,0}))*radius,v=unit(cross(axis,u))*radius;
  Vec3 r[4]={u+v,u-v,(u+v)*-1,v-u};
  for(int i=0;i<4;++i) face(a+r[i],a+r[(i+1)%4],b+r[(i+1)%4],b+r[i],color);
  face(b+r[0],b+r[1],b+r[2],b+r[3],color);
 }
};
}
void Scene::buildMonuments() {
 for(const auto& entry:Journal::catalog) {
  if(entry.id<3 || !entry.placed) continue;
  Vec3 p=entry.position;p.y=surface(p.x,p.z)-.45f;
  auto env=world::environment(p.x,p.z); lighting_=env.light;material_=5;
  // Front (+Z) faces the neighbouring stretch of the tour, not a global compass direction.
  // One per monument, sized off the catalogue and asserted below.
  constexpr float yaw[Journal::MonumentCount]={0,0,0,-.7f,-2.7f,-1.1f,.2f,-1.8f,.7f,.7f,.9f,
    2.1f, -0.9f, 1.6f, 0.4f, -2.4f};
  static_assert(sizeof(yaw)/sizeof(yaw[0])==Journal::MonumentCount,
                "every monument needs a facing");
  Mason m(*this,p,yaw[entry.id]);
  Color stone{177,167,137},sandstone{211,184,127},bronze{65,122,105},dark{44,64,72};
  switch(entry.id) {
   case 3: { // Linked base, twin stepped towers, vertical bays.
    Color concrete{165,181,180};
    m.box({0,2,0},{10,3,4.5f},concrete);
    for(int side:{-1,1}) {
     float x=side*5.5f;
     m.box({x,12,0},{3.9f,7,3.9f},concrete);
     m.taper({x,19,0},4,4.5f,3.4f,concrete,8);
     m.taper({x,23,0},3,3.4f,2.4f,concrete,8);
     for(int j=-2;j<=2;++j) {
      m.box({x+j*1.2f,12,3.94f},{.17f,6.7f,.05f},dark);
      m.box({x+j*1.2f,12,-3.94f},{.17f,6.7f,.05f},dark);
     }
     for(int h=8;h<=17;h+=3) m.box({x,float(h),0},{4,.13f,4},concrete);
    }
    break;
   }
   case 4: { material_=2; // Recumbent lion, projecting paws, human face and broad headdress.
    m.ellipsoid({0,2,-2},{3.1f,2.5f,6.8f},sandstone);
    m.box({-1.65f,.65f,5.3f},{1,.85f,3.5f},sandstone);
    m.box({1.65f,.65f,5.3f},{1,.85f,3.5f},sandstone);
    m.taper({0,2.5f,3},4.9f,2.5f,1.9f,sandstone,4,.7f);
    m.box({0,5.65f,4.5f},{1.1f,1.45f,.8f},sandstone);
    m.box({0,7.2f,3.9f},{1.5f,.3f,1.4f},sandstone);
    m.box({0,5.3f,5.4f},{.25f,.5f,.18f},stone);
    m.box({0,4.2f,5},{.3f,.65f,.25f},sandstone);
    for(int side:{-1,1}) m.box({side*.57f,6,5.32f},{.34f,.12f,.025f},dark);
    break;
   }
   case 5: {
    Color vermilion{146,74,53};material_=3;
    for(int side:{-1,1}) {
     m.bar({side*4.4f,-1,0},{side*3.8f,9,0},.6f,vermilion);
     m.box({side*4.3f,.2f,0},{.95f,.5f,.95f},stone);
    }
    m.box({0,6.6f,0},{5.4f,.35f,.38f},vermilion);
    m.box({0,8.7f,0},{5.8f,.5f,.7f},vermilion);
    m.bar({-7,9.5f,0},{-3.5f,9.1f,0},.34f,dark);
    m.bar({-3.5f,9.1f,0},{3.5f,9.1f,0},.34f,dark);
    m.bar({3.5f,9.1f,0},{7,9.5f,0},.34f,dark);
    m.box({0,7.65f,.6f},{.5f,.7f,.12f},stone);break;
   }
   case 6: { // Broken circular observatory: a missing dome sector reveals its telescope.
    m.taper({0,-1,0},2,7,7,stone,12);
    for(int i=0;i<10;++i) {
     float a=2*Pi*i/12,b=2*Pi*(i+1)/12;
     m.face({6*std::cos(a),1,6*std::sin(a)},{6*std::cos(b),1,6*std::sin(b)},
            {6*std::cos(b),5,6*std::sin(b)},{6*std::cos(a),5,6*std::sin(a)},stone);
     for(int j=0;j<3;++j) {
      auto v=[&](float angle,int h) {float t=h*Pi/6;return Vec3{6*std::cos(t)*std::cos(angle),5+5*std::sin(t),6*std::cos(t)*std::sin(angle)};};
      m.face(v(a,j),v(b,j),v(b,j+1),v(a,j+1),{88,130,129});
     }
    }
    material_=3;m.bar({0,1,0},{0,5,0},.4f,dark);
    m.bar({-2,4,1},{2.7f,7,-1},.65f,{136,139,113});break;
   }
   case 7: { // Kept blocky on purpose: it has to read at the size the panel gives it.
    m.box({0,.8f,0},{3.2f,1.25f,2.5f},stone);material_=0;
    m.taper({-.7f,2,0},3.2f,1.15f,.85f,bronze,6,.6f);
    m.ellipsoid({-.7f,6,0},{.7f,.85f,.65f},bronze);
    m.bar({-1.5f,4.7f,0},{-1.65f,3.3f,.55f},.26f,bronze);
    m.bar({.1f,4.7f,0},{.65f,3.7f,.75f},.27f,bronze);
    m.box({-.7f,3.6f,.65f},{.95f,.13f,.1f},dark);
    m.box({-1.3f,2,.6f},{.4f,.2f,.7f},bronze);m.box({-.1f,2,.6f},{.4f,.2f,.7f},bronze);
    m.ellipsoid({1.7f,2.8f,.5f},{.48f,.5f,1},bronze);
    m.ellipsoid({1.7f,3.35f,1.3f},{.42f,.5f,.45f},bronze);
    for(int s:{-1,1}) {
     m.taper({1.7f+s*.25f,3.6f,1.3f},.55f,.22f,0,bronze,4);
     for(int z:{-1,1}) m.bar({1.7f+s*.33f,2.6f,.5f+z*.65f},{1.7f+s*.33f,1.8f,.5f+z*.65f},.12f,bronze);
    }
    m.bar({1.7f,2.9f,-.4f},{1.7f,3.5f,-.85f},.13f,bronze);
    m.bar({.65f,3.7f,.75f},{1.7f,3.1f,1.1f},.035f,dark);break;
   }
   case 8: {
    for(int side:{-1,1}) {
     m.box({side*4.4f,3,0},{1.7f,4,2},stone);
     m.box({side*4.4f,.4f,0},{2.1f,.65f,2.5f},stone);
     m.box({side*4.4f,6,2.04f},{.8f,1.2f,.07f},{133,139,119});
    }
    for(int i=0;i<10;++i) {
     float a=Pi*i/10,b=Pi*(i+1)/10;
     Vec3 v[4]={{2.7f*std::cos(a),5+2.7f*std::sin(a),0},{2.7f*std::cos(b),5+2.7f*std::sin(b),0},
                {4.3f*std::cos(b),5+4.3f*std::sin(b),0},{4.3f*std::cos(a),5+4.3f*std::sin(a),0}};
     for(int side:{-1,1}) m.face(v[0]+Vec3{0,0,side*2.f},v[1]+Vec3{0,0,side*2.f},v[2]+Vec3{0,0,side*2.f},v[3]+Vec3{0,0,side*2.f},stone);
     m.face(v[0]+Vec3{0,0,-2},v[1]+Vec3{0,0,-2},v[1]+Vec3{0,0,2},v[0]+Vec3{0,0,2},stone);
    }
    m.box({0,9.7f,0},{6.5f,.7f,2.5f},stone);break;
   }
   case 9: { material_=1;
    Color basalt{112,117,127};
    m.taper({0,-2,0},5,2.5f,1.75f,basalt,6,.7f);
    m.box({0,5,0},{1.55f,2.2f,1.1f},basalt);
    m.box({0,6.2f,1.2f},{1.6f,.28f,.45f},basalt);
    m.box({0,5.2f,1.4f},{.32f,1.05f,.55f},basalt);
    m.box({0,3.9f,1.15f},{.82f,.22f,.32f},basalt);
    m.box({0,3.4f,1.05f},{1,.25f,.38f},basalt);
    for(int side:{-1,1}) {m.box({side*1.65f,5.3f,0},{.25f,1.25f,.35f},basalt);m.box({side*.8f,5.8f,1.12f},{.45f,.17f,.04f},dark);}break;
   }
   case 10: {
    m.box({0,-.2f,0},{9,.8f,4},stone);
    for(int row:{-1,1}) for(int i=0;i<4;++i) {
     float x=-6+i*4.f,height=(i==2 && row==1)?2.5f:7;
     m.taper({x,0,row*2.5f},height,.65f,.48f,stone,6);
     m.box({x,height,row*2.5f},{1,.35f,1},stone);
    }
    m.box({-4,7.6f,-2.5f},{3.2f,.4f,1},stone);
    m.box({4,7.6f,-2.5f},{3.2f,.4f,1},stone);
    m.bar({1,0,6},{5,1,7},.6f,stone);break;
   }
   case 12: { material_=6;   // Something very large died here and the sand kept the shape.
    Color bone{216,208,178},shadow{150,144,120};
    // Spine in a long slack curve, most of it still under the sand at the tail end.
    for(int i=0;i<24;++i) {
     float t=i/23.0f,x=-15+t*31;
     float y=4.2f+std::sin(t*2.6f)*2.1f-t*t*1.3f;
     float r=.92f-t*.44f;
     m.box({x,y,0},{r*.7f,r,r*.8f},i%2?bone:shadow);
     if(i%3==0 && t<.62f) m.box({x,y+r+.7f,0},{.20f,.85f,.20f},bone);   // neural spines
     if(i>3 && i<17) for(int side:{-1,1}) {
      float open=.9f+std::sin(t*3.1f)*.5f;
      m.bar({x,y,side*r*.6f},{x+.7f,y-1.5f,side*(3.4f*open)},.22f,bone);
      m.bar({x+.7f,y-1.5f,side*(3.4f*open)},{x+.3f,y-4.2f,side*(4.4f*open)},.19f,bone);
     }
    }
    // Skull: a long jaw, half sunk, with the eye socket open to the water.
    m.ellipsoid({-18.4f,4.6f,0},{4.6f,2.0f,2.1f},bone);
    m.bar({-22.6f,3.2f,-.75f},{-15.2f,4.0f,-.75f},.36f,bone);
    m.bar({-22.6f,3.2f,.75f},{-15.2f,4.0f,.75f},.36f,bone);
    m.box({-19.2f,5.4f,1.55f},{.75f,.68f,.07f},{34,30,26});
    m.box({-19.2f,5.4f,-1.55f},{.75f,.68f,.07f},{34,30,26});
    for(int i=0;i<8;++i) m.box({-22.0f+i*1.0f,2.7f,0},{.17f,.52f,1.0f},bone);
    // Ribs that have fallen away and lie loose on the bed.
    for(int i=0;i<5;++i) {
     float x=-3+i*4.6f,z=(i%2?1:-1)*(6.0f+i*.7f);
     m.bar({x,.3f,z},{x+3.6f,.45f,z+(i%2?2.0f:-2.0f)},.20f,shadow);
    }
    break;
   }
   case 13: { // A castle of the storybook kind: a keep, corner turrets, a curtain wall.
    Color wall{184,179,166},roof{66,88,116},dark2{40,52,64};
    auto turret=[&](float x,float z,float base,float shaft,float spire) {
     m.taper({x,0,z},shaft,base,base*.88f,wall,8);
     m.taper({x,shaft,z},.7f,base*1.12f,base*1.12f,wall,8);         // corbelled band
     m.taper({x,shaft+.7f,z},spire,base*1.05f,.05f,roof,8);
     for(int i=0;i<3;++i) m.box({x,shaft*.35f+i*shaft*.24f,z+base*.9f},{.24f,.42f,.06f},dark2);
    };
    // Curtain wall around three sides, broken open where the sea took it.
    for(int i=0;i<10;++i) {
     float a=Pi*.25f+i*Pi*.17f;
     float x=std::cos(a)*12.5f,z=std::sin(a)*12.5f;
     m.box({x,3.4f,z},{2.0f,3.4f,1.4f},i==4?Color{150,146,136}:wall);
     if(i!=4) m.box({x,7.2f,z},{2.0f,.6f,1.7f},wall);
    }
    turret(-9.6f,-8.0f,2.5f,12.5f,6.6f);
    turret( 9.6f,-8.0f,2.2f,10.5f,5.8f);
    turret(-8.6f, 9.1f,2.1f,9.2f,5.2f);
    turret( 8.6f, 9.1f,1.9f,8.2f,4.6f);
    // The keep, and the tall spire that makes the silhouette.
    m.box({0,9.8f,0},{6.0f,9.8f,5.5f},wall);
    m.taper({0,19.6f,0},1.3f,6.4f,6.4f,wall,4);
    m.taper({0,20.9f,0},9.0f,5.8f,.05f,roof,4);
    m.taper({3.4f,20.9f,2.9f},4.2f,1.6f,.05f,roof,6);
    m.taper({-3.4f,20.9f,2.9f},4.2f,1.6f,.05f,roof,6);
    for(int i=0;i<4;++i) m.box({-3.4f+i*2.2f,11.5f,5.6f},{.5f,1.5f,.12f},dark2);
    for(int i=0;i<3;++i) m.box({-2.4f+i*2.4f,16.5f,5.6f},{.45f,1.2f,.12f},dark2);
    // Gatehouse, with the arch open.
    m.box({-3.3f,4.0f,11.2f},{1.8f,4.0f,1.6f},wall);
    m.box({ 3.3f,4.0f,11.2f},{1.8f,4.0f,1.6f},wall);
    m.box({0,8.5f,11.2f},{5.1f,.9f,1.6f},wall);
    break;
   }
   case 14: { material_=2;   // Rectilinear terraces cut, or grown, in one mass of rock.
    Color rock{104,112,116},lit{132,140,142},shade{74,82,88};
    // Big square steps climbing to a flat summit, sharp right angles throughout. The
    // ground rises about seven metres across the site, so the lowest course is deep
    // enough to stand clear of the slope instead of being swallowed by it.
    const float w[]={21.0f,16.8f,13.0f,9.5f,6.3f};
    const float d[]={19.3f,15.4f,11.9f,8.8f,6.0f};
    float y=0;
    for(int i=0;i<5;++i) {
     float h=i==0?6.0f:2.6f;
     m.box({0,y+h*.5f,0},{w[i],h*.5f,d[i]},i%2?rock:lit);
     y+=h;
    }
    // A straight channel cut down one flank, and a stair beside it.
    for(int i=0;i<7;++i) m.box({-11.2f,2.0f+i*2.0f,-14.7f+i*4.6f},{3.7f,1.0f,2.3f},shade);
    m.box({9.8f,7.0f,-3.5f},{1.9f,7.0f,16.4f},shade);
    m.box({-3.0f,7.0f,14.6f},{9.2f,7.0f,1.8f},shade);
    // Loose slabs lying where they came off the top.
    m.box({14.7f,3.6f,10.5f},{6.0f,.9f,4.6f},rock);
    m.box({-17.2f,2.2f,8.8f},{3.9f,2.2f,3.2f},lit);
    m.box({6.0f,1.0f,-19.0f},{4.4f,1.0f,2.8f},rock);
    break;
   }
   case 15: { material_=0;   // One slab, 1 : 4 : 9, standing where nothing else does.
    Color slab{18,19,22},edge{38,40,46};
    m.box({0,6.3f,0},{.70f,6.3f,2.80f},slab);
    // Just enough edge lighting to read as an object rather than a hole.
    m.box({0,12.58f,0},{.72f,.04f,2.82f},edge);
    for(int side:{-1,1}) m.box({side*.71f,6.3f,0},{.02f,6.28f,2.78f},edge);
    // Sand piled against the foot, and two fragments knocked off long ago.
    m.taper({0,0,0},.9f,4.2f,3.0f,{116,120,104},6);
    m.box({4.6f,.5f,2.2f},{1.1f,.5f,.45f},slab);
    m.box({-3.8f,.4f,-3.4f},{.8f,.4f,.35f},slab);
    break;
   }
   case 16: { material_=0;
    // Still lit, still upright, still asking for coins - and built at a size that can be
    // made out from the cruise rather than at the size of the real thing.
    constexpr float S=3.6f;
    Color body{176,38,46},trim{226,226,230},dark3{28,30,34};
    m.box({0,.98f*S,0},{.56f*S,.98f*S,.38f*S},body);
    m.box({0,.06f*S,0},{.58f*S,.06f*S,.40f*S},dark3);
    m.box({0,1.96f*S,0},{.58f*S,.04f*S,.40f*S},trim);
    // The glass is just dark glass: only the tubes, the stock and the coin slot give
    // light, so the machine reads as lit from within rather than as a glowing slab.
    m.box({0,1.26f*S,.40f*S},{.44f*S,.52f*S,.02f*S},{26,32,40});
    material_=7;
    m.box({0,1.86f*S,.40f*S},{.50f*S,.10f*S,.03f*S},{236,204,132});
    for(int i=0;i<3;++i) for(int j=0;j<4;++j)
     m.box({(-.30f+j*.20f)*S,(.88f+i*.34f)*S,.42f*S},{.075f*S,.12f*S,.03f*S},
           (i+j)%3==0?Color{228,116,92}:(i+j)%3==1?Color{116,190,222}:Color{222,214,136});
    m.box({.40f*S,1.02f*S,.40f*S},{.07f*S,.03f*S,.02f*S},{140,224,176});   // coin slot glow
    material_=0;
    m.box({.40f*S,1.30f*S,.41f*S},{.10f*S,.30f*S,.02f*S},dark3);           // the buttons panel
    m.box({0,.42f*S,.41f*S},{.30f*S,.10f*S,.03f*S},dark3);               // the tray
    // Silt gathered round the base, and it has settled a little off true.
    m.taper({0,0,0},.26f*S,.86f*S,.62f*S,{120,126,112},7);
    break;
   }
  }
  // Partly buried fragments, not a repeated ring of identical boulders.
  material_=5;lighting_=env.light;
  for(int i=0;i<3;++i) {
   float x=(i-1)*5.f,z=-6-i*1.4f;
   Vec3 w=m.at({x,0,z});float y=surface(w.x,w.z)-p.y;
   m.box({x,y-.15f,z},{.6f+i*.35f,.35f+i*.12f,.8f},scale(stone,.8f));
  }
 }
 material_=0;lighting_=1;
}
}
