#include "gamepad.h"
namespace abyss {
// A USB boot keyboard sends a fixed eight byte report: a modifier byte, a reserved
// byte and up to six key codes, with 1..3 reserved for roll-over errors. Turning it
// into a PadState here means nothing above the USB layer has to know which of the two
// controls is plugged in, and it can be checked on the host without hardware.
//   W S / up down   forward and back      A D / left right  strafe
//   I K             pitch                 J L               yaw
//   Q E             rise and dive         space             A: confirm, boost
//   esc backspace   B: cancel             enter tab         start: next panel
PadState decodeBootKeyboard(const uint8_t* report,size_t bytes) {
 PadState state;state.connected=true;state.hat=-1;state.axisMask=0x1b;
 if(!report || bytes<3) return state;
 bool down[232]={};
 for(size_t i=2;i<bytes && i<8;++i) if(report[i]>3 && report[i]<232) down[report[i]]=true;
 auto pair=[&](uint8_t neg,uint8_t neg2,uint8_t pos,uint8_t pos2){
  return float((down[pos]||down[pos2])?1:0)-float((down[neg]||down[neg2])?1:0);
 };
 state.axes[0]=pair(Key::A,Key::Left,Key::D,Key::Right);
 state.axes[1]=pair(Key::W,Key::Up,Key::S,Key::Down);      // the pilot reads -Y as forward
 state.axes[3]=pair(Key::J,Key::J,Key::L,Key::L);
 state.axes[4]=pair(Key::I,Key::I,Key::K,Key::K);          // the pilot reads -Ry as up
 uint32_t buttons=0;
 if(down[Key::Space]) buttons|=1u;
 if(down[Key::Escape]||down[Key::Backspace]) buttons|=1u<<1;
 if(down[Key::Q]) buttons|=1u<<4;
 if(down[Key::E]) buttons|=1u<<5;
 if(down[Key::Enter]||down[Key::Tab]) buttons|=1u<<9;
 state.buttons=buttons;
 return state;
}
}
