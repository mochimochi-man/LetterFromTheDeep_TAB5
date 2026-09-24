#pragma once
#include "math3d.h"
namespace abyss {
class Scene;
namespace ecology {
// New kinds are appended: Journal::speciesId pins the save bit for every existing one.
enum class Species : uint8_t { Sprat, ReefFish, Jack, Barracuda, Goby, Flatfish, Ray, Crab, Turtle, Lanternfish, Eel, Butterflyfish, Surgeonfish, GardenEel, Goatfish, Rockfish, Sculpin, Viperfish, Squid, Anomalocaris, Sacabambaspis, Dunkleosteus, Plesiosaur, Trilobite, Opabinia, CombJelly,
  Tuna, Salmon, SeaBream, Marlin, SpanishMackerel, Cod, Mackerel,
  Sunfish, Anglerfish, Oarfish, AtkaMackerel, Herring, Saury, Yellowtail, Pufferfish, Hammerhead, Dolphin, Orca, WhiteShark, Manta, WhaleShark, Nautilus, Coelacanth, Count };
struct Animal {
  Vec3 position; float yaw,length,phase,activity; Species species; uint32_t id;
};
struct Habitat {
  Vec3 water,bottom; float heading,light; uint32_t seed;
  uint8_t zone; Species school,resident;
};
struct CoelShelter { Vec3 home,outward; uint32_t seed; };
float nautilusExtension(float distance);
float gardenEelExtension(float distance,uint32_t id);
Animal shelterCoelacanth(const CoelShelter& shelter,int index,float time);
class Ecosystem {
 public:
  static constexpr int HabitatCount=480, MaxAnimals=400, LocalCount=9;
  Habitat habitats[HabitatCount]{};
  Habitat local[LocalCount]{};                 // rebuilt around the camera every frame
  Animal animals[MaxAnimals]{};
  int count=0;
  static constexpr int ShelterLimit=6;
  CoelShelter shelters[ShelterLimit]{};
  int shelterCount=0;
  void buildRelicShelters(const Scene& scene);
  void restingCoelacanths(const Scene& scene,const Camera& camera,float time);
  void build(const Scene& scene);
  void largeGroups(const Scene& scene,const Camera& camera,float time);
  void nearbyHabitats(const Scene& scene,const Camera& camera);
  void update(const Scene& scene,const Camera& camera,float time);
};
// Where in the water column a kind lives. A species may span several bands.
enum Band : uint8_t { BandUpper=1, BandMiddle=2, BandLower=4, BandSeabed=8 };
uint8_t bands(Species species);
// A height inside the bands this species uses, for a column running from bed to surface.
float bandHeight(Species species,float bed,uint32_t seed);
const char* name(Species species);
float nominalLength(Species species);
bool bottomDweller(Species species);
bool ancient(Species species);
bool allowedInZone(Species species,int zone);
}
}
