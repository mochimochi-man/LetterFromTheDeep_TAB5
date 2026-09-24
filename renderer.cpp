#if defined(__GNUC__)
// These finite, bounded geometry loops benefit from inlining and reciprocal math.
#pragma GCC optimize ("O3", "fast-math")
#endif
#include "renderer.h"
#include <cstring>
#include <cstddef>
#if defined(ARDUINO)
#include <Arduino.h>
static inline uint32_t clockUs() { return uint32_t(micros()); }
#else
#include <chrono>
static inline uint32_t clockUs() {
  using namespace std::chrono;
  return uint32_t(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}
#endif
namespace abyss {
void Renderer::makeTextures() {
  // Small procedural patterns, generated here. No image assets of any kind.
  for(int y=0;y<32;++y) for(int x=0;x<32;++x) {
    uint32_t h=hash(x+y*137+982);
    int grain=int(h&3);
    int seam=((x+int(3*std::sin(y*.45f)))%13==0)?-3:0;
    textures_[0][y*32+x]=uint8_t(std::max(0,std::min(7,3+grain+seam)));
    float ripple=std::sin(y*.85f+std::sin(x*.31f)*1.1f);
    textures_[1][y*32+x]=uint8_t(std::max(0,std::min(7,4+int(ripple*1.4f)+int(h%3)-1)));
    textures_[2][y*32+x]=uint8_t((y%8==0)?1:2+((h>>8)%6));
    textures_[3][y*32+x]=uint8_t(2+((hash((x/2)+(y/2)*197)>>4)%6));
    textures_[4][y*32+x]=uint8_t((y%9==0 || ((x+(y/9)*9)%21==0))?1:4+(h%3));
    textures_[5][y*32+x]=uint8_t(3+(h%3));
  }
  for(int y=0;y<=Height/4;++y) for(int x=0;x<=Width/4;++x) {
    // A wider cone with a higher floor: the beam used to fall off well inside the frame.
    float dx=(x*4-Width*.5f)/(Width*.52f),dy=(y*4-Height*.60f)/(Width*.52f);
    float c=clampf(1-dx*dx-dy*dy,0,1);
    lampCone_[y][x]=uint8_t(c*(.72f+.28f*c)*16);
  }
  texturesReady_=true;
}
void Renderer::prepare(const Scene& scene,const Camera& camera,float time) {
  if(!texturesReady_) makeTextures();
  stats={}; culled=0; camera_=camera; time_=time; anyEmission_=false;
  std::fill(tileSizes,tileSizes+TileCount,0);
  auto env=scene.environment(camera.position);
  headlightStrength=headlightEnabled?clampf((1.04f-env.light)/.30f,0,1):0;
  // The haze the distance blends into is lifted with the rest of the water.
  waterTop=scale(env.top,1.16f); waterBottom=scale(env.water,1.22f); far_=env.visibility;
  fogCurve_= .70f-.40f*clampf((env.light-.68f)/.60f,0,1);
  // Out in the seas the haze is made to gather in the middle distance, which is what
  // deep water does and what keeps a basin feeling like one. The city is not a basin. It
  // is the one place down here that is lit, and light carries: a curve that has half of
  // it gone by forty-five metres makes a city end two streets away, whatever is drawn
  // beyond. So its haze is left to thin evenly and then some, and the far towers stay in
  // sight all the way to the edge of what the culling allows.
  if(scene.city) fogCurve_=-.35f;
  particleCount_=0;
  uint32_t mark=clockUs();
  int cx=int(std::floor(camera.position.x/16)),cy=int(std::floor(camera.position.y/12)),cz=int(std::floor(camera.position.z/16));
  for(int dz=-2;dz<=2;++dz) for(int dx=-2;dx<=2;++dx) for(int dy=-1;dy<=1;++dy) {
    uint32_t seed=hash(uint32_t(cx+dx)*73856093U ^ uint32_t(cy+dy)*19349663U ^ uint32_t(cz+dz)*83492791U);
    Vec3 w{(cx+dx+noise(seed))*16,(cy+dy+noise(seed+1))*12,(cz+dz+noise(seed+2))*16};
    w.y+=.25f*std::sin(time*.2f+noise(seed+3)*6);
    Vec3 v=camera.view(w);
    if(v.z<Near || v.z>32) continue;
    float sx=Width*.5f+v.x*FocalX/v.z,sy=Height*.5f-v.y*Focal/v.z;
    if(sx<0 || sx>=Width || sy<0 || sy>=Height) continue;
    particles_[particleCount_++]={int16_t(sx),int16_t(sy),uint16_t(Near*65534/v.z),
        rgb565(blend({80,139,155},waterBottom,v.z/40))};
  }
  for(int vent=0;vent<scene.ventCount;++vent) {
    const auto& source=scene.vents[vent];
    Vec3 center=camera.view(source.mouth); if(center.z<-8 || center.z>55) continue;
    for(int j=0;j<18;++j) {
      float rise=std::fmod(time*.8f+j*.51f,9.0f);
      Vec3 p=source.mouth+Vec3{std::sin(rise*.7f+vent)*rise*.18f,rise,std::sin(rise*.4f+j)*rise*.08f};
      Vec3 v=camera.view(p);if(v.z<Near || v.z>50) continue;
      float x=Width*.5f+v.x*FocalX/v.z,y=Height*.5f-v.y*Focal/v.z;
      if(x<0 || x>=Width || y<0 || y>=Height || particleCount_>=180) continue;
      uint8_t radius=uint8_t(clampf((.2f+rise*.10f)*Focal/v.z,1,4.f*UpScale));
      particles_[particleCount_++]={int16_t(x),int16_t(y),uint16_t(Near*65534/v.z),rgb565({44,58,60}),radius,uint8_t(18*(1-rise/9))};
    }
  }
  // Back-to-front translucent plume particles; static and animal surfaces still occlude them.
  for(int i=1;i<particleCount_;++i) {
    Particle p=particles_[i];int j=i;
    while(j>0 && particles_[j-1].depth>p.depth) {particles_[j]=particles_[j-1];--j;}
    particles_[j]=p;
  }
  mark=clockUs();
  auto emit=[&](int i) {
    const Triangle& t=scene.triangles[i];
    ++stats.submitted;
    Vec3 v[3]={camera.view(t.a),camera.view(t.b),camera.view(t.c)};
    // A closed body hides its own far side. The outward normal of a face we can see
    // points back towards the camera, so a positive dot with the view ray means we are
    // looking at its back and there is no point projecting, clipping or binning it.
    if(cullSolid && (t.material&SolidFace) && dot(cross(v[1]-v[0],v[2]-v[0]),v[0])>0) { ++culled; return; }
    if(v[0].z>far_ && v[1].z>far_ && v[2].z>far_) return;
    if(v[0].z<Near && v[1].z<Near && v[2].z<Near) return;
    // Reject outside the four side planes before clipping/UV/palette work.
    constexpr float sx=Width*.5f/FocalX,sy=Height*.5f/Focal;
    if((v[0].x < -v[0].z*sx && v[1].x < -v[1].z*sx && v[2].x < -v[2].z*sx) ||
       (v[0].x >  v[0].z*sx && v[1].x >  v[1].z*sx && v[2].x >  v[2].z*sx) ||
       (v[0].y < -v[0].z*sy && v[1].y < -v[1].z*sy && v[2].y < -v[2].z*sy) ||
       (v[0].y >  v[0].z*sy && v[1].y >  v[1].z*sy && v[2].y >  v[2].z*sy)) return;
    // Sutherland-Hodgman near clipping, including triangles with two vertices behind.
    Vec3 normal=cross(t.b-t.a,t.c-t.a);
    Vec3 w[3]={t.a,t.b,t.c};
    float u[3]={},vv[3]={},ou[4],ov[4];
    uint8_t mat=uint8_t(t.material&~SolidFace);
    for(int k=0;mat && mat<=6 && k<3;++k) {
      if(std::abs(normal.y)>std::abs(normal.x) && std::abs(normal.y)>std::abs(normal.z)) {
        u[k]=w[k].x*3.2f; vv[k]=w[k].z*3.2f;
      } else if(std::abs(normal.x)>std::abs(normal.z)) {
        u[k]=w[k].z*3.2f; vv[k]=w[k].y*3.2f;
      } else { u[k]=w[k].x*3.2f; vv[k]=w[k].y*3.2f; }
    }
    Vec3 out[4]; int n=0;
    for(int e=0;e<3;++e) {
      Vec3 a=v[e],b=v[(e+1)%3];
      bool ia=a.z>=Near, ib=b.z>=Near;
      if(ia) { out[n]=a; ou[n]=u[e]; ov[n]=vv[e]; ++n; }
      if(ia!=ib) {
        float fraction=(Near-a.z)/(b.z-a.z);
        Vec3 p=mix(a,b,fraction); p.z=Near; out[n]=p;
        ou[n]=u[e]+(u[(e+1)%3]-u[e])*fraction;
        ov[n]=vv[e]+(vv[(e+1)%3]-vv[e])*fraction; ++n;
      }
    }
    if(n<3) return;
    if(n!=3 || v[0].z<Near || v[1].z<Near || v[2].z<Near) ++stats.clipped;
    for(int j=1;j<n-1;++j) {
      float tu[3]={ou[0],ou[j],ou[j+1]},tv[3]={ov[0],ov[j],ov[j+1]};
      project(out[0],out[j],out[j+1],t,tu,tv,mat<=6?mat:0);
    }
  };
  if(scene.staticIndices) {
    constexpr float sx=Width*.5f/FocalX,sy=Height*.5f/Focal;
    auto outside=[&](Vec3 normal,float distance,Vec3 center,Vec3 half) {
      float radius=std::abs(normal.x)*half.x+std::abs(normal.y)*half.y+std::abs(normal.z)*half.z;
      return dot(normal,center)-distance>radius;
    };
    Vec3 right=camera.right,up=camera.up,forward=camera.forward;
    std::fill(visibleStatic_,visibleStatic_+(scene.staticCount+31)/32,0);
    for(const auto& chunk:scene.chunks) {
      if(!chunk.count) continue;
      Vec3 center=(chunk.minimum+chunk.maximum)*.5f-camera.position;
      Vec3 half=(chunk.maximum-chunk.minimum)*.5f;
      if(outside(forward,far_,center,half) || outside(forward*-1,-Near,center,half) ||
         outside(right-forward*sx,0,center,half) || outside(right*-1-forward*sx,0,center,half) ||
         outside(up-forward*sy,0,center,half) || outside(up*-1-forward*sy,0,center,half)) continue;
      ++stats.chunks;
      for(int j=0;j<chunk.count;++j) {
        unsigned index=scene.staticIndices[chunk.start+j];
        visibleStatic_[index>>5]|=uint32_t(1)<<(index&31);
      }
    }
    // Keep a stable source order, including exact coplanar depth ties at wall junctions.
    for(int word=0;word<(scene.staticCount+31)/32;++word) {
      uint32_t mask=visibleStatic_[word];
      while(mask) { int bit=__builtin_ctz(mask); emit(word*32+bit); mask&=mask-1; }
    }
    for(int i=scene.staticCount;i<scene.count;++i) emit(i);
    stats.emitUs=clockUs()-mark;
  } else {
    for(int i=0;i<scene.count;++i) emit(i);
  }
}

void Renderer::project(Vec3 a,Vec3 b,Vec3 c,const Triangle& triangle,const float* u,const float* vtex,uint8_t material) {
  if(stats.visible>=MaxProjected) { stats.overflow=true; return; }
  Projected p{};
  Vec3 v[3]={a,b,c};
  float qv[3];
  for(int j=0;j<3;++j) {
    float iz=1/v[j].z;
    p.x[j]=Width*0.5f+v[j].x*FocalX*iz;
    p.y[j]=Height*0.5f-v[j].y*Focal*iz;
    qv[j]=Near*65534.0f*iz;
  }
  // Texturing is perspective correct, so what travels is u/z and v/z. qv is already
  // proportional to 1/z, so multiplying by it is the whole of the change.
  float uq[3],vq[3];
  for(int j=0;j<3;++j) { uq[j]=u[j]*qv[j]; vq[j]=vtex[j]*qv[j]; }
  p.q0=qv[0]; p.u0=uq[0]; p.v0=vq[0];
  float area=(p.x[1]-p.x[0])*(p.y[2]-p.y[0])-
             (p.x[2]-p.x[0])*(p.y[1]-p.y[0]);
  if(std::abs(area)<0.08f) return;
  float minx=std::min(p.x[0],std::min(p.x[1],p.x[2]));
  float maxx=std::max(p.x[0],std::max(p.x[1],p.x[2]));
  float miny=std::min(p.y[0],std::min(p.y[1],p.y[2]));
  float maxy=std::max(p.y[0],std::max(p.y[1],p.y[2]));
  if(maxx<0 || minx>=Width || maxy<0 || miny>=Height) return;
  p.x0=ceilToInt(clampf(minx-0.5f,0,Width));
  p.x1=ceilToInt(clampf(maxx-0.5f,0,Width));
  p.y0=ceilToInt(clampf(miny-0.5f,0,Height));
  p.y1=ceilToInt(clampf(maxy-0.5f,0,Height));
  if(p.x0>=p.x1 || p.y0>=p.y1) return;
  for(int j=0;j<3;++j) {
    int k=(j+1)%3;
    p.dxdy[j]=std::abs(p.y[k]-p.y[j])>.00001f?(p.x[k]-p.x[j])/(p.y[k]-p.y[j]):0;
  }
  float inv=1/area;
  auto gradient=[&](const float* values,float& dx,float& dy) {
    dx=((values[1]-values[0])*(p.y[2]-p.y[0])-(values[2]-values[0])*(p.y[1]-p.y[0]))*inv;
    dy=((p.x[1]-p.x[0])*(values[2]-values[0])-(p.x[2]-p.x[0])*(values[1]-values[0]))*inv;
  };
  gradient(uq,p.dudx,p.dudy); gradient(vq,p.dvdx,p.dvdy);
  uint8_t mat=uint8_t(triangle.material&~SolidFace);
  p.material=mat==8?8:material;p.emissive=mat==7;
  p.dqdx=((qv[1]-qv[0])*(p.y[2]-p.y[0])-
          (qv[2]-qv[0])*(p.y[1]-p.y[0]))*inv;
  p.dqdy=((p.x[1]-p.x[0])*(qv[2]-qv[0])-
          (p.x[2]-p.x[0])*(qv[1]-qv[0]))*inv;
  float distance=(a.z+b.z+c.z)/3;
  float fog=clampf((distance-3)/(far_-3),0,1);
  // Preserve distant water haze while retaining nearby surface contrast.
  fog+=fogCurve_*fog*(1-fog);
  uint32_t amount=uint32_t(fog*32),keep=32-amount;
  uint16_t haze=rgb565(waterBottom);
  uint32_t rb=(haze&0xf81f)*amount,g=(haze&0x07e0)*amount;
  auto fogColor=[&](uint16_t color) {
    return uint16_t(((((color&0xf81f)*keep+rb)>>5)&0xf81f) |
                    ((((color&0x07e0)*keep+g)>>5)&0x07e0));
  };
  for(int shade=material?0:4;shade<(material?8:5);++shade) {
    uint16_t original=material?triangle.shades[shade]:rgb565(triangle.color);
    uint16_t base=mat==7?rgb565(blend(triangle.color,waterBottom,fog*.25f)):fogColor(original); p.palette[0][shade]=base;
    if(headlightStrength>0) {
      Color lit{uint8_t(std::min(255,((original>>11)&31)*13+64)),
        uint8_t(std::min(255,((original>>5)&63)*6+60)),uint8_t(std::min(255,(original&31)*12+44))};
      Color unlit{uint8_t(((base>>11)&31)*255/31),uint8_t(((base>>5)&63)*255/63),uint8_t((base&31)*255/31)};
      for(int level=1;level<5;++level) p.palette[level][shade]=mat==7?base:rgb565(blend(unlit,lit,level*.25f));
    }
  }
  int index=stats.visible++;
  // Only the reachable part of the palette travels to PSRAM: writing all five
  // lighting levels for every triangle was the bulk of the projection cost.
  int levels=headlightStrength>0?5:1;
  size_t bytes=offsetof(Projected,palette)+(levels-1)*sizeof(p.palette[0])+(material?8:5)*sizeof(uint16_t);
  std::memcpy(&projected[index],&p,bytes);
  for(int tile=p.y0/TileRows;tile<=(p.y1-1)/TileRows;++tile)
    tileIndices[tile*MaxProjected+tileSizes[tile]++]=uint16_t(index);
}
// How far the rasteriser walks between perspective divides. Sixteen keeps the texture
// error below a texel on everything this world contains, and costs one divide per
// sixteen pixels rather than one per pixel.
constexpr int PerspectiveSpan=16;
void Renderer::renderRows(int y0,int y1,uint16_t* depth,std::atomic<int>* cursor) {
  int next=y0/TileRows,last=(y1+TileRows-1)/TileRows;
  for(;;) {
    int tile=cursor?cursor->fetch_add(1,std::memory_order_relaxed):next++;
    if(tile>=(cursor?TileCount:last)) break;
    int top=tile*TileRows,bottom=std::min(top+TileRows,Height);
    uint32_t mark=clockUs();
    std::memset(emission_+top*Width/8,0,(bottom-top)*Width/8);
    std::memset(depth,0,Width*(bottom-top)*sizeof(uint16_t));
    for(int y=top;y<bottom;++y) {
      Color rowColor=water(float(y)/Height);
      uint16_t c=rgb565(rowColor);
      std::fill(pixels+y*Width,pixels+(y+1)*Width,c);
    }
    __atomic_fetch_add(&stats.fillUs,clockUs()-mark,__ATOMIC_RELAXED);
    mark=clockUs();
    bool emissive=false,tileEmissive=false;
    const bool lamp=headlightStrength>0;
    for(int i=0;i<tileSizes[tile];++i) {
      // Copy into internal stack RAM once: hot coefficients must not thrash PSRAM cache.
      const Projected p=projected[tileIndices[tile*MaxProjected+i]];
      if(p.y1<=top || p.y0>=bottom) continue;
      const uint8_t* texture=p.material && p.material<=6?textures_[p.material-1]:nullptr;
      int ya=std::max(top,int(p.y0)),yb=std::min(bottom,int(p.y1));
      for(int y=ya;y<yb;++y) {
        float scan=y+0.5f;
        float left=1e20f,right=-1e20f;
        for(int e=0;e<3;++e) {
          int n=(e+1)%3;
          float a=p.y[e],b=p.y[n];
          if((a<=scan && scan<b)||(b<=scan && scan<a)) {
            float x=p.x[e]+(scan-a)*p.dxdy[e];
            left=std::min(left,x); right=std::max(right,x);
          }
        }
        if(left>=right) continue;
        int xa=ceilToInt(clampf(left-0.5f,float(p.x0),float(p.x1)));
        int xb=ceilToInt(clampf(right-0.5f,float(p.x0),float(p.x1)));
        float q0=p.q0+p.dqdx*(xa+.5f-p.x[0])+p.dqdy*(scan-p.y[0]);
        // u/z and v/z run straight across the screen; u and v do not. Dividing them back
        // by 1/z is what keeps the seabed's texture from smearing as it is approached,
        // which is the one place the error of interpolating u and v directly is large
        // enough to see. One divide per PerspectiveSpan pixels holds the error under a
        // texel without paying for a divide at every pixel.
        float qOverZ=q0;
        float uOverZ=p.u0+p.dudx*(xa+.5f-p.x[0])+p.dudy*(scan-p.y[0]);
        float vOverZ=p.v0+p.dvdx*(xa+.5f-p.x[0])+p.dvdy*(scan-p.y[0]);
        float uNow=uOverZ/(qOverZ>1?qOverZ:1),vNow=vOverZ/(qOverZ>1?qOverZ:1);
        int32_t q=int32_t(q0*256),dq=int32_t(p.dqdx*256);
        // An untextured face never reads u or v, so it takes the span in one piece.
        const int stride=texture?PerspectiveSpan:Width;
        uint16_t* z=depth+(y-top)*Width;
        uint16_t* dst=pixels+y*Width;
        // Width is a multiple of eight, so each emission row starts on a byte boundary.
        uint8_t* erow=emission_+(y*Width>>3);
        const uint8_t* lampRow=lampCone_[y>>2];
        const int rowDither=(y&1)*16;
        for(int xs=xa;xs<xb;xs+=stride) {
        const int xe=std::min(xs+stride,int(xb)),run=xe-xs;
        const float qNext=qOverZ+p.dqdx*run,qSafe=qNext>1?qNext:1;
        const float uNext=(uOverZ+p.dudx*run)/qSafe,vNext=(vOverZ+p.dvdx*run)/qSafe;
        // 12 fractional UV bits keep the expanded world's coordinates in int32 range.
        int32_t u=int32_t(uNow*4096),du=int32_t((uNext-uNow)*(4096.0f/run));
        int32_t v=int32_t(vNow*4096),dv=int32_t((vNext-vNow)*(4096.0f/run));
        for(int x=xs;x<xe;++x,q+=dq,u+=du,v+=dv) {
          // Fixed screen-door coverage: skipped glass pixels never write depth.
          if(p.material==8 && ((x+2*y)&3)!=0)continue;
          int value=q>>8;
          uint16_t d=uint16_t(value<1?1:value>65535?65535:value);
          if(d>z[x]) {
            z[x]=d;
            // Clearing a bit only matters once something emissive has been drawn in this tile.
            if(p.emissive) {erow[x>>3]|=uint8_t(1u<<(x&7));emissive=tileEmissive=true;}
            else if(tileEmissive) erow[x>>3]&=uint8_t(~(1u<<(x&7)));
            int shade=texture?texture[((uint32_t(v)>>12)&31)*32+((uint32_t(u)>>12)&31)]:4;
            int level=0;
            if(lamp) {
              // full reach to 28m, fades to zero by 70m; *1866>>16 divides by 35.1
              int range=std::max(0,std::min(16,((int(d)-374)*1866)>>16));
              int strength=int(lampRow[x>>2]*range*headlightStrength);
              level=std::min(4,(strength+(strength>>1)+(x&1)*32+rowDither)>>6);
            }
            dst[x]=p.palette[level][shade];
          }
        }
        qOverZ=qNext;uOverZ+=p.dudx*run;vOverZ+=p.dvdx*run;uNow=uNext;vNow=vNext;
        }
      }
    }
    __atomic_fetch_add(&stats.rasterUs,clockUs()-mark,__ATOMIC_RELAXED);
    if(emissive) anyEmission_=true;
    for(int i=0;i<particleCount_;++i) {
      const Particle& p=particles_[i];
      if(p.y+p.radius<top || p.y-p.radius>=bottom) continue;
      for(int y=std::max(top,int(p.y)-p.radius);y<std::min(bottom,int(p.y)+p.radius+1);++y)
        for(int x=std::max(0,int(p.x)-p.radius);x<std::min(Width,int(p.x)+p.radius+1);++x) {
          int dx=x-p.x,dy=y-p.y;
          float ex=dx/PixelAspect;
          if(ex*ex+float(dy)*dy>float(p.radius)*p.radius || p.depth<=depth[(y-top)*Width+x]) continue;
          uint16_t old=pixels[y*Width+x];uint32_t alpha=p.alpha,keep=32-alpha;
          pixels[y*Width+x]=uint16_t(((((old&0xf81f)*keep+(p.color&0xf81f)*alpha)>>5)&0xf81f)|
                                    ((((old&0x07e0)*keep+(p.color&0x07e0)*alpha)>>5)&0x07e0));
        }
    }
  }
}
}

namespace abyss {
void Renderer::bloom() { bloomSeed(); bloomApply(0,Height); }
void Renderer::bloomSeed() {
 constexpr int W=Width/4,H=Height/4;
 // Most basins draw no emissive fragment at all; the halo passes are pure cost there.
 if(!anyEmission_) return;
 // Only visible emissive fragments seed the halo; hidden windows cannot shine through walls.
 for(int y=0;y<H;++y) for(int x=0;x<W;++x) {
  // A 4x4 block covers one nibble on each of its four rows; skip the block when all are clear.
  unsigned present=0;
  for(int j=0;j<4;++j) present|=unsigned(emission_[((y*4+j)*Width+x*4)>>3]>>((x*4)&7))&0xFu;
  if(!present) { glow_[0][y*W+x]=0; continue; }
  int r=0,g=0,b=0;
  for(int j=0;j<4;++j) for(int i=0;i<4;++i) {
   int k=(y*4+j)*Width+x*4+i;
   if(!(emission_[k>>3]&(1u<<(k&7)))) continue;
   uint16_t c=pixels[k];r=std::max(r,int(c>>11));g=std::max(g,int((c>>5)&63));b=std::max(b,int(c&31));
  }
  glow_[0][y*W+x]=uint16_t((r<<11)|(g<<5)|b);
 }
 for(int pass=0;pass<2;++pass) for(int y=0;y<H;++y) for(int x=0;x<W;++x) {
  int r=0,g=0,b=0;
  for(int k=-2;k<=2;++k) {
   int xx=std::max(0,std::min(W-1,x+(pass==0?k:0)));
   int yy=std::max(0,std::min(H-1,y+(pass==1?k:0)));
   int weight=3-std::abs(k);uint16_t c=glow_[pass][yy*W+xx];
   r+=(c>>11)*weight;g+=((c>>5)&63)*weight;b+=(c&31)*weight;
  }
  glow_[1-pass][y*W+x]=uint16_t((((r*7282)>>16)<<11)|(((g*7282)>>16)<<5)|((b*7282)>>16));
 }
}
void Renderer::bloomApply(int y0,int y1) {
 constexpr int W=Width/4,H=Height/4;
 if(!anyEmission_) return;
 for(int y=y0;y<y1;++y) {
  int gy=y>>2,fy=y&3;
  const uint16_t* top=glow_[0]+gy*W;
  const uint16_t* bot=glow_[0]+std::min(H-1,gy+1)*W;
  uint16_t* row=pixels+y*Width;
  // Inside a four pixel group the two glow columns are fixed, so the weighted sum is
  // linear in fx: evaluate the left edge once and step by the difference.
  for(int gx=0;gx<W;++gx) {
   int gx1=std::min(W-1,gx+1);
   uint16_t a=top[gx],b2=top[gx1],c2=bot[gx],d2=bot[gx1];
   if(!(a|b2|c2|d2)) continue;
   int lr=(a>>11)*(4-fy)+(c2>>11)*fy, rr=(b2>>11)*(4-fy)+(d2>>11)*fy;
   int lg=((a>>5)&63)*(4-fy)+((c2>>5)&63)*fy, rg=((b2>>5)&63)*(4-fy)+((d2>>5)&63)*fy;
   int lb=(a&31)*(4-fy)+(c2&31)*fy, rb=(b2&31)*(4-fy)+(d2&31)*fy;
   int r=lr*4,g=lg*4,b=lb*4,dr=rr-lr,dg=rg-lg,db=rb-lb;
   uint16_t* out=row+gx*4;
   for(int fx=0;fx<4;++fx,r+=dr,g+=dg,b+=db) {
    // (v*3277)>>16 matches v/20 over the reachable 0..496 range without a divide.
    uint16_t c=out[fx];
    out[fx]=uint16_t((std::min(31,int(c>>11)+((r*3277)>>16))<<11)|
                     (std::min(63,int((c>>5)&63)+((g*3277)>>16))<<5)|
                      std::min(31,int(c&31)+((b*3277)>>16)));
   }
  }
 }
}
}
