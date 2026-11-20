#pragma once
#include <cmath>
#include "shelly_blu_xiaomi.hpp"
#include "shelly_blu_secure.hpp"
namespace shelly { namespace blu {
// MiBeacon v4/v5 authenticated AES-CCM. Older unauthenticated formats are rejected.
// Format reference: Bluetooth-Devices/xiaomi-ble parser (MIT).
struct MiEnvelope {
  const uint8_t *ciphertext, *mic;
  size_t length;
  uint16_t model;
  uint32_t counter;
  uint8_t nonce[12];
};
inline bool XiaomiMotion2SModel(uint16_t model) {
  return model==0x3531 || model==0x4c60;
}
inline bool QingpingModel(uint16_t model) {
  return model==0x0347 || model==0x0b48 || model==0x03d6 || model==0x0a83;
}
inline bool XiaomiClockModel(int model) {
  return model == 0x16e4 || model == 0x2542;  // LYWSD02MMC o2 and t8 revisions
}
inline bool MiRoleModel(int role, uint16_t model) {
  return role==6 ? model==0x0997 : role<2 ? (model==0x098b || model==0x03d6) :
         role==2 ? (model==0x0a83 || XiaomiMotion2SModel(model)) : (role==3 || role==4) &&
         (XiaomiClockModel(model) || model==0x055b || model==0x0347 || model==0x0b48);
}
struct MiFrame {
  const uint8_t *payload;
  size_t size;
  uint16_t model;
  uint8_t packet;
  bool encrypted;
};
inline bool ParseMiFrame(const uint8_t *data,size_t size,const uint8_t mac[6],MiFrame *out) {
  if (size<5 || size>64) return false;
  uint16_t flags=data[0] | uint16_t(data[1])<<8;
  unsigned version=flags>>12;
  uint16_t model=data[2] | uint16_t(data[3])<<8;
  if (version<2 || version>5 || (flags&0x80) || !(flags&0x40) ||
      (!QingpingModel(model) && !XiaomiClockModel(model) && model!=0x055b && model!=0x098b && model!=0x0997 && !XiaomiMotion2SModel(model))) return false;
  size_t offset=5;
  if (flags&0x10) {
    if(size-offset<6)return false;
    for(size_t i=0;i<6;++i)if(data[offset+i]!=mac[5-i])return false;
    offset+=6;
  }
  if(flags&0x20) {
    if(offset>=size)return false;
    uint8_t capability=data[offset++];
    if(capability&0x20){if(offset>=size)return false;++offset;}
  }
  const bool encrypted=flags&8;
  if(encrypted && (version<4 || size<offset+11))return false;
  if(!encrypted && size<offset+4)return false;
  *out={data+offset,size-offset,model,data[4],encrypted};return true;
}
inline bool ParseMiEnvelope(const uint8_t *data, size_t size,
                            const uint8_t mac[6], MiEnvelope *out) {
  MiFrame frame;
  if(!ParseMiFrame(data,size,mac,&frame) || !frame.encrypted)return false;
  MiEnvelope e={}; e.model=frame.model;e.ciphertext=frame.payload;e.length=frame.size-7;
  e.mic=data+size-4;const uint8_t *counter=data+size-7;
  e.counter=uint32_t(frame.packet) | uint32_t(counter[0])<<8 |
            uint32_t(counter[1])<<16 | uint32_t(counter[2])<<24;
  for(size_t i=0;i<6;++i)e.nonce[i]=mac[5-i];
  memcpy(e.nonce+6,data+2,3);memcpy(e.nonce+9,counter,3);
  *out=e;return true;
}
inline bool FindMiService(const uint8_t *data, size_t size,
                           const uint8_t **service, size_t *length, uint16_t uuid=0xfe95) {
  bool found = false;
  for (size_t at=0; at<size;) {
    size_t n=data[at++]; if (!n) break;
    if (n>size-at) return false;
    if (n>=3 && data[at]==0x16 && ((data[at+1]==0x95 && data[at+2]==0xfe) ||
        (uuid==0 && data[at+1]==0xcd && data[at+2]==0xfd))) {
      if (found) return false;
      *service=data+at+3; *length=n-3; found=true;
    }
    at+=n;
  }
  return found;
}
inline bool FindQingpingService(const uint8_t *data,size_t size,const uint8_t **service,size_t *length) {
  return FindMiService(data,size,service,length,0);
}
inline bool MiObjects(uint16_t model, const uint8_t *plain, size_t size,
                       Reading *out, bool *contact) {
  *contact = false;
  if (MiRoleModel(3, model)) {
    return DecodeThermometerObjects(plain, size, 0, XiaomiClockModel(model), out);
  }
  if (model != 0x098b && model != 0x03d6 && model != 0x0a83 && model != 0x0997 && !XiaomiMotion2SModel(model)) return false;
  Reading r; bool any=false, battery=false, light=false;
  for (size_t at=0; at<size;) {
    if (size-at<3) return false;
    unsigned type=plain[at] | unsigned(plain[at+1])<<8;
    size_t n=plain[at+2]; at+=3;
    if (!n || n>size-at) return false;
    if (type==0x1019 && (model==0x098b || model==0x03d6)) {
      if (*contact || n!=1 || plain[at]>2) return false;
      // 0=open, 1=closed, 2=left open. A magnet means endpoint reached.
      r.window_open=plain[at]!=1; *contact=true; any=true;
    } else if (model==0x0a83 && (type==0x000f || type==0x1007)) {
      if(n!=3 || light || (type==0x000f && *contact))return false;
      uint32_t lux=uint32_t(plain[at]) | uint32_t(plain[at+1])<<8 | uint32_t(plain[at+2])<<16;
      // Reading stores hundredths of lux; all uint24 whole-lux values fit int32.
      r.illuminance100=int32_t(lux*100);light=true;any=true;
      if(type==0x000f){r.motion=true;*contact=true;}
    } else if (model==0x0a83 && type==0x1017) {
      if(n!=4 || *contact)return false;
      uint32_t seconds=uint32_t(plain[at]) | uint32_t(plain[at+1])<<8 |
                       uint32_t(plain[at+2])<<16 | uint32_t(plain[at+3])<<24;
      r.motion=seconds<=30;*contact=true;any=true;
    } else if (XiaomiMotion2SModel(model) && (type==0x4a08 || type==0x4805)) {
      if(n!=4 || light || (type==0x4a08 && *contact))return false;
      uint32_t raw=uint32_t(plain[at]) | uint32_t(plain[at+1])<<8 |
                   uint32_t(plain[at+2])<<16 | uint32_t(plain[at+3])<<24;
      float lux;memcpy(&lux,&raw,sizeof(lux));
      if(!std::isfinite(lux) || lux<0 || lux>21474836.0f)return false;
      r.illuminance100=int32_t(double(lux)*100);light=true;any=true;
      if(type==0x4a08){r.motion=true;*contact=true;}
    } else if (XiaomiMotion2SModel(model) && type==0x4818) {
      if(n!=2 || *contact)return false;
      unsigned seconds=unsigned(plain[at]) | unsigned(plain[at+1])<<8;
      r.motion=seconds<=30;*contact=true;any=true;
    } else if (model==0x0997 && type==0x1001) {
      if(n!=3 || r.button>=0)return false;
      // Authenticated test-button packet is usable for key verification, not
      // as a smoke all-clear or an alarm. It never marks the smoke state seen.
      r.button=plain[at];any=true;
    } else if (model==0x0997 && type==0x1015) {
      if(n!=1 || *contact)return false;
      r.smoke=plain[at]>0;*contact=true;any=true;
    } else if (type==0x100a || type==0x4803) {
      if (battery || n!=1 || plain[at]>100) return false;
      r.battery=plain[at]; battery=true; any=true;
    }
    at+=n;
  }
  if (any) *out=r;
  return any;
}
inline bool DecodeQingpingNative(const uint8_t *data,size_t size,const uint8_t mac[6],
                                 uint16_t *model,Reading *out,bool *state) {
  if(size<11 || size>64 || (data[0]&0x3f)!=8)return false;
  uint16_t id=data[1]==1 || data[1]==7 || data[1]==0x16 ? 0x0347 :
              data[1]==4 ? 0x03d6 : data[1]==0x12 ? 0x0a83 : 0;
  if(!id)return false;
  for(size_t i=0;i<6;++i)if(data[2+i]!=mac[5-i])return false;
  Reading r;*state=false;unsigned fields=0;bool any=false;
  const bool event=data[0]&0x40;
  for(size_t at=8;at<size;) {
    if(size-at<2)return false;
    uint8_t type=data[at++];size_t n=data[at++];
    if(!n || n>size-at)return false;
    unsigned mask=0;
    if(type==1 && id==0x0347) {
      if(n!=4)return false;
      int raw=int(data[at]) | int(data[at+1])<<8;
      r.temperature10=raw>=32768?raw-65536:raw;
      unsigned hum=unsigned(data[at+2]) | unsigned(data[at+3])<<8;
      if(r.temperature10< -1000 || r.temperature10>1000 || hum>1000)return false;
      r.temperature_present=true;r.humidity=(hum+5)/10;mask=1;any=true;
    } else if(type==2) {
      if(n!=1 || (data[at]>100 && data[at]!=255))return false;
      r.battery=data[at]==255?100:data[at];mask=2;any=true;
    } else if(type==4 && id==0x03d6) {
      if(n!=1 || data[at]>2)return false;
      r.window_open=data[at]!=1;*state=true;mask=4;any=true;
    } else if(type==8 && id==0x0a83) {
      if(n!=4 || data[at]>1)return false;
      r.motion=data[at];*state=true;mask=4;any=true;
      if(!event) {
        uint32_t lux=uint32_t(data[at+1]) | uint32_t(data[at+2])<<8 | uint32_t(data[at+3])<<16;
        r.illuminance100=int32_t(lux*100);mask|=8;
      }
    } else if(type==9 && id==0x0a83) {
      if(n!=4)return false;
      uint32_t lux=uint32_t(data[at]) | uint32_t(data[at+1])<<8 |
                   uint32_t(data[at+2])<<16 | uint32_t(data[at+3])<<24;
      if(lux>21474836)return false;
      r.illuminance100=int32_t(lux*100);mask=8;any=true;
    }
    if(fields&mask)return false;
    fields|=mask;at+=n;
  }
  if(!any)return false;
  *model=id;*out=r;return true;
}
inline bool DecodeQingping(const uint8_t *data,size_t size,const uint8_t mac[6],
                            uint16_t *model,Reading *reading,bool *state) {
  MiFrame frame;
  if(!ParseMiFrame(data,size,mac,&frame))return DecodeQingpingNative(data,size,mac,model,reading,state);
  if(frame.encrypted || !QingpingModel(frame.model) ||
     !MiObjects(frame.model,frame.payload,frame.size,reading,state))return false;
  reading->packet_id=frame.packet;*model=frame.model;return true;
}
} }
