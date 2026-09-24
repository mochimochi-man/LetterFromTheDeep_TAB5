#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#if defined(__GNUC__)
#define ABYSS_INLINE inline __attribute__((always_inline))
#else
#define ABYSS_INLINE inline
#endif
namespace abyss {
constexpr float Pi = 3.14159265358979323846f;
ABYSS_INLINE float clampf(float a, float lo, float hi) { return std::max(lo, std::min(a, hi)); }
// ceilf is a real library call on this toolchain; the raster loop calls it per scanline.
ABYSS_INLINE int ceilToInt(float a) { int i=int(a); return float(i)<a?i+1:i; }
struct Vec3 {
  float x = 0, y = 0, z = 0;
  ABYSS_INLINE Vec3 operator+(Vec3 b) const { return {x+b.x, y+b.y, z+b.z}; }
  ABYSS_INLINE Vec3 operator-(Vec3 b) const { return {x-b.x, y-b.y, z-b.z}; }
  ABYSS_INLINE Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
};
ABYSS_INLINE float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
ABYSS_INLINE Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
ABYSS_INLINE float length(Vec3 v) { return std::sqrt(dot(v,v)); }
ABYSS_INLINE Vec3 unit(Vec3 v) { return v*(1.0f/std::max(0.00001f,length(v))); }
ABYSS_INLINE Vec3 mix(Vec3 a, Vec3 b, float t) { return a*(1-t)+b*t; }
ABYSS_INLINE Vec3 rotateY(Vec3 p, float a) {
  float s=std::sin(a), c=std::cos(a);
  return {c*p.x+s*p.z,p.y,-s*p.x+c*p.z};
}
ABYSS_INLINE Vec3 spline(Vec3 a, Vec3 b, Vec3 c, Vec3 d, float t) {
  float t2=t*t, t3=t2*t;
  return (b*2+(c-a)*t+(a*2-b*5+c*4-d)*t2+
          (b*3-a-c*3+d)*t3)*0.5f;
}
struct Color { uint8_t r,g,b; };
ABYSS_INLINE Color blend(Color a, Color b, float t) {
  t=clampf(t,0,1);
  return {uint8_t(a.r+(b.r-a.r)*t), uint8_t(a.g+(b.g-a.g)*t),
          uint8_t(a.b+(b.b-a.b)*t)};
}
ABYSS_INLINE Color scale(Color c, float s) {
  return {uint8_t(clampf(c.r*s,0,255)),uint8_t(clampf(c.g*s,0,255)),
          uint8_t(clampf(c.b*s,0,255))};
}
ABYSS_INLINE uint16_t rgb565(Color c) {
  return uint16_t(((c.r&248)<<8)|((c.g&252)<<3)|(c.b>>3));
}
ABYSS_INLINE uint32_t hash(uint32_t n) {
  n ^= n>>16; n *= 0x7feb352dU; n ^= n>>15; n *= 0x846ca68bU;
  return n^(n>>16);
}
ABYSS_INLINE float noise(uint32_t n) { return (hash(n)&65535)/65535.0f; }
struct Camera {
  Vec3 position, right, up, forward;
  void lookAt(Vec3 eye, Vec3 target) {
    position=eye; forward=unit(target-eye);
    right=unit(cross({0,1,0},forward)); up=cross(forward,right);
  }
  ABYSS_INLINE Vec3 view(Vec3 p) const {
    p=p-position; return {dot(p,right),dot(p,up),dot(p,forward)};
  }
};
}
