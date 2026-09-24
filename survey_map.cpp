#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include "survey_map.h"
#include "journal.h"
#include <cstring>
namespace abyss {
namespace {
uint16_t dim(uint16_t c) {return uint16_t((((c>>11)&31)/5)<<11 | (((c>>5)&63)/5)<<5 | (c&31)/5);}
}
void SurveyMap::build(const Scene& scene) {
 for(int z=0;z<Cells;++z) for(int x=0;x<Cells;++x) {
#if defined(ARDUINO)
  if(x==0) delay(1);
#endif
  float wx=WorldMin+x*8+4,wz=WorldMin+z*8+4;
  auto env=scene.environment({wx,0,wz});
  float ground=scene.surface(wx,wz),height=scene.surface(wx,wz,true)-ground;
  const Color colors[]={{57,185,181},{235,197,113},{150,144,205},{128,193,108},{92,129,211},{112,199,221}};
  Color color=colors[env.zone];
  if(height>5) color=scale(color,.65f);
  if(scene.city && height>4) color={190,230,238};
  terrain_[z*Cells+x]=rgb565(color);
 }
}
bool SurveyMap::visited(int x,int z,bool city) const {
 if(x<0 || z<0 || x>=Cells || z>=Cells) return false;
 int bit=z*Cells+x;return explored_[city?1:0][bit/8]&(1u<<(bit%8));
}
bool SurveyMap::visit(Vec3 eye,bool city) {
 if(!std::isfinite(eye.x) || !std::isfinite(eye.z) || eye.x<WorldMin || eye.z<WorldMin || eye.x>=WorldMin+WorldSize || eye.z>=WorldMin+WorldSize) return false;
 int cx=int((eye.x-WorldMin)/8),cz=int((eye.z-WorldMin)/8);bool changed=false;
 for(int z=std::max(0,cz-2);z<=std::min(Cells-1,cz+2);++z) for(int x=std::max(0,cx-2);x<=std::min(Cells-1,cx+2);++x) {
  float dx=WorldMin+x*8+4-eye.x,dz=WorldMin+z*8+4-eye.z;
  if(dx*dx+dz*dz>12*12 || visited(x,z,city)) continue;
  int bit=z*Cells+x;explored_[city?1:0][bit/8]|=uint8_t(1u<<(bit%8));changed=true;
 }
 return changed;
}
void SurveyMap::draw(uint16_t* pixels,int x0,int width,int height,const Camera& camera,bool city,uint32_t monuments) const {
 if(!pixels || width<40 || height<40) return;
 // The frame reaches the panel stretched across, so a square drawn in interface
 // coordinates would arrive as a wide rectangle. The map is the one thing on screen
 // that has to be square, so it is drawn narrower by exactly what the panel adds back.
 int sizeY=expanded?std::min(height-32,int((width-16)/PixelAspect))
                   :std::min(80,int((width/4)/PixelAspect));
 int sizeX=int(sizeY*PixelAspect);
 int left=x0+(expanded?(width-sizeX)/2:width-sizeX-5),top=expanded?16:5;
 // Panel coordinates in; each one covers an UpScale square of the render.
 auto pixel=[&](int x,int y,uint16_t c) {
  if(x<0 || y<0 || x>=PanelW || y>=height) return;
  for(int j=0;j<UpScale;++j) {
   uint16_t* row=pixels+(y*UpScale+j)*Width+x*UpScale;
   for(int i=0;i<UpScale;++i) row[i]=c;
  }
 };
 // Twenty large square cells replace the one-pixel terrain noise.
 constexpr int Blocks=20,Group=Cells/Blocks;
 for(int bz=0;bz<Blocks;++bz) for(int bx=0;bx<Blocks;++bx) {
  bool seen=false;
  for(int z=0;z<Group;++z) for(int x=0;x<Group;++x) seen|=visited(bx*Group+x,bz*Group+z,city);
  uint16_t c=terrain_[(bz*Group+Group/2)*Cells+bx*Group+Group/2];
  if(!seen) c=dim(c);
  for(int y=bz*sizeY/Blocks;y<(bz+1)*sizeY/Blocks;++y)
   for(int x=bx*sizeX/Blocks;x<(bx+1)*sizeX/Blocks;++x) pixel(left+x,top+sizeY-1-y,c);
 }
 auto marker=[&](Vec3 p,uint16_t color,int radius) {
  int mx=int((p.x-WorldMin)/8),mz=int((p.z-WorldMin)/8);
  if(!visited(mx,mz,city)) return;
  int px=left+int((p.x-WorldMin)*sizeX/WorldSize),py=top+sizeY-1-int((p.z-WorldMin)*sizeY/WorldSize);
  for(int y=-radius;y<=radius;++y) for(int x=-radius;x<=radius;++x)
   if(std::abs(x)+std::abs(y)<=radius && px+x>=left && py+y>=top && px+x<left+sizeX && py+y<top+sizeY) pixel(px+x,py+y,color);
 };
 if(!city) {for(const auto& m:Journal::catalog) if(monuments&(1u<<m.id)) marker(m.position,0xff29,expanded?2:1);}
 else {marker({0,0,35},0xff29,expanded?3:1);marker({-12,0,-86},0x7fff,expanded?3:1);}
 if(!std::isfinite(camera.position.x) || !std::isfinite(camera.position.z)) return;
 int px=left+int(clampf((camera.position.x-WorldMin)*sizeX/WorldSize,1,float(sizeX-2)));
 int py=top+sizeY-1-int(clampf((camera.position.z-WorldMin)*sizeY/WorldSize,1,float(sizeY-2)));
 float norm=std::sqrt(camera.forward.x*camera.forward.x+camera.forward.z*camera.forward.z);
 float dx=norm>.001f?camera.forward.x/norm:0,dz=norm>.001f?-camera.forward.z/norm:-1;
 // A filled arrowhead reads at a glance; the dark hull keeps it legible over the bright sand.
 const float len=expanded?9.f:6.f,half=expanded?5.5f:3.6f,back=expanded?3.5f:2.2f;
 auto fill=[&](const float* vx,const float* vy,uint16_t color) {
  float lox=std::fmin(vx[0],std::fmin(vx[1],vx[2])),hix=std::fmax(vx[0],std::fmax(vx[1],vx[2]));
  float loy=std::fmin(vy[0],std::fmin(vy[1],vy[2])),hiy=std::fmax(vy[0],std::fmax(vy[1],vy[2]));
  for(int y=std::max(top,int(std::floor(loy)));y<=std::min(top+sizeY-1,int(std::ceil(hiy)));++y)
   for(int x=std::max(left,int(std::floor(lox)));x<=std::min(left+sizeX-1,int(std::ceil(hix)));++x) {
    float sx=x+.5f,sy=y+.5f,inside=1;
    for(int e=0;e<3;++e) {
     int f=(e+1)%3;
     if((vx[f]-vx[e])*(sy-vy[e])-(vy[f]-vy[e])*(sx-vx[e])<0) {inside=0;break;}
    }
    if(inside) pixel(x,y,color);
   }
 };
 auto arrowhead=[&](float grow,uint16_t color) {
  float vx[3]={px+dx*(len+grow)*PixelAspect,
               px+(-dx*(back+grow)-dz*(half+grow))*PixelAspect,
               px+(-dx*(back+grow)+dz*(half+grow))*PixelAspect};
  float vy[3]={py+dz*(len+grow),py-dz*(back+grow)+dx*(half+grow),py-dz*(back+grow)-dx*(half+grow)};
  fill(vx,vy,color);
 };
 arrowhead(1.4f,0x0000);
 arrowhead(0,0xffe0);
 // A yellow wedge alone does not say which end is the nose, so the nose is red.
 {
  const float nose=expanded?.66f:.72f;
  float tipX=px+dx*len*PixelAspect,tipY=py+dz*len;
  float leftX=px+(-dx*back-dz*half)*PixelAspect,leftY=py-dz*back+dx*half;
  float rightX=px+(-dx*back+dz*half)*PixelAspect,rightY=py-dz*back-dx*half;
  float vx[3]={tipX,tipX+(leftX-tipX)*nose,tipX+(rightX-tipX)*nose};
  float vy[3]={tipY,tipY+(leftY-tipY)*nose,tipY+(rightY-tipY)*nose};
  fill(vx,vy,0xf800);
 }
 // No border, compass tick or radar decoration. Positive Z is upward, matching the camera right-handed view.
}
std::array<uint8_t,SurveyMap::SaveBytes> SurveyMap::encode() const {
 std::array<uint8_t,SaveBytes> out{};out[0]='A';out[1]='B';out[2]='M';out[3]=1;
 std::memcpy(out.data()+4,explored_,sizeof(explored_));
 uint32_t checksum=2166136261u;for(int i=0;i<SaveBytes-4;++i) checksum=(checksum^out[i])*16777619u;
 for(int i=0;i<4;++i) out[SaveBytes-4+i]=uint8_t(checksum>>(i*8));
 return out;
}
bool SurveyMap::decode(const uint8_t* in,size_t size) {
 if(!in || size!=SaveBytes || in[0]!='A' || in[1]!='B' || in[2]!='M' || in[3]!=1) return false;
 uint32_t checksum=2166136261u,stored=0;for(int i=0;i<SaveBytes-4;++i) checksum=(checksum^in[i])*16777619u;
 for(int i=0;i<4;++i) stored|=uint32_t(in[SaveBytes-4+i])<<(i*8);
 if(checksum!=stored) return false;
 std::memcpy(explored_,in+4,sizeof(explored_));return true;
}
}
