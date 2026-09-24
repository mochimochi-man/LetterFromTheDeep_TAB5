#pragma once
#include "scene.h"
#include <array>
namespace abyss {
struct Monument { uint8_t id,zone; const char* name; Vec3 position; float radius; bool placed; };
class Journal {
 public:
  static constexpr int MonumentCount=17, SaveBytes=28;
  uint32_t monuments=0,nature=0;
  uint8_t finalFlags=0; // bit 0: collapsed gate, bit 1: visited city; reserved v1 byte 20
  uint64_t species=0;
  const char* notice=nullptr;
  const char* category=nullptr;
  float noticeSeconds=0;
  static const Monument catalog[MonumentCount];
  static uint8_t speciesId(ecology::Species s);
  bool update(const Scene& scene,const Camera& camera,float seconds);
  bool allMonumentsFound() const;
  std::array<uint8_t,SaveBytes> encode() const;
  bool decode(const uint8_t* bytes,size_t size);
 private:
  float interval_=0,dwell_=0;
  int candidate_=-1;
};
}
