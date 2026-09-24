#include "f310.h"
#include <cmath>
namespace abyss {
bool decodeF310X(const uint8_t* data,size_t bytes,PadState& state) {
 if(!data || bytes<20 || data[0]!=0 || data[1]!=20) return false;
 PadState next;next.connected=true;next.axisMask=0x1b;
 auto axis=[&](unsigned offset) {
  unsigned raw=unsigned(data[offset])|(unsigned(data[offset+1])<<8);
  int value=raw>=32768?int(raw)-65536:int(raw);
  float x=value<0?value/32768.f:value/32767.f,magnitude=std::abs(x);
  return magnitude<=.12f?0.f:std::copysign((magnitude-.12f)/.88f,x);
 };
 next.axes[0]=axis(6);next.axes[1]=-axis(8);next.axes[3]=axis(10);next.axes[4]=-axis(12);
 unsigned raw=data[2]|(unsigned(data[3])<<8);
 const unsigned source[]={12,13,14,15,8,9,4,5,6,7},target[]={0,1,2,3,4,5,9,8,10,11};
 for(unsigned i=0;i<10;++i) if(raw&(1u<<source[i])) next.buttons|=1u<<target[i];
 if(data[4]>96) next.buttons|=1u<<6;
 if(data[5]>96) next.buttons|=1u<<7;
 int x=((raw&8)?1:0)-((raw&4)?1:0),y=((raw&1)?1:0)-((raw&2)?1:0);
 if(x || y) {const int8_t hat[3][3]={{5,4,3},{6,-1,2},{7,0,1}};next.hat=hat[y+1][x+1];}
 state=next;return true;
}
PadState normalizeF310D(PadState state) {
 // Logitech DirectInput face buttons are X,A,B,Y; expose the common A,B,X,Y order.
 unsigned face=state.buttons&7;state.buttons&=~7u;
 state.buttons|=((face&2)>>1)|((face&4)>>1)|((face&1)<<2);
 return state;
}
}
