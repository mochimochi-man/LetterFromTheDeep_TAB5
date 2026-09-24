#include "gamepad.h"
#include <algorithm>
#include <cmath>
namespace abyss {
namespace {
int64_t signedValue(uint32_t value,unsigned bits) {
 if(bits && (value&(uint32_t(1)<<(bits-1)))) return int64_t(value)-(int64_t(1)<<bits);
 return value;
}
}
bool HidGamepad::parse(const uint8_t* data,size_t bytes) {
 count_=reportCount_=0;ids_=false;
 struct Guard {int& count;bool accepted=false;~Guard() {if(!accepted) count=0;}} guard{count_};
 if(!data || bytes>1024) return false;
 struct Global {uint32_t page=0,size=0,count=0,id=0;int64_t minimum=0,maximum=0;} g,stack[8];int sp=0;
 uint32_t usages[32]{},low=0,high=0;int usageCount=0;bool ranged=false;
 bool collections[16]{},game=false;int depth=0;uint8_t axes=0;
 auto resetLocal=[&]() {usageCount=0;low=high=0;ranged=false;};
 auto usageAt=[&](unsigned i) {return i<unsigned(usageCount)?usages[i]:(ranged && uint64_t(low)+i<=high?low+i:0u);};
 for(size_t offset=0;offset<bytes;) {
  uint8_t prefix=data[offset++];if(prefix==0xfe) return false;
  unsigned size=prefix&3;if(size==3) size=4;
  if(size>bytes-offset) return false;
  uint32_t value=0;for(unsigned i=0;i<size;++i) value|=uint32_t(data[offset++])<<(8*i);
  unsigned type=(prefix>>2)&3,tag=prefix>>4;
  if(type==1) {
   switch(tag) {
    case 0:g.page=value;break;
    case 1:g.minimum=signedValue(value,size*8);break;
    case 2:g.maximum=g.minimum<0?signedValue(value,size*8):int64_t(value);break;
    case 7:g.size=value;break;
    case 8:if(value==0 || value>255) return false;g.id=value;ids_=true;break;
    case 9:g.count=value;break;
    case 10:if(sp==8) return false;stack[sp++]=g;break;
    case 11:if(sp==0) return false;g=stack[--sp];break;
    default:break;
   }
  } else if(type==2) {
   uint32_t usage=size==4?value:((g.page<<16)|(value&65535));
   if(tag==0) {if(usageCount==32) return false;usages[usageCount++]=usage;}
   else if(tag==1) {low=usage;ranged=true;}
   else if(tag==2) high=usage;
  } else if(type==0) {
   if(tag==10) {
    if(depth==16) return false;
    collections[depth++]=game;
    uint32_t usage=usageAt(0);game=game || (value==1 && (usage==0x10004 || usage==0x10005));
   } else if(tag==12) {if(depth==0) return false;game=collections[--depth];}
   else if(tag==8) {
    if(g.size>32 || g.count>512 || uint64_t(g.size)*g.count>512) return false;
    int report=0;while(report<reportCount_ && reports_[report].id!=g.id) ++report;
    if(report==reportCount_) {if(reportCount_==16) return false;reports_[reportCount_++]={0,uint8_t(g.id)};}
    unsigned bit=reports_[report].bits,total=g.size*g.count;if(bit+total>(ids_?504u:512u)) return false;
    for(unsigned i=0;game && g.size && i<g.count;++i) {
     uint32_t usage=usageAt(i),page=usage>>16,id=usage&65535;
     bool axis=page==1 && id>=0x30 && id<=0x35;
     bool supported=axis || (page==1 && id==0x39) || (page==9 && id>=1 && id<=32);
     if(!(value&1) && (value&2) && !(value&4) && supported) {
      if(count_==64 || g.maximum<=g.minimum) return false;
      fields_[count_++]={g.minimum,g.maximum,uint16_t(bit+i*g.size),uint16_t(id),uint8_t(g.size),uint8_t(g.id),uint8_t(page)};
      if(axis) axes|=uint8_t(1u<<(id-0x30));
     }
    }
    reports_[report].bits=uint16_t(bit+total);
   }
   resetLocal();
  }
 }
 if(depth || sp || !(axes&1) || !(axes&2)) {count_=0;return false;}
 if(ids_) for(int i=0;i<reportCount_;++i) if(reports_[i].id==0 && reports_[i].bits) {count_=0;return false;}
 guard.accepted=true;return true;
}
bool HidGamepad::decode(const uint8_t* data,size_t bytes,PadState& state) const {
 if(!count_ || !data || !bytes) return false;
 uint8_t id=ids_?*data++:0;if(ids_) --bytes;
 int report=0;while(report<reportCount_ && reports_[report].id!=id) ++report;
 if(report==reportCount_ || bytes<(reports_[report].bits+7u)/8) return false;
 PadState next=state;bool changed=false;
 for(int i=0;i<count_;++i) {
  const auto& f=fields_[i];if(f.report!=id) continue;
  uint32_t raw=0;for(unsigned b=0;b<f.size;++b) raw|=uint32_t((data[(f.bit+b)/8]>>((f.bit+b)%8))&1)<<b;
  int64_t value=f.minimum<0?signedValue(raw,f.size):int64_t(raw);changed=true;
  if(f.page==9) {uint32_t mask=uint32_t(1)<<(f.usage-1);if(value) next.buttons|=mask;else next.buttons&=~mask;}
  else if(f.usage==0x39) next.hat=(value>=f.minimum && value<=f.maximum && value-f.minimum<8)?int8_t(value-f.minimum):-1;
  else {
   float axis=float(2.0*(double(value)-double(f.minimum))/double(f.maximum-f.minimum)-1.0);
   axis=std::max(-1.f,std::min(1.f,axis));
   float magnitude=std::abs(axis);axis=magnitude<=.12f?0:std::copysign((magnitude-.12f)/.88f,axis);
   next.axes[f.usage-0x30]=axis;next.axisMask|=uint8_t(1u<<(f.usage-0x30));
  }
 }
 if(changed) {next.connected=true;state=next;}
 return changed;
}
}
