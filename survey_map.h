#pragma once
#include "scene.h"
#include <array>
namespace abyss {
class SurveyMap {
 public:
  static constexpr int Cells=80,LayerBytes=Cells*Cells/8,SaveBytes=LayerBytes*2+8;
  bool expanded=false;
  void build(const Scene& scene);
  bool visit(Vec3 eye,bool city);
  bool visited(int x,int z,bool city) const;
  // Drawn inside [x0, x0+width): the small map keeps clear of the touch controls and
  // the expanded one is centred on the 4:3 box, so neither ends up underneath them.
  void draw(uint16_t* pixels,int x0,int width,int height,const Camera& camera,bool city,uint32_t monuments) const;
  std::array<uint8_t,SaveBytes> encode() const;
  bool decode(const uint8_t* data,size_t size);
 private:
  uint8_t explored_[2][LayerBytes]{};
  uint16_t terrain_[Cells*Cells]{};
};
}
