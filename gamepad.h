#pragma once
#include <cstddef>
#include <cstdint>
namespace abyss {
struct PadState {
 float axes[6]{}; // X,Y,Z,Rx,Ry,Rz, normalized -1..1
 uint32_t buttons=0,pressed=0;uint8_t axisMask=0;int8_t hat=-1;bool connected=false;
};
// Preserve brief button taps even when rendering is slower than USB input polling.
class PadLatch {
 public:
  void update(const PadState& next) {
    if(next.connected) presses_|=next.buttons&~state_.buttons;else presses_=0;
    state_=next;
  }
  PadState take(bool fresh=true) {
    PadState result=fresh?state_:PadState{};
    if(fresh) result.pressed=presses_;
    presses_=0;return result;
  }
 private:
  PadState state_;uint32_t presses_=0;
};
class HidGamepad {
 public:
  bool parse(const uint8_t* descriptor,size_t bytes);
  bool decode(const uint8_t* report,size_t bytes,PadState& state) const;
 private:
  struct Field {int64_t minimum,maximum;uint16_t bit,usage;uint8_t size,report,page;};
  struct Report {uint16_t bits=0;uint8_t id=0;};
  Field fields_[64]{};Report reports_[16]{};int count_=0,reportCount_=0;bool ids_=false;
};
// USB HID usage ids for the keys the game binds, so the mapping reads as key names.
namespace Key {
enum : uint8_t {
 A=0x04,D=0x07,E=0x08,I=0x0c,J=0x0d,K=0x0e,L=0x0f,Q=0x14,S=0x16,W=0x1a,
 Enter=0x28,Escape=0x29,Backspace=0x2a,Tab=0x2b,Space=0x2c,
 Right=0x4f,Left=0x50,Down=0x51,Up=0x52
};
}
// A USB keyboard is the second way in. Boot protocol only: eight bytes, six key codes.
PadState decodeBootKeyboard(const uint8_t* report,size_t bytes);
// Set while a binary frame is going out, so the USB task does not interleave log lines.
extern volatile bool serialQuiet;
bool startGamepadHost();
PadState gamepadSnapshot();
// Devices on the USB host, whatever they are. Zero means nobody can play: the title
// drops its menu and the tour runs itself - unless the panel itself can be touched.
int usbDeviceCount();
// The Tab5's screen, reported in the same shape as a pad so that nothing above this
// layer has to know which of the three is being used. The buttons are drawn into the
// black margins beside the picture; the picture itself is the two sticks.
bool touchBegin();
bool touchAvailable();
// Which set of controls is live. The title screen passes neither, so nothing is drawn
// over it and nothing there is pressable: a press lands on the picture as a tap instead,
// which is how its menu and every yes-or-no question are answered.
enum : uint8_t { TouchFlying = 1, TouchPanels = 2 };
PadState touchSnapshot(uint8_t mode);
// Draws the controls into the frame. They sit on the picture now, so this belongs with
// the rest of the interface rather than being painted once at start-up.
void touchDraw(uint16_t* pixels,uint8_t mode);
// Where a finger came down this frame, in interface coordinates, if it came down
// somewhere no control was listening. Good for the frame it happened in.
bool touchTap(int& x,int& y);
}
