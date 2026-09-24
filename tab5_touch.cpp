// Flying the boat with a finger.
//
// The picture fills the screen, so there are no margins left to put controls in and they
// sit on the sea itself. That makes them part of the frame rather than something painted
// once into the panel's buffer: they are drawn every frame, in interface coordinates, and
// they let the water through so nothing important is ever hidden.
//
// Which controls are drawn depends on what the pilot is doing. Flying wants the two that
// change depth and the one that hurries; a panel wants the two that move through the
// panels; the title screen wants none at all, because it is a picture with two words on
// it. Anything not drawn is not pressable either - a press where no control is listening
// lands on whatever is underneath and is reported as a tap, which is how the lists and
// the yes-or-no questions are worked.
//
// The picture is also the two sticks. A finger that lands on the left half moves the
// boat, one on the right half turns it, each behaving as a stick pushed from wherever it
// first touched down - and while it is down, a ring marks where it landed and a disc
// follows it, so the stick that has no physical existence at least has a visible one.
//
// All of it is published as a PadState, the same shape the USB pad reports, so the menus,
// the panels and the flying code do not know or care which one is being used.
#include "config.h"
#include "gamepad.h"
#include "hud.h"
#if defined(ARDUINO)
#include "lgfx_setup.h"
#include <Arduino.h>
#endif
#include <cstring>
#include <cmath>

namespace abyss {
namespace {

struct Button {
  int16_t x,y,w,h;             // the area a finger has to land in
  uint8_t bit;                 // which pad button it reports as
  const char* label;           // null for a region that is pressable but not drawn
  uint8_t when;                // which modes it belongs to
};
// Interface coordinates: the screen is 1280x720 and the interface is 320x240, so a column
// is four screen pixels across and a row is three down.
//
// Bit 0 is A, bit 1 is B, bits 4 and 5 are the shoulder buttons the pad uses to rise and
// dive, and bit 9 is Start.
//
// The survey map in the top right corner has no pane drawn over it, because it is already
// something to look at: pressing the map is what opens the panels. The panels then carry
// their own pair, which is why nothing has to sit on the sea saying BACK while the pilot
// is flying and there is nothing to go back from.
//
// The drawn ones are discs inscribed in these boxes: a finger is round, and a rectangle
// laid over the sea reads as a hole cut in the picture. Thirty columns by forty rows
// comes out the same distance either way once the panel has stretched it - 120 pixels
// across - and each carries one word, because the second line the panes used to have
// said the same thing twice and was being read as another control.
constexpr Button Buttons[]={
  {  5,132,30,40,4,"UP",   TouchFlying},
  {  5,191,30,40,5,"DOWN", TouchFlying},
  {285,191,30,40,0,"BOOST",TouchFlying},
  {255,  2,62,86,9,nullptr,TouchFlying},
  {285,132,30,40,9,"NEXT", TouchPanels},
  {285,191,30,40,1,"BACK", TouchPanels},
};
constexpr int ButtonCount=sizeof(Buttons)/sizeof(Buttons[0]);

// How far a finger travels for a stick at full deflection, in screen pixels. Short enough
// to reach without lifting the thumb, long enough that small corrections stay small.
constexpr float StickReach=150.0f;
// Below this the finger counts as still, so resting a thumb does not creep the boat.
constexpr float StickDead=12.0f;
// The ring drawn where a finger landed, and the disc that follows it, in interface rows.
constexpr int RingRadius=20,DiscRadius=8;

bool ready=false;
uint32_t held=0;
// Where a finger came down this frame, in interface coordinates, when it came down
// somewhere no control was listening. Good for one frame.
int tapX=-1,tapY=-1;
// Where each stick is being worked, in interface coordinates. A base of -1 means nobody
// is touching that half of the picture.
struct Stick { int baseX=-1,baseY=0,x=0,y=0; };
Stick moveStick,lookStick;

// Screen pixels to interface columns and rows.
inline int toColumn(int screenX) { return screenX*PanelW/ScreenW; }
inline int toRow(int screenY) { return screenY*PanelH/ScreenH; }

// An instrument bezel. A dark field, a fine ring set outside it with a gap between the
// two, and a mark at each quarter crossing that gap - the face of something that came
// down here to take readings, rather than a button off a web page.
//
// It is all drawn in render pixels rather than interface columns, both because a circle
// has to be measured properly and because the panel stretches the frame across, so the
// horizontal distance is divided by the pixel's own aspect before it is measured.
float bezelStep(float a,float b,float x) {
  const float t=clampf((x-a)/(b-a),0,1);
  return t*t*(3-2*t);
}
void drawButton(uint16_t* pixels,const Button& b,bool down) {
  const float radius=b.h*0.5f*UpScale;
  const float cx=(b.x+b.w*0.5f)*UpScale,cy=(b.y+b.h*0.5f)*UpScale;
  constexpr float Reach=1.05f;
  const int x0=int(cx-radius*Reach*PixelAspect)-1,x1=int(cx+radius*Reach*PixelAspect)+1;
  const int y0=int(cy-radius*Reach)-1,y1=int(cy+radius*Reach)+1;
  const uint16_t field=down?0x3c3f:0x1063,ring=down?0xffff:0xbdf7;
  const float fill=down?0.70f:0.46f,bezel=down?1.00f:0.75f;
  for(int y=std::max(0,y0);y<=std::min(Height-1,y1);++y) {
    const float ey=float(y)-cy;
    for(int x=std::max(0,x0);x<=std::min(Width-1,x1);++x) {
      const float ex=(float(x)-cx)/PixelAspect;
      const float d=std::sqrt(ex*ex+ey*ey)/radius;
      if(d>Reach) continue;
      const float body=1-bezelStep(0.86f,0.90f,d);
      if(body>0.004f) hudCover(pixels,x,y,field,unsigned(body*fill*255));
      const float edge=bezelStep(0.93f,0.96f,d)*(1-bezelStep(1.00f,1.03f,d));
      if(edge>0.004f) hudCover(pixels,x,y,ring,unsigned(edge*bezel*255));
    }
  }
  // The four marks, one pixel wide, stepped a pixel at a time so nothing is written twice
  // and blended twice.
  const unsigned mark=unsigned(bezel*185);
  const int ix=int(radius*0.86f*PixelAspect+.5f),ox=int(radius*0.99f*PixelAspect+.5f);
  const int iy=int(radius*0.86f+.5f),oy=int(radius*0.99f+.5f);
  const int mx=int(cx+.5f),my=int(cy+.5f);
  for(int i=ix;i<=ox;++i) { hudCover(pixels,mx+i,my,ring,mark); hudCover(pixels,mx-i,my,ring,mark); }
  for(int j=iy;j<=oy;++j) { hudCover(pixels,mx,my+j,ring,mark); hudCover(pixels,mx,my-j,ring,mark); }
  // The word, four fifths the size the rest of the interface is lettered at: inside a
  // disc it wants to be a shade smaller than it would be on a line of its own.
  constexpr int Num=4,Den=5;
  const int width=hudScaledWidth(int(std::strlen(b.label)),Num,Den);
  hudTextScaled(pixels,mx-width/2,my-hudScaledMiddle(Num,Den),b.label,
                down?0xffff:0xe73c,Num,Den);
}

void drawStick(uint16_t* pixels,const Stick& s) {
  if(s.baseX<0) return;
  hudRing(pixels,s.baseX,s.baseY,RingRadius,0xffff,110,2);
  hudDisc(pixels,s.x,s.y,DiscRadius,0xffff,150);
  hudRing(pixels,s.x,s.y,DiscRadius,0xffff,200,2);
}

float stick(int distance) {
  float d=float(distance);
  if(d>-StickDead && d<StickDead) return 0;
  d=(d<0?d+StickDead:d-StickDead)/(StickReach-StickDead);
  return d<-1?-1:(d>1?1:d);
}

}  // namespace

bool touchAvailable() { return ready; }

bool touchBegin() {
#if defined(ARDUINO)
  ready=M5.Touch.isEnabled();
  if(!ready) { Serial.println("TOUCH: no panel"); return false; }
  Serial.printf("TOUCH: %d controls over the picture, two sticks on the halves of it\n",ButtonCount);
  return true;
#else
  // Off the board there is no panel to read, but the controls still have to be drawn:
  // tools/flight.cpp renders the picture the panel would show, controls and all.
  ready=true;
  return true;
#endif
}

bool touchTap(int& x,int& y) {
  if(tapX<0) return false;
  x=tapX; y=tapY; return true;
}

// Drawn into the frame after the rest of the interface, so the controls sit on top of it.
void touchDraw(uint16_t* pixels,uint8_t mode) {
  if(!ready || !pixels) return;
  for(int i=0;i<ButtonCount;++i) {
    const Button& b=Buttons[i];
    if(!(b.when&mode) || !b.label) continue;
    drawButton(pixels,b,(held&(1u<<b.bit))!=0);
  }
  drawStick(pixels,moveStick);
  drawStick(pixels,lookStick);
}

// Reads whatever the panel is reporting now. M5.update() must have run this frame.
PadState touchSnapshot(uint8_t mode) {
  PadState state;
  tapX=tapY=-1;
  if(!ready) return state;
  uint32_t now=0;
  float mx=0,my=0,lx=0,ly=0;
  Stick nextMove,nextLook;
#if !defined(ARDUINO)
  // No panel off the board, so nothing is ever held and no stick is ever down.
  moveStick=nextMove; lookStick=nextLook; held=0;
  return state;
#else
  const int points=M5.Touch.getCount();
  for(int i=0;i<points;++i) {
    auto finger=M5.Touch.getDetail(i);
    if(!finger.isPressed()) continue;
    const int col=toColumn(finger.base_x),row=toRow(finger.base_y);
    // A control claims the finger for as long as it is held, judged by where it landed:
    // sliding off the edge of a button should not release it mid-manoeuvre.
    bool onButton=false;
    for(int b=0;b<ButtonCount;++b) {
      const Button& r=Buttons[b];
      if(!(r.when&mode)) continue;
      if(col>=r.x && col<r.x+r.w && row>=r.y && row<r.y+r.h) { now|=1u<<r.bit; onButton=true; break; }
    }
    if(onButton) continue;
    // A control that is not listening is not there to be pressed, so the press counts as
    // a tap on whatever is underneath it instead.
    if(finger.wasPressed()) { tapX=col; tapY=row; }
    // Anything else is a stick, and which one depends on the half it started in.
    Stick here;
    here.baseX=col; here.baseY=row;
    here.x=toColumn(finger.x); here.y=toRow(finger.y);
    if(finger.base_x<ScreenW/2) { nextMove=here; mx=stick(finger.distanceX()); my=stick(finger.distanceY()); }
    else { nextLook=here; lx=stick(finger.distanceX()); ly=stick(finger.distanceY()); }
  }
  moveStick=nextMove; lookStick=nextLook;
  state.connected=true;
  state.buttons=now;
  state.pressed=now&~held;
  held=now;
  // Both sticks report the way the pad's do, which is also the way the keyboard's do:
  // the pilot reads -Y as forward and -Ry as looking up, so dragging a finger up the
  // screen sends the boat forward and lifts the view, as pushing a stick up does.
  state.axes[0]=mx;                    // strafe, right is positive
  state.axes[1]=my;                    // forward
  state.axes[3]=lx;                    // yaw, right is positive
  state.axes[4]=ly;                    // pitch
  state.axisMask=0x18;                 // Rx and Ry are present, so the pad path uses them
  state.hat=-1;
  return state;
#endif
}
}  // namespace abyss
