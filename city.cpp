#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include "scene.h"
namespace abyss {
world::Environment Scene::environment(Vec3 eye) const {
 if(!city) return world::environment(eye.x,eye.z);
 return {{9,23,51},{3,11,30},{117,150,165},{79,103,135}, .88f,CityVisibility,5};
}
void Scene::sinkGate(float metres) {
 if(city || gateOpen) return;
 metres=clampf(metres,0,20);float delta=metres-gateSink;gateSink=metres;
 for(int i=gateStart;i<gateEnd;++i) {triangles[i].a.y-=delta;triangles[i].b.y-=delta;triangles[i].c.y-=delta;}
 // X/Z membership stays unchanged. Expand the affected bounds before parallel rendering.
 for(auto& chunk:chunks) for(int j=0;j<chunk.count;++j) {
  int i=staticIndices[chunk.start+j];
  if(i>=gateStart && i<gateEnd) {chunk.minimum.y=std::min(chunk.minimum.y,-90.f);break;}
 }
}
bool Scene::buildCity(Triangle* storage,uint16_t* indices) {
 if(!storage || !indices) return false;
 city=true;triangles=storage;count=staticCount=solidCount=0;overflow=false;
 colonyCount=ventCount=0;ecosystem.count=0;whalePresent=false;
 lighting_=1.15f;material_=6;
 // Half of this place faces away from wherever the pilot is standing, and all of it used
 // to be drawn: the helpers here are the two-sided ones, so nothing carried the flag that
 // lets the renderer throw a face out before it projects it. Everything that follows is
 // built as a closed shape so that it can - a shape with a hole in it shows its inside
 // the moment the far side stops being drawn, which is why the shafts and the domes have
 // floors nobody will ever look at, and why beam caps both of its ends now.
 //
 // This marks a run of triangles as solid and turns any that are wound the wrong way, the
 // outward side of a face being the side away from the middle of whatever it belongs to.
 // It leaves the colour alone: add() has already shaded these as two-sided faces, lit
 // from either side, and keeping that is what makes this cost nothing to look at. The
 // city is lit exactly as it was; only the halves nobody can see stop being drawn.
 auto solidify=[&](int first,Vec3 middle) {
  for(int i=first;i<count;++i) {
   Triangle& t=triangles[i];
   Vec3 centre=(t.a+t.b+t.c)*(1.f/3);
   if(dot(cross(t.b-t.a,t.c-t.a),centre-middle)<0) { Vec3 swap=t.b; t.b=t.c; t.c=swap; }
   t.material=uint8_t(t.material|SolidFace);
  }
 };
 // Keep the common 8-metre terrain layout so collision and spatial lookup remain identical.
 for(int z=WorldMin;z<WorldMin+WorldSize;z+=8) for(int x=WorldMin;x<WorldMin+WorldSize;x+=8) {
#if defined(ARDUINO)
  if(x==WorldMin) delay(1);
#endif
  Vec3 a{float(x),floor(x,z),float(z)},b{float(x+8),floor(x+8,z),float(z)};
  Vec3 c{float(x+8),floor(x+8,z+8),float(z+8)},d{float(x),floor(x,z+8),float(z+8)};
  Color color=scale({88,119,142},.88f+noise(uint32_t(x*7919+z))*.22f);
  if(((x-z)/8)&1) {add(a,b,d,color);add(b,c,d,color);} else quad(a,b,c,d,color);
 }
 const Color stone{147,171,183},dark{75,100,129},cyan{73,237,237},gold{255,205,107},violet{167,131,255};
 auto glowBeam=[&](Vec3 a,Vec3 b,float width,Color color) {
  int first=count;beam(a,b,width,color);
  for(int i=first;i<count;++i) {triangles[i].material=7;triangles[i].color=color;}
  solidify(first,(a+b)*.5f);
 };
 auto prism=[&](Vec3 p,float radius,float height,int sides,Color color) {
  material_=5;
  int first=count;
  // The floor sits a little under the shape's own base rather than exactly on it: a
  // tower stands on a plinth whose lid is at that very height, and two surfaces in one
  // plane trade places from frame to frame.
  const Vec3 sunk{0,-.06f,0};
  for(int i=0;i<sides;++i) {
   float a=i*2*Pi/sides,b=(i+1)*2*Pi/sides;
   Vec3 u{std::cos(a)*radius,0,std::sin(a)*radius},v{std::cos(b)*radius,0,std::sin(b)*radius};
   quad(p+u,p+v,p+v+Vec3{0,height,0},p+u+Vec3{0,height,0},color);
   add(p+Vec3{0,height,0},p+u+Vec3{0,height,0},p+v+Vec3{0,height,0},color);
   add(p+sunk,p+v+sunk,p+u+sunk,color);
  }
  solidify(first,p+Vec3{0,height*.5f,0});
 };
 // Solid donut sections and luminous terrace edges, rather than wire rings.
 auto ring=[&](Vec3 p,float major,float tube,Color light) {
  auto point=[&](float a,float b) {return p+Vec3{std::cos(a)*(major+std::cos(b)*tube),std::sin(b)*tube*.65f,std::sin(a)*(major+std::cos(b)*tube)};};
  for(int i=0;i<12;++i) {
   float a=i*Pi/6,b=(i+1)*Pi/6;
   for(int j=0;j<4;++j) {
    float u=j*Pi/2,v=(j+1)*Pi/2;material_=0;
    quad(point(a,u),point(b,u),point(b,v),point(a,v),j==0?stone:dark);
   }
   // The rings are left as they were. A tube is a closed shape and its sections ought to
   // be safe to cull, but measuring says otherwise and the reason has not been found, so
   // they keep being drawn from both sides. They are a small part of the city.
   material_=7;float r=major+tube+.2f;
   Vec3 u{std::cos(a)*r,0,std::sin(a)*r},v{std::cos(b)*r,0,std::sin(b)*r};
   quad(p+u-Vec3{0,.3f,0},p+v-Vec3{0,.3f,0},p+v+Vec3{0,.3f,0},p+u+Vec3{0,.3f,0},light);
  }
 };
 auto dome=[&](Vec3 p,float radius,Color light) {
  int first=count;
  for(int level=0;level<3;++level) for(int i=0;i<10;++i) {
   float a=i*Pi/5,b=(i+1)*Pi/5,u=level*Pi/6,v=(level+1)*Pi/6;
   auto point=[&](float yaw,float angle) {return p+Vec3{std::cos(yaw)*radius*std::cos(angle),radius*.65f*std::sin(angle),std::sin(yaw)*radius*std::cos(angle)};};
   material_=0;quad(point(a,u),point(b,u),point(b,v),point(a,v),blend(stone,light,.3f));
  }
  material_=7;
  for(int i=0;i<12;++i) {
   float a=i*Pi/6,b=(i+1)*Pi/6;Vec3 u{std::cos(a)*(radius+.15f),0,std::sin(a)*(radius+.15f)},v{std::cos(b)*(radius+.15f),0,std::sin(b)*(radius+.15f)};
   quad(p+u,p+v,p+v+Vec3{0,.35f,0},p+u+Vec3{0,.35f,0},light);
   // A floor across the rim, sunk the same way the shafts' are and for the same reason.
   material_=0;
   add(p-Vec3{0,.06f,0},p+v-Vec3{0,.06f,0},p+u-Vec3{0,.06f,0},blend(stone,light,.3f));
   material_=7;
  }
  solidify(first,p);
 };
 auto tower=[&](float x,float z,float radius,float height,int seed,bool small) {
  Vec3 base{x,surface(x,z)-.6f,z};Color light=seed%3==0?gold:seed%3==1?cyan:violet;
  prism(base,radius+1.2f,1.5f,8,dark);
  prism(base+Vec3{0,1.5f,0},radius,height,8,blend(stone,{111,154,174},(seed%3)*.18f));
  for(int side=0;side<8;++side) {
   float a=(side+.5f)*Pi/4;Vec3 n{std::cos(a),0,std::sin(a)},t{-n.z,0,n.x};
   // On the wall, not a hand's breadth clear of it. Standing proud made no difference
   // while every face was drawn, but a lit pane that reaches past the shaft shows its
   // back around the silhouette once the far side of the tower stops being drawn.
   Vec3 wall=base+n*(radius*.924f+.05f);material_=7;
   int panes=count;
   for(int level=0;level<int(height/3);++level) for(int window=0;window<(small?1:2);++window) {
    if((side*3+level+window+seed)%11==0) continue;
    Vec3 c=wall+Vec3{0,3.0f+level*2.8f,0}+t*(small?0.f:(window?1.f:-1.f)*radius*.18f);
    float width=radius*(small?.24f:.11f);
    quad(c-t*width-Vec3{0,.55f,0},c+t*width-Vec3{0,.55f,0},c+t*width+Vec3{0,.55f,0},c-t*width+Vec3{0,.55f,0},light);
   }
   solidify(panes,base+Vec3{0,height*.5f,0});
  }
  Vec3 crown=base+Vec3{0,height+1.5f,0};
  if(small || seed%3==0) dome(crown,radius*1.05f,light);
  else {
   ring(crown-Vec3{0,2,0},radius*1.85f,radius*.43f,light);
   dome(crown,radius*.65f,light);
   if(seed%3==2) ring(base+Vec3{0,height*.48f,0},radius*1.55f,radius*.36f,light);
  }
 };
 // The city, row by row up the bowl.
 //
 // A row stands on one terrace, which is a curve of constant q rather than a line of
 // constant z, so the rows wrap round the arrival instead of running straight across in
 // front of it. Each row is shorter than the one below it and carries fewer buildings,
 // which is how a hillside town is built and is what gives the place a silhouette; four
 // shelves of identical length were a flight of steps with houses on.
 const float RowQ[4]={2,2+CityTier,2+2*CityTier,2+3*CityTier};
 const float RowHalf[4]={70,58,44,28};
 const int RowTowers[4]={8,7,5,4};
 for(int row=0;row<4;++row) {
  const float half=RowHalf[row],q=RowQ[row];
  const int wide=RowTowers[row];
  for(int column=0;column<wide;++column) {
   uint32_t seed=420+row*31+column*7;
   float x=((column+.5f)/wide-.5f)*2*half+(noise(seed)-.5f)*6;
   float z=cityRowZ(x,q+(noise(seed+1)-.5f)*6);
   float radius=3.0f+noise(seed+2)*2.5f;
   float height=9+noise(seed+3)*24;
   tower(x,z,radius,height,row*8+column,false);
  }
  for(int column=0;column+1<wide;++column) {
   uint32_t seed=750+row*43+column*11;
   float x=((column+1.f)/wide-.5f)*2*half+(noise(seed)-.5f)*5;
   float z=cityRowZ(x,q+10+(noise(seed+1)-.5f)*4);
   tower(x,z,2+noise(seed+2)*2.4f,3+noise(seed+3)*7,100+row*7+column,true);
  }
  // Broad cantilever terraces follow the lip of the shelf, with luminous fascia. The lip
  // is a curve like everything else here, so each section is cut between two points on it
  // rather than laid along a straight line that would leave the terrace behind.
  const float edge=half+8,lip=q-7;
  const int sections=std::max(8,int(edge/7));
  for(int section=0;section<sections;++section) {
   float x=-edge+section*(2*edge/sections),nx=x+2*edge/sections;
   Vec3 a{x,surface(x,cityRowZ(x,lip))+1.1f,cityRowZ(x,lip)};
   Vec3 b{nx,surface(nx,cityRowZ(nx,lip))+1.1f,cityRowZ(nx,lip)};
   material_=5;quad(a-Vec3{0,0,4},b-Vec3{0,0,4},b+Vec3{0,0,2},a+Vec3{0,0,2},stone);
   material_=7;quad(a-Vec3{0,.5f,4.15f},b-Vec3{0,.5f,4.15f},b-Vec3{0,0,4.15f},a-Vec3{0,0,4.15f},row%2?cyan:gold);
   material_=0;quad(a-Vec3{0,2,4},b-Vec3{0,2,4},b-Vec3{0,.5f,4},a-Vec3{0,.5f,4},dark);
  }
 }
 // A clustered central landmark rises above the surrounding neighbourhoods.
 for(int i=0;i<5;++i) {
  float a=i*2*Pi/5,x=std::cos(a)*13;
  tower(x,cityRowZ(x,RowQ[1]+24+std::sin(a)*10),3.8f+(i%2)*1.4f,30+(i%3)*9,201+i,false);
 }
 tower(0,cityRowZ(0,RowQ[1]+24),7,62,209,false);
 // Service pylons and rising light rails stand against the walls of the hollow, where the
 // terraces give out and the rock takes over, and climb with them.
 for(int side:{-1,1}) for(int row=0;row<3;++row) {
  float x=side*(RowHalf[row]+9);
  float za=cityRowZ(x,RowQ[row]-6),zb=cityRowZ(x,RowQ[row]+22);
  Vec3 a{x,surface(x,za)+4,za},b{x,surface(x,zb)+4,zb};
  beam(a,b,.32f,dark);glowBeam(a+Vec3{0,.45f,0},b+Vec3{0,.45f,0},.09f,cyan);
 }
 // Return well: an open shaft framed by four illuminated pylons, with no invisible wall.
 Vec3 portal{-12,floor(-12,-86),-86};
 for(int side:{-1,1}) for(int end:{-1,1}) {
  Vec3 p=portal+Vec3{side*8.f,0,end*8.f};
  beam(p,{p.x,-165,p.z},.48f,stone);
  glowBeam(p+Vec3{-side*.55f,0,-end*.55f},{p.x-side*.55f,-165,p.z-end*.55f},.12f,cyan);
 }
 for(int side:{-1,1}) {
  glowBeam({-20,-165,-86+side*8.f},{-4,-165,-86+side*8.f},.22f,gold);
  glowBeam({-12+side*8.f,-165,-94},{-12+side*8.f,-165,-78},.22f,gold);
 }
 solidCount=count;
 // Non-solid gold guidance: chevrons lead out of the city, then climb the return shaft.
 for(int step=0;step<9;++step) {
  float z=-18-step*7.5f;
  float y=-209+(step/8.f)*5;
  Vec3 tip{-12,y,z-2.4f};
  glowBeam(tip+Vec3{-2.2f,0,3.4f},tip,.18f,gold);
  glowBeam(tip+Vec3{2.2f,0,3.4f},tip,.18f,gold);
 }
 for(int level=0;level<7;++level) {
  float y=-206+level*6.f;
  for(int side:{-1,1}) {
   Vec3 tip{-12+side*6.f,y+2,-86};
   glowBeam(tip+Vec3{0,-3,-2},tip,.2f,gold);
   glowBeam(tip+Vec3{0,-3,2},tip,.2f,gold);
  }
 }
 // Wide luminous exit crown, clear through its centre.
 ring({-12,-169,-86},10, .55f,gold);
 // Garden beds of branching luminous polyps, not solid obstacles.
 for(int i=0;i<48;++i) {
  uint32_t seed=91003+i*37;float a=noise(seed)*Pi*2;
  const int bed=i%4;
  Vec3 p{(noise(seed+5)-.5f)*2*RowHalf[bed],0,0};
  p.z=cityRowZ(p.x,RowQ[bed]-5);p.y=surface(p.x,p.z)+1.3f;
  float h=1+noise(seed+2)*3;Color color=i%3==0?violet:cyan;
  for(int branch=0;branch<3;++branch) {
   float yaw=a+branch*2*Pi/3;
   Vec3 tip=p+Vec3{std::cos(yaw)*h*.45f,h*(.7f+branch*.1f),std::sin(yaw)*h*.45f};
   glowBeam(p,tip,.035f,color);material_=7;
   Vec3 up{0,.35f,0},side{.24f,0,0},front{0,0,.24f};
   add(tip+up,tip+side,tip+front,color);add(tip+up,tip+front,tip-side,color);
   add(tip+up,tip-side,tip-front,color);add(tip+up,tip-front,tip+side,color);
  }
 }
 material_=0;lighting_=1;staticCount=count;buildIndex(indices);
 return !overflow;
}
Camera Scene::cityTour(float time) const {
 // The cruise runs round the inside of the bowl: out across the floor at the front, up one
 // wall, along the top of the terraces and down the other, looking in at the city the
 // whole way. Every node is a place on a terrace and a height above it rather than a point
 // in the water, so the route follows the ground plan wherever that is tuned to and can
 // never end up inside the walls of the hollow.
 struct Node {float x,q,above,tx,tq,tabove;};
 static const Node route[]={
  {-60,-26,32,  -8, 12,14}, { 60,-26,32,   8, 12,14},
  { 64, 16,28,  12, 46,12}, { 54, 50,28,   6, 78,10},
  { 30, 92,26,  -4, 64,10}, {-30, 92,26,   4, 64,10},
  {-54, 50,28,  -6, 78,10}, {-64, 16,28, -12, 46,12}
 };
 auto place=[&](float x,float q,float above) {
  float z=cityRowZ(x,q);return Vec3{x,floor(x,z)+above,z};
 };
 auto eyeOf=[&](const Node& n) {return place(n.x,n.q,n.above);};
 auto aimOf=[&](const Node& n) {return place(n.tx,n.tq,n.tabove);};
 Vec3 p,target;float t=std::max(0.f,time);
 if(t<18) {
  float q=t/18;q=q*q*(3-2*q);
  p=mix(place(-12,cityQ(-12,-44),38),eyeOf(route[0]),q);
  target=mix(place(0,24,7),aimOf(route[0]),q);
 } else {
  float q=std::fmod(t-18,240.f)/30;int i=int(q);q-=i;q=q*q*(3-2*q);
  p=mix(eyeOf(route[i]),eyeOf(route[(i+1)%8]),q);
  target=mix(aimOf(route[i]),aimOf(route[(i+1)%8]),q);
 }
 Camera c;c.lookAt(p,target);return c;
}
void Scene::animateCity(float time,const Camera& camera) {
 ecosystem.count=0;lighting_=1.15f;cityMachineCount=0;
 for(int i=0;i<CityMachineCount;++i){
  MachinePose p=cityMachinePose(i,time);Vec3 v=camera.view(p.position);
  cityMachines[cityMachineCount++]=p;
  if(v.z>-p.length && v.z<80 && std::abs(v.x)<v.z*.8f+p.length && std::abs(v.y)<v.z*.7f+p.length)machineMesh(p);
 }
 ecosystem.largeGroups(*this,camera,time);
 for(int i=0;i<ecosystem.count;++i) animalMesh(ecosystem.animals[i]);
 auto animal=[&](Vec3 p,float yaw,float size,ecology::Species species,uint32_t id) {
  if(length(p-camera.position)>65 || ecosystem.count>=ecology::Ecosystem::MaxAnimals) return;
  ecology::Animal a{p,yaw,size,time*2+id*.37f,1,species,id};
  ecosystem.animals[ecosystem.count++]=a;animalMesh(a);
 };
 for(int school=0;school<4;++school) for(int j=0;j<12;++j) {
  float a=time*.065f+school*Pi*.5f,r=48+std::sin(j*1.7f)*4;
  Vec3 p{std::sin(a+j*.012f)*r,-208+std::sin(j*2.1f)*2,-42+std::cos(a+j*.012f)*10};
  animal(p,Pi*.5f+a,.22f+(j%4)*.06f,ecology::Species::Lanternfish,school*12+j);
 }
 for(int i=0;i<12;++i) {
  float a=i*Pi/6+time*.012f;Vec3 p{std::sin(a)*47,-198+std::sin(time*.3f+i)*3,-30+std::cos(a)*12};
  animal(p,-a,.7f,ecology::Species::CombJelly,80+i);
 }
 for(int i=0;i<2;++i) {
  float a=time*.022f+i*Pi;Vec3 p{std::sin(a)*70,-190+i*3.f,-36+std::cos(a)*16};
  animal(p,a+Pi*.5f,2.8f,ecology::Species::Ray,100+i);
 }
 // The Alcyone and her boats, at the far end of the approach.
 //
 // The chevrons lead out of the city and down the shaft; keep going past the shaft and
 // the carrier is lying there with four submarines around her. It is a display and
 // nothing else - nothing drives it, nothing records it, it is not in the catalogue and
 // the pilot is never told it is there. Finding it is the whole of it.
 //
 // Between them they are heavier than the whole city, so they are built only while they
 // are in shot, and the display is given a budget of its own which it spends nearest
 // first. The meshes have one dial between them - how far away they are being looked
 // from - so once the budget is gone the rest are told they are a long way off and come
 // out coarse. Whichever one the pilot is actually looking at is the one built in full.
 {
  // She lies across the end of the channel rather than pointing down it, so the first
  // thing seen from the city is her length. The boats are moored two to a side, on her
  // heading and a little below her, the way they would be if they had just come in.
  constexpr float Heading=.40f;
  const Vec3 anchorage{0,-204,-280};
  const Vec3 along{std::sin(Heading),0,std::cos(Heading)};
  const Vec3 beam{std::cos(Heading),0,-std::sin(Heading)};
  struct Berth { Vec3 p; float yaw,length; };
  Berth moored[5]={{anchorage,Heading,46.f}};
  for(int i=0;i<4;++i) {
   const float side=(i&1)?1.f:-1.f,fore=(i&2)?-1.f:1.f;
   moored[i+1]={anchorage+beam*(side*32)+along*(fore*18)-Vec3{0,4,0},Heading,13.f};
  }
  constexpr int Moored=5;
  // How far this sea is clear, asked of the sea rather than of a constant: the Tab5 sees
  // three times as far into the city as the board this started on.
  const float clear=environment(camera.position).visibility;
  const Vec3 eye=viewer_;
  const int ceiling=staticCount+(MaxTriangles-staticCount)*2/5;
  bool built[Moored]={};
  for(int n=0;n<Moored;++n) {
   int pick=-1; float nearest=0;
   for(int i=0;i<Moored;++i) {
    if(built[i]) continue;
    const float d=length(moored[i].p-eye);
    if(pick<0 || d<nearest) { pick=i; nearest=d; }
   }
   built[pick]=true;
   const Berth& b=moored[pick];
   const Vec3 p=b.p;
   const Vec3 view=camera.view(p);
   if(view.z<-b.length || view.z>clear) continue;
   if(std::abs(view.x)>view.z*.9f+b.length || std::abs(view.y)>view.z*.8f+b.length) continue;
   viewer_=count>ceiling ? p+unit(eye-p+Vec3{0,.001f,0})*(b.length*20) : eye;
   const MachinePose pose{p,b.yaw,b.length,0,MachineKind::ArmoredFish};
   if(b.length>20) alcyoneMesh(pose); else submarineMesh(pose);
  }
  viewer_=eye;
 }
 material_=0;lighting_=1;
}
}
