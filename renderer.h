#pragma once
#include "scene.h"
#include <atomic>
namespace abyss {
// The palette sits last so a triangle can copy only the lighting levels it can reach.
struct Projected {
  float x[3],y[3];
  // Only vertex 0 travels with the gradients; the other two are consumed while projecting.
  // u0/v0 and their gradients are u/z and v/z, not u and v: those are the quantities that
  // run straight across the screen, and the rasteriser divides them back by 1/z as it
  // goes. Interpolating u and v directly is what makes a surface seen edge-on smear.
  float q0,u0,v0;
  float dqdx,dqdy,dxdy[3];
  float dudx,dudy,dvdx,dvdy;
  uint8_t material;
  bool emissive;
  int16_t x0,x1,y0,y1;
  uint16_t palette[5][8];
};
struct RenderStats { int submitted=0, visible=0, clipped=0, chunks=0; bool overflow=false;
  // Diagnostics for the render cost breakdown; summed from both render threads.
  uint32_t fillUs=0, rasterUs=0, emitUs=0; };
class Renderer {
 public:
  Projected* projected=nullptr;
  uint16_t* pixels=nullptr;
  uint16_t* tileIndices=nullptr;
  uint16_t tileSizes[TileCount]{};
  RenderStats stats;
  bool headlightEnabled=true;
  bool cullSolid=true;                 // off only to check what culling is costing
  int culled=0;                        // back faces thrown away last frame
  float headlightStrength=0;
  void attach(Projected* work,uint16_t* output,uint16_t* bins) { projected=work; pixels=output; tileIndices=bins; }
  void prepare(const Scene& scene,const Camera& camera,float time);
  // Each caller exclusively owns [y0,y1), plus its private Width*TileRows depth buffer.
  // With a shared cursor both callers pass the full height and pull tiles as they finish,
  // so the empty water band above the seabed no longer leaves one core idle.
  void renderRows(int y0,int y1,uint16_t* depth,std::atomic<int>* cursor=nullptr);
  void bloom();
  void bloomSeed();
  void bloomApply(int y0,int y1);
 private:
  uint8_t emission_[Width*Height/8]{};
  bool anyEmission_=false;
  uint16_t glow_[2][Width/4*Height/4]{};
  Color waterTop{},waterBottom{};
  uint32_t visibleStatic_[(MaxTriangles+31)/32]{};
  Camera camera_;
  float time_=0,far_=Far,fogCurve_=.7f;
  struct Particle { int16_t x,y; uint16_t depth,color; uint8_t radius=0,alpha=32; };
  Particle particles_[180]{};
  int particleCount_=0;
  uint8_t lampCone_[Height/4+1][Width/4+1]{};
  bool texturesReady_=false;
  uint8_t textures_[6][32*32]{};
  void makeTextures();
  Color water(float sy) const { return blend(waterTop,waterBottom,clampf(sy*3.2f,0,1)); }
  void project(Vec3 a,Vec3 b,Vec3 c,const Triangle& triangle,const float* u,const float* v,uint8_t material);
};
}
