#pragma once
#include "scene.h"
#include "hud_font.h"
#include "hud_font_aa.h"
namespace abyss {
// Every interface coordinate is in panel space. Drawing multiplies by UpScale, so the
// layout stays put whatever resolution the world is rendered at.
//
// Blocks of colour - panels, rules, cursors, the survey map - are drawn that way and
// nothing is lost by it. Lettering is not. A face drawn for a 320x240 panel and then
// squared up to fill a 640x480 frame is a staircase, and a glyph whose every pixel is
// either on or off reads as a pixel font however large it gets. The type is drawn from a
// face rendered at the frame's own resolution, kept as coverage and blended, and placed
// by the same interface coordinates - so the layouts are untouched and only the strokes
// change.
inline void hudPlot(uint16_t* pixels,int x,int y,uint16_t color) {
  if(x<0 || y<0 || x>=PanelW || y>=PanelH) return;
  for(int j=0;j<UpScale;++j) {
    uint16_t* row=pixels+(y*UpScale+j)*Width+x*UpScale;
    for(int i=0;i<UpScale;++i) row[i]=color;
  }
}
// One render pixel, mixed into what is already there. Coverage runs 0..255.
inline void hudCover(uint16_t* pixels,int x,int y,uint16_t color,unsigned coverage) {
  if(!coverage || x<0 || y<0 || x>=Width || y>=Height) return;
  uint16_t& dst=pixels[y*Width+x];
  if(coverage>=250) { dst=color; return; }
  int r=(dst>>11)&31,g=(dst>>5)&63,b=dst&31;
  // The step is scaled by 255/256, which at full coverage is a difference of one level
  // in thirty-two and saves a divide on every pixel of every character.
  r+=((int((color>>11)&31)-r)*int(coverage))>>8;
  g+=((int((color>>5)&63)-g)*int(coverage))>>8;
  b+=((int(color&31)-b)*int(coverage))>>8;
  dst=uint16_t((r<<11)|(g<<5)|b);
}
// Interface columns per character. The drawn face is baked to HudAdvance; the 6x12
// bitmap that letters a build drawn at the interface itself has always used seven.
constexpr int HudStep = UpScale==2 ? HudAdvance : 7;
// How wide a string will be, in interface columns.
inline int hudWidth(int characters) { return characters*HudStep; }
inline void hudText(uint16_t* pixels,int x,int y,const char* text,uint16_t color=0xffff) {
 if(!text) return;
 for(;*text;++text,x+=HudStep) {
  unsigned c=static_cast<unsigned char>(*text);if(c<32 || c>126) continue;
  if(UpScale==2) {
   const uint8_t* cell=HudFontAA+(c-32)*HudCellW*HudCellH;
   const int ox=x*UpScale,oy=y*UpScale;
   for(int j=0;j<HudCellH;++j) {
    const uint8_t* row=cell+j*HudCellW;
    for(int i=0;i<HudCellW;++i) if(row[i]) hudCover(pixels,ox+i,oy+j,color,row[i]);
   }
  } else {
   for(int j=0;j<12;++j) for(int i=0;i<8;++i)
    if(HudFont[(c-32)*12+j]&(1<<i)) hudPlot(pixels,x+i,y+j,color);
  }
 }
}
// The same face, drawn smaller, in render pixels rather than interface columns. The
// coverage map is sampled between its own pixels rather than stepped over, so what comes
// out is a smaller letter and not a letter with holes in it. Only the touch controls use
// it: a word inside a disc wants to be a shade smaller than a word on a line of its own,
// and baking a second size of the face for five short labels would be a header nobody
// else ever reads.
inline void hudTextScaled(uint16_t* pixels,int rx,int ry,const char* text,uint16_t color,
                          int num,int den) {
 if(!text || num<=0 || den<=0) return;
 const int advance=HudAdvance*UpScale*num/den;
 const int cw=(HudCellW*num+den-1)/den,ch=(HudCellH*num+den-1)/den;
 for(;*text;++text,rx+=advance) {
  unsigned c=static_cast<unsigned char>(*text); if(c<32 || c>126) continue;
  const uint8_t* cell=HudFontAA+(c-32)*HudCellW*HudCellH;
  for(int j=0;j<ch;++j) {
   const int sy=j*den*256/num,y0=sy>>8,fy=sy&255;
   if(y0>=HudCellH) continue;
   const int y1=(y0+1<HudCellH)?y0+1:y0;
   for(int i=0;i<cw;++i) {
    const int sx=i*den*256/num,x0=sx>>8,fx=sx&255;
    if(x0>=HudCellW) continue;
    const int x1=(x0+1<HudCellW)?x0+1:x0;
    const int a00=cell[y0*HudCellW+x0],a01=cell[y0*HudCellW+x1];
    const int a10=cell[y1*HudCellW+x0],a11=cell[y1*HudCellW+x1];
    const int top=a00+(((a01-a00)*fx)>>8),bottom=a10+(((a11-a10)*fx)>>8);
    const int a=top+(((bottom-top)*fy)>>8);
    if(a>0) hudCover(pixels,rx+i,ry+j,color,unsigned(a));
   }
  }
 }
}
// How wide and how far down a scaled line runs, for placing it. The ink of a glyph sits
// between the third render row of its cell and the twentieth, so the middle of what can
// be seen is eleven and a half rows down before scaling.
inline int hudScaledWidth(int characters,int num,int den) {
 return characters*(HudAdvance*UpScale*num/den);
}
inline int hudScaledMiddle(int num,int den) { return 23*num/(2*den); }
inline void hudRect(uint16_t* pixels,int x,int y,int w,int h,uint16_t color) {
 for(int j=std::max(0,y);j<std::min(PanelH,y+h);++j) {
  for(int p=0;p<UpScale;++p) {
   uint16_t* row=pixels+(j*UpScale+p)*Width;
   for(int i=std::max(0,x)*UpScale;i<std::min(PanelW,x+w)*UpScale;++i) row[i]=color;
  }
 }
}
// A baked picture, one source pixel per panel pixel.
inline void hudBlit(uint16_t* pixels,int x,int y,const uint16_t* source,int w,int h) {
 for(int j=0;j<h;++j) for(int i=0;i<w;++i) hudPlot(pixels,x+i,y+j,source[j*w+i]);
}
// Interface-space shapes that let the sea through. The touch controls sit on the picture
// rather than beside it, so they have to be visible without hiding what is behind them.
inline void hudBlendRect(uint16_t* pixels,int x,int y,int w,int h,uint16_t color,unsigned alpha) {
 for(int j=std::max(0,y);j<std::min(PanelH,y+h);++j)
  for(int i=std::max(0,x);i<std::min(PanelW,x+w);++i)
   for(int p=0;p<UpScale;++p) for(int q=0;q<UpScale;++q)
    hudCover(pixels,i*UpScale+q,j*UpScale+p,color,alpha);
}
// Rings and discs are the one place a circle has to survive the panel's stretch, so they
// are drawn in render pixels and narrowed by the pixel aspect on the way.
inline void hudRing(uint16_t* pixels,int cx,int cy,int radius,uint16_t color,unsigned alpha,int thickness) {
 const float ax=PixelAspect;
 const int rx=int(radius*UpScale*ax)+thickness,ry=radius*UpScale+thickness;
 const int ox=cx*UpScale,oy=cy*UpScale;
 const float outer=float(radius*UpScale+thickness*0.5f),inner=float(radius*UpScale-thickness*0.5f);
 for(int j=-ry;j<=ry;++j) for(int i=-rx;i<=rx;++i) {
  float ex=i/ax,d=std::sqrt(ex*ex+float(j)*j);
  if(d>outer || d<inner) continue;
  hudCover(pixels,ox+i,oy+j,color,alpha);
 }
}
inline void hudDisc(uint16_t* pixels,int cx,int cy,int radius,uint16_t color,unsigned alpha) {
 const float ax=PixelAspect;
 const int rx=int(radius*UpScale*ax)+1,ry=radius*UpScale+1;
 const int ox=cx*UpScale,oy=cy*UpScale;
 const float edge=float(radius*UpScale);
 for(int j=-ry;j<=ry;++j) for(int i=-rx;i<=rx;++i) {
  float ex=i/ax;
  if(ex*ex+float(j)*j>edge*edge) continue;
  hudCover(pixels,ox+i,oy+j,color,alpha);
 }
}
}
