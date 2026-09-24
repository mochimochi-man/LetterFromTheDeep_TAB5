#include "journal.h"
namespace abyss {
const Monument Journal::catalog[MonumentCount]={
 {0,0,"THE STONE GATE",{-12,8,6},7,true},
 {1,0,"THE FORGOTTEN HULL",{14,5,25},5,true},
 {2,0,"SUNKEN COLUMNS",{-1,4,39},3,true},
 {3,1,"TWIN TOWERS",{249,12,-214},12,true},
 {4,1,"SPHINX",{267,5,-83},6,true},
 {5,2,"SUNKEN TORII",{253,6,129},5,true},
 {6,2,"LOST OBSERVATORY",{205,8,264},7,true},
 {7,0,"THE MAN AND HIS DOG",{-65,3,-40},3,true},
 {8,0,"TRIUMPHAL ARCH",{70,6,-50},5,true},
 {9,4,"SLEEPING MOAI",{-259,4,-158},4,true},
 {10,4,"DEEP COLONNADE",{-245,5,-26},5,true},
 {11,3,"THE VANITY OF MANKIND",{-44,12,173},16,true},
 {12,1,"BONES OF A GIANT",{84,4,-150},14,true},
 {13,1,"THE DROWNED CASTLE",{156,14,-30},16,true},
 {14,2,"THE STEPPED TERRACE",{84,9,270},18,true},
 {15,3,"THE BLACK MONOLITH",{-120,12,102},8,true},
 {16,4,"A VENDING MACHINE",{-90,5,-144},9,true}
};
uint8_t Journal::speciesId(ecology::Species s) {
 // Permanent save IDs. Reordering the species enum must not change these numbers.
 using S=ecology::Species;
 switch(s) {
#define RECORD(name,id) case S::name: return id;
 RECORD(Sprat,0) RECORD(ReefFish,1) RECORD(Jack,2) RECORD(Barracuda,3)
 RECORD(Goby,4) RECORD(Flatfish,5) RECORD(Ray,6) RECORD(Crab,7) RECORD(Turtle,8)
 RECORD(Lanternfish,9) RECORD(Eel,10) RECORD(Butterflyfish,11) RECORD(Surgeonfish,12)
 RECORD(GardenEel,13) RECORD(Goatfish,14) RECORD(Rockfish,15) RECORD(Sculpin,16)
 RECORD(Viperfish,17) RECORD(Squid,18) RECORD(Anomalocaris,19) RECORD(Sacabambaspis,20)
 RECORD(Dunkleosteus,21) RECORD(Plesiosaur,22) RECORD(Trilobite,23) RECORD(Opabinia,24)
 RECORD(CombJelly,25)
 RECORD(Tuna,27) RECORD(Salmon,28) RECORD(SeaBream,29) RECORD(Marlin,30)
 RECORD(SpanishMackerel,31) RECORD(Cod,32) RECORD(Mackerel,33)
 RECORD(Sunfish,34) RECORD(Anglerfish,35) RECORD(Oarfish,36) RECORD(AtkaMackerel,37)
 RECORD(Herring,38) RECORD(Saury,39) RECORD(Yellowtail,40) RECORD(Pufferfish,41)
 RECORD(Hammerhead,42) RECORD(Dolphin,43) RECORD(Orca,44)
 RECORD(WhiteShark,45) RECORD(Manta,46) RECORD(WhaleShark,47)
 RECORD(Nautilus,48) RECORD(Coelacanth,49)
#undef RECORD
 default:return 63;
 }
}
bool Journal::allMonumentsFound() const {
 uint32_t required=0; uint8_t zones=0;
 for(const auto& entry:catalog) {
   if(!entry.placed) return false; // Future catalog entries must be built before counting completion.
   required|=1u<<entry.id; zones|=uint8_t(1u<<entry.zone);
 }
 return zones==31 && (monuments&required)==required;
}
bool Journal::update(const Scene& scene,const Camera& cam,float seconds) {
 noticeSeconds=std::max(0.0f,noticeSeconds-seconds);
 interval_+=seconds;
 if(interval_<.25f) return false;
 float dt=std::min(interval_,.5f); interval_=0;
 if(noticeSeconds>0) { candidate_=-1; dwell_=0; return false; }
 int chosen=-1; const char* title=nullptr; const char* type=nullptr;
 auto visible=[&](Vec3 p,float radius,float range,float pixels) {
   Vec3 v=cam.view(p);
   return v.z>.5f && v.z<range && std::abs(v.x)<v.z*.46f+radius && std::abs(v.y)<v.z*.36f+radius &&
     radius*Focal/v.z>=pixels && scene.clearView(cam.position,p,radius);
 };
 for(const auto& entry:catalog) {
   if(scene.city || !entry.placed || (monuments&(1u<<entry.id))) continue;
   Vec3 p=entry.position; p.y+=scene.surface(p.x,p.z);
   if(visible(p,entry.radius,48,12)) { chosen=entry.id;title=entry.name;type="MONUMENT DISCOVERED";break; }
 }
 if(chosen<0) for(int i=0;i<scene.ventCount;++i) {
   if(nature&(1u<<i)) continue;
   if(visible(scene.vents[i].mouth,2,28,10)) { chosen=32+i;title="HYDROTHERMAL VENTS";type="LANDSCAPE RECORDED";break; }
 }
 if(chosen<0) {
   float best=0;
   for(int i=0;i<scene.ecosystem.count;++i) {
     const auto& a=scene.ecosystem.animals[i]; uint8_t id=speciesId(a.species);
     if(id==63 || (species&(uint64_t(1)<<id))) continue;
     Vec3 v=cam.view(a.position); float size=a.length*Focal/std::max(1.0f,v.z);
     if(size<=best || !visible(a.position,a.length*.5f,28,2.5f)) continue;
     best=size;chosen=64+id;title=ecology::name(a.species);type="SPECIES RECORDED";
   }
   // Save ID 26 belongs to the original large whale, which is outside the small-animal ecosystem pool.
   if(scene.whalePresent && !(species&(uint64_t(1)<<26)) && visible(scene.whaleLocation,3.5f,45,10)) {
     chosen=64+26;title="whale";type="SPECIES RECORDED";
   }
   // The machines that still swim the sunken city. They live outside the ecosystem pool
   // too, so they are looked for by hand, and only while down there.
   if(scene.city) for(int i=0;i<scene.cityMachineCount;++i) {
     const MachinePose& m=scene.cityMachines[i];
     uint8_t id=uint8_t(MachineFirstId+int(m.kind));
     if(species&(uint64_t(1)<<id)) continue;
     Vec3 v=cam.view(m.position); float size=m.length*Focal/std::max(1.0f,v.z);
     if(size<=best || !visible(m.position,m.length*.5f,34,3.0f)) continue;
     best=size;chosen=64+id;title=machineName(m.kind);type="MACHINE RECORDED";
   }
 }
 if(chosen!=candidate_) { candidate_=chosen;dwell_=0; }
 if(chosen<0) return false;
 dwell_+=dt;
 if(dwell_<.75f) return false;
 if(chosen<32) monuments|=1u<<chosen;
 else if(chosen<64) nature|=1u<<(chosen-32);
 else species|=uint64_t(1)<<(chosen-64);
 notice=title;category=type;noticeSeconds=3.5f;candidate_=-1;dwell_=0;
 return true;
}
std::array<uint8_t,Journal::SaveBytes> Journal::encode() const {
 std::array<uint8_t,SaveBytes> out{};
 out[0]='A';out[1]='B';out[2]='J';out[3]=1;
 out[20]=allMonumentsFound()?(finalFlags&3):0;
 for(int i=0;i<4;++i) { out[4+i]=uint8_t(monuments>>(i*8));out[16+i]=uint8_t(nature>>(i*8)); }
 for(int i=0;i<8;++i) out[8+i]=uint8_t(species>>(i*8));
 uint32_t sum=2166136261u;for(int i=0;i<24;++i) sum=(sum^out[i])*16777619u;
 for(int i=0;i<4;++i) out[24+i]=uint8_t(sum>>(i*8));
 return out;
}
bool Journal::decode(const uint8_t* in,size_t size) {
 if(size!=SaveBytes || in[0]!='A' || in[1]!='B' || in[2]!='J' || in[3]!=1) return false;
 uint32_t sum=2166136261u,stored=0;for(int i=0;i<24;++i) sum=(sum^in[i])*16777619u;
 for(int i=0;i<4;++i) stored|=uint32_t(in[24+i])<<(i*8);
 if(sum!=stored) return false;
 monuments=nature=0;species=0;
 for(int i=0;i<4;++i) { monuments|=uint32_t(in[4+i])<<(i*8);nature|=uint32_t(in[16+i])<<(i*8); }
 for(int i=0;i<8;++i) species|=uint64_t(in[8+i])<<(i*8);
 finalFlags=allMonumentsFound()?(in[20]&3):0;
 return true;
}
}
