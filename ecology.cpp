#include "ecology.h"
#include "scene.h"
#if defined(__GNUC__)
#pragma GCC optimize("O3", "fast-math")
#endif
namespace abyss { namespace ecology {
const char* name(Species s) {
  const char* names[]={"small silver school","reef fish","horse mackerel","barracuda","goby","flatfish","ray","crab","turtle","lanternfish","eel","butterflyfish","surgeonfish","garden eel","goatfish","rockfish","sculpin","viperfish","squid","Anomalocaris","Sacabambaspis","Dunkleosteus","plesiosaur","trilobite","Opabinia","comb jelly",
    "tuna","salmon","sea bream","marlin","Spanish mackerel","cod","mackerel",
    "ocean sunfish","anglerfish","oarfish","Atka mackerel","herring","saury","yellowtail","pufferfish","scalloped hammerhead","bottlenose dolphin","orca","great white shark","giant manta","whale shark","chambered nautilus","coelacanth"};
  return names[int(s)];
}
float nominalLength(Species s) {
  const float lengths[]={.26f,.22f,.62f,1.35f,.18f,.38f,2.40f,.27f,.90f,.16f,.85f,.20f,.28f,.40f,.32f,.45f,.23f,.55f,.65f,.60f,.32f,6.0f,9.5f,.22f,.18f,.36f,
    1.90f,.85f,.50f,3.20f,.95f,.90f,.35f,
    1.80f,.40f,3.40f,.40f,.30f,.35f,.90f,.45f,3.2f,2.8f,6.8f,4.5f,3.2f,9.0f,.38f,1.6f};
  return lengths[int(s)];
}
bool bottomDweller(Species s) {
  return s==Species::Goby || s==Species::Flatfish || s==Species::Ray || s==Species::Crab || s==Species::Eel || s==Species::GardenEel || s==Species::Goatfish || s==Species::Rockfish || s==Species::Sculpin || s==Species::Trilobite || s==Species::Opabinia;
}
bool ancient(Species s) { return (s>=Species::Anomalocaris && s<=Species::Opabinia) || s==Species::Nautilus || s==Species::Coelacanth; }
uint8_t bands(Species s) {
  // Sprat/reef/jack/barracuda/goby/flatfish/ray/crab/turtle/lanternfish/eel/butterfly/
  // surgeon/gardeneel/goatfish/rockfish/sculpin/viper/squid/anomalo/sacab/dunkle/plesio/
  // trilobite/opabinia/combjelly
  constexpr uint8_t table[]={
    BandUpper|BandMiddle|BandLower, BandMiddle|BandLower|BandSeabed, BandUpper|BandMiddle|BandLower,
    BandUpper|BandMiddle|BandLower,
    BandSeabed, BandSeabed, BandSeabed, BandSeabed, BandUpper|BandMiddle,
    BandLower, BandSeabed, BandMiddle|BandLower, BandUpper|BandMiddle|BandLower,
    BandSeabed, BandSeabed, BandSeabed, BandSeabed,
    BandLower, BandMiddle|BandLower, BandLower|BandSeabed, BandLower|BandSeabed,
    BandMiddle|BandLower, BandUpper|BandMiddle|BandLower, BandSeabed, BandSeabed,
    BandUpper|BandMiddle|BandLower,
    BandUpper|BandMiddle|BandLower,          // tuna
    BandUpper|BandMiddle,                    // salmon
    BandMiddle|BandLower,                    // sea bream
    BandUpper|BandMiddle,                    // marlin
    BandUpper|BandMiddle|BandLower,          // Spanish mackerel
    BandLower|BandSeabed,                    // cod
    BandUpper|BandMiddle|BandLower,          // mackerel
    BandUpper|BandMiddle,                    // ocean sunfish
    BandLower|BandSeabed,                    // anglerfish
    BandMiddle|BandLower,                    // oarfish
    BandLower|BandSeabed,                    // Atka mackerel
    BandUpper|BandMiddle|BandLower,          // herring
    BandUpper,                               // saury
    BandUpper|BandMiddle,                    // yellowtail
    BandLower|BandSeabed, BandMiddle, BandUpper|BandMiddle, BandMiddle, BandMiddle|BandLower, BandMiddle, BandUpper|BandMiddle, BandLower, BandLower};                   // pufferfish
  return int(s)<int(Species::Count)?table[int(s)]:uint8_t(BandMiddle);
}
float bandHeight(Species s,float bed,uint32_t seed) {
  // The bands are measured over the thirty metres above the bed, not the whole column:
  // in a seventy metre trench an "upper" school would otherwise be out of sight forever.
  float top=std::min(-2.5f,bed+30.0f);
  if(top<=bed+1.5f) return bed+1.0f;
  uint8_t mask=bands(s);
  // Pick one of the bands this kind uses, then a height inside it.
  int choices[4],count=0;
  for(int i=0;i<4;++i) if(mask&(1<<i)) choices[count++]=i;
  if(!count) return bed+2.0f;
  int band=choices[hash(seed)%unsigned(count)];
  float span=top-bed;
  float low[4]={bed+span*.66f,bed+span*.33f,bed+span*.08f,bed};
  float high[4]={top,bed+span*.70f,bed+span*.38f,bed+span*.10f};
  float t=noise(seed+41);
  return clampf(low[band]+(high[band]-low[band])*t,bed+.8f,top);
}
bool allowedInZone(Species s,int zone) {
  // Explicit ranges: even common living animals never enter the primordial basin.
  // Bit per zone: garden, dunes, labyrinth, primordial, rift. Each area has its own cast.
  constexpr uint8_t masks[]={7,3,6,3,7,2,2,23,3,16,20,3,3,2,2,4,4,16,16,8,8,8,8,8,8,16,
    2,3,1,2,6,4,6,
    2,16,16,4,4,2,6,1,16,1,32,4,2,2,8,8};
  return int(s)<int(Species::Count) && zone>=0 && zone<6 && (masks[int(s)]&(1<<zone));
}
namespace {
// Every basin gets its own cast, six deep so a stretch of water is not all one fish.
constexpr int SchoolChoices=8;
const Species SchoolTable[5][SchoolChoices]={
  {Species::ReefFish,Species::Butterflyfish,Species::Surgeonfish,Species::SeaBream,
   Species::Sprat,Species::Pufferfish,Species::ReefFish,Species::SeaBream},
  {Species::Sprat,Species::Jack,Species::Tuna,Species::Salmon,
   Species::Marlin,Species::Mackerel,Species::Sunfish,Species::Saury},
  {Species::Jack,Species::Sprat,Species::SpanishMackerel,Species::Cod,
   Species::Mackerel,Species::AtkaMackerel,Species::Herring,Species::Yellowtail},
  {Species::Sacabambaspis,Species::Anomalocaris,Species::Sacabambaspis,Species::Anomalocaris,
   Species::Nautilus,Species::Coelacanth,Species::Sacabambaspis,Species::Nautilus},
  {Species::Lanternfish,Species::Squid,Species::CombJelly,Species::Viperfish,
   Species::Anglerfish,Species::Oarfish,Species::Lanternfish,Species::Squid}};
const Species ResidentTable[5][4]={
  {Species::Goby,Species::Crab,Species::Goby,Species::Crab},
  {Species::Flatfish,Species::Ray,Species::GardenEel,Species::Goatfish},
  {Species::Eel,Species::Rockfish,Species::Sculpin,Species::Goby},
  {Species::Trilobite,Species::Opabinia,Species::Trilobite,Species::Opabinia},
  {Species::Eel,Species::Crab,Species::Eel,Species::Crab}};
}
// Habitats made up on the spot for the ground around the submarine. The stored list can
// only be so dense; these guarantee that open water is never empty, wherever the pilot goes.
void Ecosystem::nearbyHabitats(const Scene& scene,const Camera& cam) {
  constexpr float Cell=26.0f;
  int cx=int(std::floor(cam.position.x/Cell)),cz=int(std::floor(cam.position.z/Cell));
  int n=0;
  for(int dz=-1;dz<=1;++dz) for(int dx=-1;dx<=1;++dx) {
    auto& h=local[n++];
    int gx=cx+dx,gz=cz+dz;
    h.seed=hash(uint32_t(gx)*73856093u ^ uint32_t(gz)*19349663u ^ 0x5bd17u);
    float x=clampf((gx+.18f+noise(h.seed)*.64f)*Cell,WorldMin+8.f,WorldMin+WorldSize-8.f);
    float z=clampf((gz+.18f+noise(h.seed+1)*.64f)*Cell,WorldMin+8.f,WorldMin+WorldSize-8.f);
    float bed=scene.surface(x,z);
    h.bottom={x,bed,z};
    auto env=world::environment(x,z);
    h.zone=uint8_t(env.zone); h.light=env.light;
    h.heading=noise(h.seed+3)*Pi*2;
    h.school=SchoolTable[h.zone][h.seed%SchoolChoices];
    if(h.zone==1 && world::coralGarden(x,z)>.3f) {
      const Species reef[]={Species::ReefFish,Species::Butterflyfish,Species::Surgeonfish};
      h.school=reef[h.seed%3];
    }
    h.resident=ResidentTable[h.zone][(h.seed>>5)%4];
    h.water={x,h.zone==1 && world::coralGarden(x,z)>.3f?bed+2.4f:bandHeight(h.school,bed,h.seed+2),z};
  }
}
void Ecosystem::build(const Scene& scene) {
  for(int i=0;i<HabitatCount;++i) {
    auto& h=habitats[i]; h.seed=9317+i*101;
    // Two thirds hug the tour, the rest are scattered over open water: hand flying leaves
    // the route, and empty basins were the result.
    Camera cam;
    constexpr int OnRoute=HabitatCount/2;
    if(i<OnRoute) cam=scene.tour(i*TourSeconds/OnRoute);
    else {
      float x=WorldMin+18+noise(h.seed+7)*(WorldSize-36);
      float z=WorldMin+18+noise(h.seed+8)*(WorldSize-36);
      float ground=scene.surface(x,z);
      Vec3 eye{x,ground+4+noise(h.seed+9)*6,z};
      float yaw=noise(h.seed+10)*Pi*2;
      cam.lookAt(eye,eye+Vec3{std::sin(yaw),-.1f,std::cos(yaw)});
    }
    // Sit the school on the anchor rather than well ahead of it: pushed forward, the
    // nearest school was always eighteen metres off and read as a few specks.
    h.water=cam.position+cam.forward*(2+noise(h.seed)*5)+cam.right*((noise(h.seed+1)-.5f)*9);
    // Spread the schools through the water column. Anchoring them to the tour camera put
    // every one of them at the same altitude above the bed.
    // Schools used to inherit the tour camera's own altitude, which is why every one of
    // them sat at the same height. Scatter them through the band a diver occupies instead.
    float bed=scene.surface(h.water.x,h.water.z);
    h.bottom=cam.position+cam.forward*(15+noise(h.seed+2)*8)+cam.right*((i%2?1:-1)*(3+noise(h.seed+3)*5));
    for(int trial=0;trial<8;++trial) {
      float base=scene.surface(h.bottom.x,h.bottom.z);
      float obstruction=scene.surface(h.bottom.x,h.bottom.z,true)-base;
      float slope=std::abs(scene.surface(h.bottom.x+2,h.bottom.z)-scene.surface(h.bottom.x-2,h.bottom.z))+
                  std::abs(scene.surface(h.bottom.x,h.bottom.z+2)-scene.surface(h.bottom.x,h.bottom.z-2));
      if(obstruction<.3f && slope<2.5f) break;
      h.bottom=h.bottom+cam.right*(trial%2?-(trial+1)*1.5f:(trial+1)*1.5f);
    }
    h.bottom.y=scene.surface(h.bottom.x,h.bottom.z);
    h.heading=std::atan2(cam.right.x,cam.right.z);
    auto env=world::environment(h.water.x,h.water.z); h.zone=uint8_t(env.zone); h.light=env.light;
    h.school=SchoolTable[h.zone][i%SchoolChoices]; h.resident=ResidentTable[h.zone][i%4];
    if(h.zone==1 && world::coralGarden(h.water.x,h.water.z)>.3f) {
      const Species reef[]={Species::ReefFish,Species::Butterflyfish,Species::Surgeonfish};
      h.school=reef[h.seed%3];
    }
    h.water.y=bandHeight(h.school,bed,h.seed+11);
    // Reef residents and grazing turtles use actual colonies, not just a biome label.
    bool grazer=h.zone<2 && i%26==14;
    if(h.resident==Species::Goby || (h.school==Species::ReefFish || h.school==Species::Butterflyfish || h.school==Species::Surgeonfish) || grazer) {
      const Scene::Colony* closest=nullptr; float best=24*24;
      for(int j=0;j<scene.colonyCount;++j) {
        const auto& colony=scene.colonies[j];
        if(grazer && colony.type!=0) continue;
        if(colony.type==3) continue;
        if(h.zone==1 && colony.type<6 && world::coralGarden(h.water.x,h.water.z)>.3f)continue;
        Vec3 d=colony.position-h.bottom; d.y=0;
        float distance=dot(d,d);
        if(distance<best) { closest=&colony; best=distance; }
      }
      if(closest) {
        h.bottom=closest->position+Vec3{1.2f,0,.8f};
        h.bottom.y=scene.surface(h.bottom.x,h.bottom.z);
        if((h.school==Species::ReefFish || h.school==Species::Butterflyfish || h.school==Species::Surgeonfish)) {
          h.water=closest->position+Vec3{0,closest->height*.75f+1.0f,0};
        }
      }
    }
  }
  int reefAnchor=0;
  for(int c=0;c<scene.colonyCount && reefAnchor<12;++c) {
    const auto& colony=scene.colonies[c];if(colony.type<6)continue;
    if(c%5!=0)continue;
    auto& h=habitats[HabitatCount-1-reefAnchor];
    h.seed=153301+reefAnchor*73;h.zone=1;h.light=1.28f;
    const Species reef[]={Species::ReefFish,Species::Butterflyfish,Species::Surgeonfish};
    h.school=reef[reefAnchor%3];h.resident=Species::Goby;
    h.bottom=colony.position;h.water=colony.position+Vec3{0,colony.height+1.2f,0};
    h.heading=noise(h.seed)*2*Pi;++reefAnchor;
  }
  buildRelicShelters(scene);
}
void Ecosystem::update(const Scene& scene,const Camera& cam,float time) {
  count=0;
  largeGroups(scene,cam,time);
  nearbyHabitats(scene,cam);
  const int cameraZone=world::environment(cam.position.x,cam.position.z).zone;
  if(cameraZone==3)restingCoelacanths(scene,cam,time);
  auto at=[&](int i)->const Habitat& { return i<HabitatCount?habitats[i]:local[i-HabitatCount]; };
  // What names a habitat. The stored ones are named by their place in the list, which
  // never moves. The nine made up around the submarine are refilled from a grid that
  // follows the pilot, so their slot changes every time a cell boundary is crossed - and
  // everything drawn from that slot changed with it: how many fish are in the school,
  // which way it faces, whether a predator is working its edge, what colour each fish is.
  // Naming them by the cell they sit in instead holds all of it still while the pilot
  // swims up to them. The stored habitats keep the numbers they always had.
  auto identity=[&](int i) { return i<HabitatCount?i:int(HabitatCount+at(i).seed%4096u); };
  // Nearest visible habitats have priority; no allocation or frame-dependent state.
  // The pool grew with the habitat count: scattered sites must not crowd out the nearby ones.
  constexpr int Pool=20;
  int selected[Pool]; float distances[Pool]; int selectedCount=0;
  for(int i=0;i<HabitatCount+LocalCount;++i) {
    Vec3 a=cam.view(at(i).water),b=cam.view(at(i).bottom);
    auto visible=[](Vec3 v) { return v.z>-5 && v.z<48 && std::abs(v.x)<v.z*.7f+9 && std::abs(v.y)<v.z*.6f+9; };
    if(!visible(a) && !visible(b)) continue;
    float d=std::min(dot(a,a),dot(b,b));
    int j=selectedCount;
    if(j==Pool) { if(d>=distances[Pool-1]) continue; j=Pool-1; } else ++selectedCount;
    while(j>0 && d<distances[j-1]) { selected[j]=selected[j-1]; distances[j]=distances[j-1]; --j; }
    selected[j]=i; distances[j]=d;
  }
  auto emit=[&](Animal animal) {
    if(count>=MaxAnimals || !allowedInZone(animal.species,cameraZone)) return;
    Vec3 relative=animal.position-cam.position;
    // Only push far enough away to keep the camera out of the animal. It used to hold
    // everything at a metre and a half, which meant you could never get close enough to
    // a small fish to tell one from another.
    float distance=length(relative),safe=std::max(.60f,animal.length*.85f);
    if(animal.species==Species::Nautilus)animal.activity=nautilusExtension(std::max(distance,safe));
    // Keep real body dimensions; a nearby submarine causes avoidance instead of enlarged fish.
    if(animal.species==Species::GardenEel) {
      // Burrows stay anchored: never push a sand-dweller away from the submarine.
      animal.position.y=scene.surface(animal.position.x,animal.position.z);
      animal.activity=gardenEelExtension(length(animal.position-cam.position),animal.id);
      if(animal.activity<=.001f)return; // concealed animals cannot be discovered
    }
    else if(distance<safe) animal.position=cam.position+unit(relative)*safe;
    if(!allowedInZone(animal.species,world::environment(animal.position.x,animal.position.z).zone)) return;
    Vec3 view=cam.view(animal.position);
    if(view.z<.28f || view.z>45 || std::abs(view.x)>view.z*.68f+animal.length || std::abs(view.y)>view.z*.53f+animal.length) return;
    float ground=scene.surface(animal.position.x,animal.position.z);
    if(bottomDweller(animal.species)) {
      float clearance=animal.species==Species::Ray?.20f:animal.species==Species::Eel?.12f:animal.species==Species::GardenEel?0.f:.045f;
      animal.position.y=ground+clearance+animal.activity*(animal.species==Species::Flatfish?.18f:animal.species==Species::Goby?.25f:0);
    } else if(animal.position.y<ground+animal.length*.3f) return;
    animals[count++]=animal;
  };
  int schools=0;
  for(int n=0;n<selectedCount && schools<9;++n) {
    const auto& h=at(selected[n]); const int index=identity(selected[n]);
    // Schools sit at varied depths now, so the gate that admits them has to be wider or
    // the ones above and below the submarine never appear.
    // The gate never looked sideways, so a school just off the edge of the screen could
    // take one of the few slots and nothing visible came of it.
    Vec3 view=cam.view(h.water);
    // A school is several metres across, so its centre passing the camera must not take
    // the whole school with it: swimming into a shoal should put you inside it, not
    // empty the water. Individual fish are still checked against the frustum below.
    if(view.z<-9 || view.z>40) continue;
    if(std::abs(view.x)>std::abs(view.z)*.62f+7 || std::abs(view.y)>std::abs(view.z)*.50f+7) continue;
    if(!allowedInZone(h.school,cameraZone)) continue;
    ++schools;
    bool reef=(h.school==Species::ReefFish || h.school==Species::Butterflyfish || h.school==Species::Surgeonfish || h.school==Species::SeaBream);
    bool medium=h.school==Species::Jack || h.school==Species::Tuna || h.school==Species::Marlin ||
                h.school==Species::Salmon || h.school==Species::SpanishMackerel || h.school==Species::Cod ||
                h.school==Species::Sunfish || h.school==Species::Oarfish || h.school==Species::Yellowtail;
    int population=medium?16+index%10:reef?26+index%12:64+index%36;
    if(h.school==Species::Sacabambaspis) population=22+index%14;
    if(h.school==Species::Anomalocaris || h.school==Species::Squid) population=10+index%8;
    if(h.school==Species::Viperfish) population=6;
    if(h.school==Species::CombJelly) population=16;
    if(h.school==Species::Tuna) population=7+index%6;
    if(h.school==Species::Marlin) population=2+index%2;
    if(h.school==Species::Salmon) population=22+index%16;
    if(h.school==Species::SeaBream) population=12+index%8;
    if(h.school==Species::SpanishMackerel) population=14+index%10;
    if(h.school==Species::Cod) population=9+index%6;
    if(h.school==Species::Mackerel) population=48+index%30;
    if(h.school==Species::Sunfish) population=2+index%2;
    if(h.school==Species::Anglerfish) population=3;
    if(h.school==Species::Oarfish) population=2;
    if(h.school==Species::AtkaMackerel) population=18+index%12;
    if(h.school==Species::Herring) population=70+index%40;
    if(h.school==Species::Saury) population=40+index%26;
    if(h.school==Species::Yellowtail) population=9+index%7;
    if(h.school==Species::Pufferfish) population=5+index%4;
    bool relic=h.school==Species::Nautilus || h.school==Species::Coelacanth;
    if(relic) population=h.school==Species::Nautilus?3:2;
    float angle=time*(relic?.022f:h.school==Species::CombJelly?.035f:medium?.105f:.14f)+index*1.71f;
    Vec3 center=h.water+Vec3{std::cos(angle)*2.1f,std::sin(angle*.7f)*.35f,std::sin(angle)*2.1f};
    float heading=std::atan2(-std::sin(angle),std::cos(angle));
    // A sparse predator follows the edge of a forage school; the school contracts and turns away.
    bool predator=index%22==8 && h.zone<2;
    float huntingRadius=3.0f+4.0f*(.5f+.5f*std::sin(time*.21f+index));
    Vec3 hunter=center+Vec3{std::cos(time*.21f+index)*huntingRadius,-.6f,std::sin(time*.21f+index)*huntingRadius};
    float alarm=predator?clampf((6-huntingRadius)/3,0,1):0;
    Vec3 escape=predator?unit(center-hunter)*alarm*1.4f:Vec3{};
    for(int j=0;j<population;++j) {
      uint32_t seed=h.seed+j*31;
      float a=j*2.399963f,r=std::sqrt((j+.5f)/population)*(relic?3.0f:medium?5.0f:reef?2.6f:4.4f)*(1-alarm*.35f);
      // Open water schools are a loose cloud, not a plate; reef fish stay tight to the coral.
      Vec3 offset{std::cos(a)*r,(noise(seed)-.5f)*(reef?1.4f:2.6f)+std::sin(time*.5f+j*1.7f)*.22f,std::sin(a)*r*.65f};
      float peck=reef?std::pow(std::max(0.0f,std::sin(time*.8f+j)),8.0f):0;
      Vec3 p=center+rotateY(offset,heading)+escape;
      p.y-=peck*.3f;
      if(relic) p.y=scene.surface(p.x,p.z)+1.6f+(h.school==Species::Nautilus?.35f:1.0f)+std::sin(time*.18f+j)*.3f;
      float body=nominalLength(h.school)*(.68f+.64f*noise(seed+1));   // real size spread inside a shoal
      emit({p,heading+(reef?std::sin(time+j)*.5f:std::sin(time*.8f+j)*.08f)+alarm*.3f,body,time*(relic?1.2f:medium?5:8)+j,relic?.65f:1.f,h.school,uint32_t(index*100+j)});
    }
    if(predator) emit({hunter,std::atan2(center.x-hunter.x,center.z-hunter.z),1.2f+noise(h.seed)*.4f,time*2,1,Species::Barracuda,uint32_t(index*100+80)});
  }
  int giants=0;
  for(int n=0;n<selectedCount && n<7;++n) {
    const auto& h=at(selected[n]); const int index=identity(selected[n]);
    int population=h.resident==Species::Goby?10:h.resident==Species::Crab?7:4;
    if(h.resident==Species::GardenEel) population=24;
    if(h.resident==Species::Trilobite || h.resident==Species::Opabinia) population=11;
    // One at a time and seldom: a plesiosaur should be something you come across, not
    // scenery. The time term moves which hollow is hosting one, so they are not parked.
    if(h.zone==3 && cameraZone==3 && !giants && (index+int(time/80))%41==0) {
      Species kind=index%8==0?Species::Dunkleosteus:Species::Plesiosaur;
      float a=time*.035f+index;
      Vec3 p=h.water+Vec3{std::cos(a)*5,2+std::sin(a*.7f),std::sin(a)*5};
      p.y=std::max(p.y,scene.surface(p.x,p.z)+3);
      emit({p,std::atan2(-std::sin(a),std::cos(a)),nominalLength(kind),time*.9f,1,kind,uint32_t(index*100+95)});
      ++giants;
    }
    for(int j=0;j<population;++j) {
      uint32_t seed=h.seed+j*47;
      float cycle=std::fmod(time+noise(seed)*64,64.0f);
      auto smooth=[](float x) { x=clampf(x,0,1); return x*x*(3-2*x); };
      float transit=smooth((cycle-24)/6)-smooth((cycle-56)/6);
      float active=((cycle>24 && cycle<30)||(cycle>56 && cycle<62))?1.0f:0.0f;
      float angle=time*.07f+index;
      Vec3 p=h.bottom+Vec3{(noise(seed+1)-.5f)*2,0,(noise(seed+2)-.5f)*2};
      float heading=h.heading;
      if(h.resident==Species::Ray) { p=p+Vec3{std::cos(angle)*2,0,std::sin(angle)*2}; heading=std::atan2(-std::sin(angle),std::cos(angle)); active=1; }
      else if(h.resident==Species::Crab) { p=p+rotateY({transit*.8f,0,0},heading); }
      else if(h.resident==Species::Eel || h.resident==Species::GardenEel || h.resident==Species::Rockfish || h.resident==Species::Sculpin) { active=.25f; }
      else { p=p+rotateY({0,0,transit*1.4f},heading); if(cycle>40) heading+=Pi; }
      emit({p,heading,nominalLength(h.resident)*(.8f+.4f*noise(seed+3)),time*4+j,active,h.resident,uint32_t(index*100+85+j)});
    }
    if(h.zone<2 && index%26==14) {
      float angle=time*.045f+index;
      Vec3 p=h.bottom+Vec3{std::cos(angle)*3,1.0f+.7f*std::sin(angle),std::sin(angle)*3};
      p.y=scene.surface(p.x,p.z)+.8f+.5f*(1+std::sin(angle));
      emit({p,std::atan2(-std::sin(angle),std::cos(angle)),.85f,time*1.4f,.5f+.5f*std::sin(angle),Species::Turtle,uint32_t(index*100+90)});
    }
  }
}
} }
