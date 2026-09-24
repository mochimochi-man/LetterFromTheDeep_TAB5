#pragma once
#include "config.h"
#include "math3d.h"
#include "world.h"
#include "ecology.h"
#include "mechanical.h"
// ESP newlib headers expose a legacy quad -> quad_t macro.
// Keep the polygon helper name identical in every translation unit.
#ifdef quad
#undef quad
#endif
namespace abyss {
// Top bit of material marks a face belonging to a closed body, so the renderer may
// throw it away when it is wound away from the camera. Everything else stays two-sided,
// because most of this world is thin plates - fins, fronds, kelp - with no back to hide.
constexpr uint8_t SolidFace=0x80;
struct Triangle { Vec3 a,b,c; Color color; uint8_t material; uint16_t shades[8]; };
struct Chunk { Vec3 minimum,maximum; uint32_t start; uint16_t count; };
struct Landmark { Vec3 position; const char* name; };
// The undersea city's ground plan.
//
// Its terraces are curves of constant q rather than lines of constant z. q grows with the
// distance from the middle, so a terrace bends away from the arrival where the pilot is
// and comes forward at its ends: the place is a bowl the city sits inside, not a fan
// spreading out of it. Because the height depends on q alone, every building on one
// terrace stands at one height however far along it is, which is what a row of seats in
// an arena does.
//
// CityTier is how far apart the terraces are, in q. CityEdge is where the built ground
// ends and the wall of the hollow starts climbing, which is what closes the two ends off
// instead of leaving empty shelves running away into the water.
constexpr float CityBowl=260, CityTier=28, CityEdge=72, CityWall=78;
// The eye a large fish wears.
//
// A dark pupil inside a thin pale ring, and both of them lying in the plane of the cheek
// rather than square to the world. A fish's head narrows towards the snout, so the disc
// leans: `taper` is how far its forward edge moves inward for each unit of reach across
// it. Without that lean the eye is a patch stuck on the side of the head, which is what
// gave the tuna a square one and what shows on anything big enough to be looked at.
//
// Only the big fish get this. Small ones are a few pixels across and already carry eyes
// of their own, and the whales, the dolphins, the orca and the plesiosaur are not fish:
// their eyes are dark and lidded and have no ring at all.
//
// Every mesh here has its own idea of what local space is - its own bend, its own
// rotation, its own scale - so the caller passes in whatever it uses to emit a triangle
// and this decides nothing but the shape.
template<class Emit>
void fishEye(Emit&& emit,Vec3 centre,float side,float radius,float taper,bool close) {
  const int facets=close?12:6;
  auto disc=[&](float r,float lift,Color colour) {
    const Vec3 c=centre+Vec3{side*lift,0,0};
    auto rim=[&](float angle) {
      const float dz=std::cos(angle)*r;
      return c+Vec3{-side*dz*taper,std::sin(angle)*r,dz};
    };
    for(int k=0;k<facets;++k)
      emit(c,rim(k*2*Pi/facets),rim((k+1)*2*Pi/facets),colour);
  };
  // Far enough away that the ring would be one pixel, the pupil is the whole eye - which
  // is what an eye is at that range, and cheaper than the patches these replace.
  if(close) disc(radius,0,Color{132,146,150});
  disc(radius*.70f,radius*.055f,Color{18,26,34});
}
class Scene {
 public:
  Triangle* triangles=nullptr;
  int count=0, staticCount=0, solidCount=0;
  ecology::Ecosystem ecosystem;
  struct Colony { Vec3 position; float height; uint8_t type; };
  static constexpr int ColonyLimit=560;
  Colony colonies[ColonyLimit]{};
  int colonyCount=0;
  struct Vent { Vec3 mouth; uint32_t seed; };
  Vent vents[4]{};
  int ventCount=0;
  uint16_t* staticIndices=nullptr;
  Chunk chunks[ChunkCount]{};
  bool overflow=false;
  bool city=false,gateOpen=false;
  int gateStart=0,gateEnd=0;
  float gateSink=0;
  bool whalePresent=false;
  Vec3 whaleLocation{};
  // This frame's city machines, kept so the journal can record them like any animal.
  MachinePose cityMachines[CityMachineCount]{};
  int cityMachineCount=0;
  bool build(Triangle* storage,uint16_t* indices);
  bool buildCity(Triangle* storage,uint16_t* indices);
  void sinkGate(float metres);
  world::Environment environment(Vec3 eye) const;
  // A place in the city as its terraces see it, and the way back: where on the terrace of
  // a given q a point of that x sits. The buildings are laid out in q and turned into
  // places through these, so the city and the ground it stands on cannot drift apart.
  static float cityQ(float x,float z);
  static float cityRowZ(float x,float q);
  Camera cityTour(float time) const;
  void animateCity(float time,const Camera& camera);
  void animate(float time);
  void animate(float time,const Camera& camera);
  Camera tour(float time) const;
  float floor(float x,float z) const;
  float surface(float x,float z,bool solids=false) const;
  bool clearView(Vec3 from,Vec3 to,float allowance=0) const;
  bool clearHull(Vec3 position,float radius) const;
  const Landmark* nearby(Vec3 eye) const;
  void add(Vec3 a,Vec3 b,Vec3 c,Color col,bool twoSided=true);
  // Same, but for a face on a closed body: the winding is corrected against an outward
  // direction first, so a mesh built without caring which way round its faces go still
  // ends up cullable. `outward` only has to point away from the middle of the body.
  void addSolid(Vec3 a,Vec3 b,Vec3 c,Color col,Vec3 outward);
  void quadSolid(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col,Vec3 outward);
  // Catalogue pictures: stage one creature on an empty scene, nothing else in the world.
  void poseMachine(const MachinePose& pose);
  void machineMesh(const MachinePose& pose);
  // The wreck-side display: a carrier and her boats, moored at the end of the approach.
  // They are scenery. Nothing drives them, nothing records them, and they are posed from
  // a MachinePose only because it is already the right shape for a place and a heading.
  void poseAlcyone(const MachinePose& pose);
  void alcyoneMesh(const MachinePose& pose);
  void poseSubmarine(const MachinePose& pose);
  void submarineMesh(const MachinePose& pose);
  void poseCreature(const ecology::Animal& animal,bool whale);
  // Running out of triangles is fatal, so the creature meshes watch how full the buffer
  // is and give up detail as it fills. A crowded reef then degrades instead of dying.
  // Measured against what is left after the static world, not against the whole buffer.
  // The static world has grown to two thirds of the budget, and a fixed fraction of the
  // total meant the very first animal of the frame already counted as crowded, so every
  // creature was permanently held at reduced detail.
  bool crowded() const { return count>staticCount+(MaxTriangles-staticCount)*7/10; }
  bool jammed() const { return count>staticCount+(MaxTriangles-staticCount)*17/20; }
 private:
  uint8_t material_=0;
  float lighting_=1;
  Vec3 viewer_{};                              // camera position, so a mesh can pick its detail
  void buildMap();
  void buildVents();
  void buildMonuments();
  void buildSeabedLife();
  void reefCoral(Vec3 p,float size,uint32_t seed,int type);
  void seabedLife(Vec3 p,float size,uint32_t seed,int type);
  void buildIndex(uint16_t* indices);
  void quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Color col);
  void rock(Vec3 p,Vec3 size,uint32_t seed,int style=0);
  void beam(Vec3 a,Vec3 b,float width,Color col);
  void cliff(Vec3 start,Vec3 end,float width,float height,uint32_t seed);
  void ruins();
  void vanity();
  void arch();
  void wreck();
  void coral(Vec3 p,float size,uint32_t seed);
  void prehistoricMesh(const ecology::Animal& animal);
  void livingRelicMesh(const ecology::Animal& animal);
  void sunfishMesh(const ecology::Animal& animal);
  void turtlePufferMesh(const ecology::Animal& animal);
  void largeMarineMesh(const ecology::Animal& animal);
  void animalMesh(const ecology::Animal& animal);
  void fish(Vec3 p,float yaw,float size,float phase,Color col,bool whale);
};
}
