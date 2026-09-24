#pragma once
#include "gamepad.h"
namespace abyss {
// Enumeration events may arrive while a hub/another device is still being inspected.
class UsbAddressQueue {
 uint32_t pending_[4]{};
 public:
 void push(unsigned address) {if(address>0 && address<128) pending_[address/32]|=uint32_t(1)<<(address%32);}
 uint8_t pop() {for(unsigned i=1;i<128;++i) if(pending_[i/32]&(uint32_t(1)<<(i%32))) {pending_[i/32]&=~(uint32_t(1)<<(i%32));return uint8_t(i);}return 0;}
};
bool decodeF310X(const uint8_t* data,size_t bytes,PadState& state);
PadState normalizeF310D(PadState state);
}
