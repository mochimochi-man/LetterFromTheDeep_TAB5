#include "config.h"
#include "gamepad.h"
#include "f310.h"
#if defined(ARDUINO) && ABYSS_ENABLE_USB_GAMEPAD
#include <Arduino.h>
#include "usb/usb_host.h"
// On the ESP32-S3 the console and the host share the one native USB PHY, so enabling CDC
// on boot takes the socket the pad plugs into. The P4 has no such conflict: the console
// is the separate USB-Serial/JTAG peripheral on the USB-C socket, while the host runs on
// the high-speed OTG controller behind the USB-A socket, and both are wanted at once.
#if ARDUINO_USB_CDC_ON_BOOT && !defined(CONFIG_IDF_TARGET_ESP32P4)
#error "USB gamepad host needs CDC On Boot disabled; use the board's USB-UART port."
#endif
namespace abyss {
volatile bool serialQuiet=false;
// Read by the title screen to decide whether anyone is able to play.
static volatile int attached=0;
namespace {
usb_host_client_handle_t client=nullptr;
usb_device_handle_t device=nullptr;
usb_transfer_t *control=nullptr,*input=nullptr;
HidGamepad parser;
PadState working;
PadLatch samples;
portMUX_TYPE sampleLock=portMUX_INITIALIZER_UNLOCKED;
uint32_t lastReport=0;
bool retainF310State=false;
UsbAddressQueue addresses;
uint8_t endpoint=0,interfaceNumber=0;
bool f310X=false,f310D=false;
uint32_t reportCount=0;
int packetSize=0,descriptorSize=0;
bool claimed=false,gone=false,controlPending=false,inputPending=false,ready=false,flushed=false;
// A USB keyboard is a second way in, held in its own slot so a pad and a keyboard can
// both be on the bus at once. Boot protocol, so no report descriptor is parsed.
usb_device_handle_t kbDevice=nullptr;
usb_transfer_t *kbControl=nullptr,*kbInput=nullptr;
PadLatch kbSamples;
portMUX_TYPE kbLock=portMUX_INITIALIZER_UNLOCKED;
uint8_t kbEndpoint=0,kbInterface=0;
int kbPacket=0;
bool kbClaimed=false,kbGone=false,kbControlPending=false,kbInputPending=false,kbReady=false,kbFlushed=false;
void publish(const PadState& value) {
 portENTER_CRITICAL(&sampleLock);samples.update(value);retainF310State=value.connected && (f310X || f310D);lastReport=millis();portEXIT_CRITICAL(&sampleLock);
}
void publishKeys(const PadState& value) {
 portENTER_CRITICAL(&kbLock);kbSamples.update(value);portEXIT_CRITICAL(&kbLock);
}
void event(const usb_host_client_event_msg_t* message,void*) {
 if(message->event==USB_HOST_CLIENT_EVENT_NEW_DEV) addresses.push(message->new_dev.address);
 if(message->event==USB_HOST_CLIENT_EVENT_DEV_GONE) {
  if(device==message->dev_gone.dev_hdl) {gone=true;publish({});}
  if(kbDevice==message->dev_gone.dev_hdl) {kbGone=true;publishKeys({});}
 }
}
void keyReportDone(usb_transfer_t* transfer) {
 kbInputPending=false;
 if(kbGone) return;
 if(transfer->status!=USB_TRANSFER_STATUS_COMPLETED) {kbGone=true;publishKeys({});return;}
 publishKeys(decodeBootKeyboard(transfer->data_buffer,size_t(transfer->actual_num_bytes)));
}
void protocolDone(usb_transfer_t* transfer) {
 kbControlPending=false;
 if(kbGone) return;
 // A keyboard that stalls SET_PROTOCOL is left in report protocol, whose report still
 // begins with the same modifier byte and key codes, so play carries on either way.
 if(transfer->status!=USB_TRANSFER_STATUS_COMPLETED) Serial.println("KEY: SET_PROTOCOL refused, reading the report as sent");
 kbReady=true;Serial.println("KEY: keyboard ready (WASD move, IJKL look, Q/E rise, space select, enter panel)");
}
void closeKeyboard() {
 if(!kbDevice) return;
 if(kbInputPending && !kbFlushed) {
  usb_host_endpoint_halt(kbDevice,kbEndpoint);usb_host_endpoint_flush(kbDevice,kbEndpoint);kbFlushed=true;
 }
 if(kbInputPending || kbControlPending) return;
 if(kbInput) {usb_host_transfer_free(kbInput);kbInput=nullptr;}
 if(kbControl) {usb_host_transfer_free(kbControl);kbControl=nullptr;}
 if(kbClaimed) usb_host_interface_release(client,kbDevice,kbInterface);
 usb_host_device_close(client,kbDevice);
 kbDevice=nullptr;kbClaimed=kbReady=kbGone=kbFlushed=false;publishKeys({});
 Serial.println("KEY: keyboard detached");
}
// Take the already-open handle if this device offers a boot keyboard interface. The
// caller gives up its own reference when this returns true.
bool adoptKeyboard(const usb_config_desc_t* config) {
 const auto* data=reinterpret_cast<const uint8_t*>(config);size_t length=config->wTotalLength;
 uint8_t face=0,ep=0;int size=0;bool candidate=false;
 for(size_t pos=0;pos+2<=length;) {
  unsigned n=data[pos],type=data[pos+1];if(n<2 || n>length-pos) return false;
  const uint8_t* d=data+pos;
  if(type==4 && n>=9) {
   if(ep) break;
   candidate=d[5]==3 && d[6]==1 && d[7]==1;      // HID, boot subclass, keyboard
   if(candidate) face=d[2];
  } else if(candidate && type==5 && n>=7 && (d[2]&0x80) && (d[3]&3)==3) {
   unsigned packet=d[4]|(unsigned(d[5])<<8);
   if(packet>0 && packet<=64) {ep=d[2];size=int(packet);}
  }
  pos+=n;
 }
 if(!ep) return false;
 kbDevice=device;kbInterface=face;kbEndpoint=ep;kbPacket=size;
 kbGone=kbReady=kbClaimed=kbFlushed=false;kbInputPending=kbControlPending=false;
 if(usb_host_interface_claim(client,kbDevice,kbInterface,0)!=ESP_OK) {kbGone=true;return true;}
 kbClaimed=true;
 if(usb_host_transfer_alloc(size_t(kbPacket),0,&kbInput)!=ESP_OK) {kbGone=true;return true;}
 kbInput->device_handle=kbDevice;kbInput->bEndpointAddress=kbEndpoint;
 kbInput->num_bytes=kbPacket;kbInput->callback=keyReportDone;
 if(usb_host_transfer_alloc(8,0,&kbControl)!=ESP_OK) {kbGone=true;return true;}
 const uint8_t setup[]={0x21,0x0b,0,0,kbInterface,0,0,0};   // SET_PROTOCOL(boot)
 memcpy(kbControl->data_buffer,setup,8);
 kbControl->device_handle=kbDevice;kbControl->bEndpointAddress=0;
 kbControl->num_bytes=8;kbControl->callback=protocolDone;
 if(usb_host_transfer_submit_control(client,kbControl)==ESP_OK) kbControlPending=true;else kbGone=true;
 return true;
}
void reportDone(usb_transfer_t* transfer) {
 inputPending=false;
 if(gone) return;
 if(transfer->status!=USB_TRANSFER_STATUS_COMPLETED) {gone=true;publish({});return;}
 bool valid=f310X?decodeF310X(transfer->data_buffer,size_t(transfer->actual_num_bytes),working):parser.decode(transfer->data_buffer,size_t(transfer->actual_num_bytes),working);
 if(valid) {
  PadState state=f310D?normalizeF310D(working):working;publish(state);
  if(reportCount++==0) Serial.printf("PAD: first valid %s report, %d bytes\n",f310X?"F310 XInput":f310D?"F310 DirectInput":"HID",transfer->actual_num_bytes);
 }
}
void descriptorDone(usb_transfer_t* transfer) {
 controlPending=false;if(gone) return;
 if(transfer->status!=USB_TRANSFER_STATUS_COMPLETED || transfer->actual_num_bytes<8 ||
    !parser.parse(transfer->data_buffer+8,size_t(transfer->actual_num_bytes-8))) {
  Serial.println("PAD: unsupported or invalid HID report descriptor");gone=true;return;
 }
 ready=true;Serial.println("PAD: HID gamepad ready (Start toggles manual tour)");
}
void closeDevice() {
 if(!device) return;
 // A canceled transfer still owns its buffer until its completion callback has run.
 if(inputPending && !flushed) {
  usb_host_endpoint_halt(device,endpoint);usb_host_endpoint_flush(device,endpoint);flushed=true;
 }
 if(inputPending || controlPending) return;
 if(input) {usb_host_transfer_free(input);input=nullptr;}
 if(control) {usb_host_transfer_free(control);control=nullptr;}
 if(claimed) usb_host_interface_release(client,device,interfaceNumber);
 usb_host_device_close(client,device);
 device=nullptr;claimed=ready=gone=flushed=false;working={};publish({});
 Serial.println("PAD: detached / manual thrust neutral");
}
void openDevice(uint8_t addr) {
 if(usb_host_device_open(client,addr,&device)!=ESP_OK) {device=nullptr;return;}
 const usb_device_desc_t* identity=nullptr;
 if(usb_host_get_device_descriptor(device,&identity)!=ESP_OK) {gone=true;return;}
 Serial.printf("USB: addr=%u VID=%04x PID=%04x class=%02x\n",addr,identity->idVendor,identity->idProduct,identity->bDeviceClass);
 if(identity->bDeviceClass==9) {
  Serial.println("USB: hub managed by SDK; waiting for downstream devices");
  usb_host_device_close(client,device);device=nullptr;return;
 }
 f310X=identity->idVendor==0x046d && identity->idProduct==0xc21d;
 f310D=identity->idVendor==0x046d && identity->idProduct==0xc216;
 reportCount=0;working={};
 const usb_config_desc_t* config=nullptr;
 if(usb_host_get_active_config_descriptor(device,&config)!=ESP_OK) {gone=true;return;}
 // Hand a keyboard over to its own slot and leave the pad slot free for a pad.
 if(!f310X && !f310D && !kbDevice && adoptKeyboard(config)) {device=nullptr;return;}
 endpoint=0;packetSize=descriptorSize=0;bool candidate=false;
 const auto* data=reinterpret_cast<const uint8_t*>(config);size_t length=config->wTotalLength;
 for(size_t pos=0;pos+2<=length;) {
  unsigned n=data[pos],type=data[pos+1];if(n<2 || n>length-pos) {gone=true;return;}
  const uint8_t* d=data+pos;
  if(type==4 && n>=9) {
   if(endpoint && (descriptorSize || f310X)) break;
   candidate=d[3]==0 && (f310X?(d[5]==0xff && d[6]==0x5d && d[7]==1):(d[5]==3 && d[7]==0));
   if(candidate) {interfaceNumber=d[2];endpoint=0;descriptorSize=0;}
  } else if(candidate && type==0x21 && n>=9) {
   for(unsigned i=0;i<d[5] && 6+3*i+2<n;++i) if(d[6+3*i]==0x22) descriptorSize=d[7+3*i]|(unsigned(d[8+3*i])<<8);
  } else if(candidate && type==5 && n>=7 && (d[2]&0x80) && (d[3]&3)==3) {
   unsigned size=d[4]|(unsigned(d[5])<<8);
   if(size>0 && size<=64) {endpoint=d[2];packetSize=int(size);}
  }
  pos+=n;
 }
 if(!endpoint || (!f310X && (descriptorSize<=0 || descriptorSize>1024))) {Serial.println("PAD: no supported interrupt HID interface");gone=true;return;}
 if(usb_host_interface_claim(client,device,interfaceNumber,0)!=ESP_OK) {gone=true;return;}
 claimed=true;
 if(usb_host_transfer_alloc(size_t(packetSize),0,&input)!=ESP_OK) {gone=true;return;}
 input->device_handle=device;input->bEndpointAddress=endpoint;input->num_bytes=packetSize;input->callback=reportDone;
 if(f310X) {ready=true;Serial.println("PAD: F310 XInput ready");return;}
 if(usb_host_transfer_alloc(size_t(descriptorSize)+8,0,&control)!=ESP_OK) {gone=true;return;}
 uint8_t setup[]={0x81,6,0,0x22,interfaceNumber,0,uint8_t(descriptorSize),uint8_t(descriptorSize>>8)};
 memcpy(control->data_buffer,setup,8);
 control->device_handle=device;control->bEndpointAddress=0;control->num_bytes=descriptorSize+8;control->callback=descriptorDone;
 input->device_handle=device;input->bEndpointAddress=endpoint;input->num_bytes=packetSize;input->callback=reportDone;
 if(usb_host_transfer_submit_control(client,control)==ESP_OK) controlPending=true;else gone=true;
}
void hostTask(void*) {
 uint32_t lastStatus=0;unsigned statusLines=0;uint32_t lastCensus=0;
 for(;;) {
  uint32_t events=0;usb_host_lib_handle_events(0,&events);
  usb_host_client_handle_events(client,0);
  if(gone) closeDevice();
  if(kbGone) closeKeyboard();
  if(!device || !kbDevice) {uint8_t next=addresses.pop();if(next) openDevice(next);}
  if(ready && !gone && !inputPending) {
   if(usb_host_transfer_submit(input)==ESP_OK) inputPending=true;else {gone=true;publish({});}
  }
  if(kbReady && !kbGone && !kbInputPending) {
   if(usb_host_transfer_submit(kbInput)==ESP_OK) kbInputPending=true;else {kbGone=true;publishKeys({});}
  }
  if(uint32_t(millis()-lastCensus)>=250) {
   lastCensus=millis(); usb_host_lib_info_t census{};
   if(usb_host_lib_info(&census)==ESP_OK) attached=census.num_devices;
  }
  if(statusLines<12 && uint32_t(millis()-lastStatus)>=5000) {
   lastStatus=millis();++statusLines;usb_host_lib_info_t info{};
   if(usb_host_lib_info(&info)==ESP_OK && !serialQuiet)
    Serial.printf("USB: devices=%d ready=%d packets=%lu buttons=%08lx LX=%.2f LY=%.2f\n",info.num_devices,ready,(unsigned long)reportCount,(unsigned long)working.buttons,working.axes[0],working.axes[1]);
  }
  vTaskDelay(std::max<TickType_t>(1,pdMS_TO_TICKS(2)));
 }
}
}
bool startGamepadHost() {
 usb_host_config_t config{};config.intr_flags=ESP_INTR_FLAG_LEVEL1;
 esp_err_t result=usb_host_install(&config);
 if(result!=ESP_OK) {Serial.printf("PAD: host unavailable %s\n",esp_err_to_name(result));return false;}
 usb_host_client_config_t options{};options.max_num_event_msg=5;options.async.client_event_callback=event;
 if(usb_host_client_register(&options,&client)!=ESP_OK) {usb_host_uninstall();return false;}
 if(xTaskCreatePinnedToCore(hostTask,"abyss-usb",6144,nullptr,2,nullptr,0)!=pdPASS) {
  usb_host_client_deregister(client);client=nullptr;usb_host_device_free_all();usb_host_uninstall();return false;
 }
 Serial.printf("PAD: USB host on the Tab5 USB-A socket | hubs=%d multi-level=%d\n",CONFIG_USB_HOST_HUBS_SUPPORTED,CONFIG_USB_HOST_HUB_MULTI_LEVEL);return true;
}
int usbDeviceCount() {return attached;}
PadState gamepadSnapshot() {
 // F310 may send only state changes: retain held controls until release or USB disconnect/error.
 portENTER_CRITICAL(&sampleLock);PadState state=samples.take(retainF310State || uint32_t(millis()-lastReport)<=1500);portEXIT_CRITICAL(&sampleLock);
 // A keyboard reports only on change, so its state is held until it says otherwise.
 // Whichever of the two is moving wins each axis; buttons are the union of both.
 portENTER_CRITICAL(&kbLock);PadState keys=kbSamples.take(kbReady && !kbGone);portEXIT_CRITICAL(&kbLock);
 if(!keys.connected) return state;
 if(!state.connected) return keys;
 state.buttons|=keys.buttons;state.pressed|=keys.pressed;state.axisMask|=keys.axisMask;
 for(int i=0;i<6;++i) {
  float a=state.axes[i]<0?-state.axes[i]:state.axes[i],b=keys.axes[i]<0?-keys.axes[i]:keys.axes[i];
  if(b>a) state.axes[i]=keys.axes[i];
 }
 return state;
}
}
#else
namespace abyss {
bool startGamepadHost() {return false;}
PadState gamepadSnapshot() {return {};}
int usbDeviceCount() {return 0;}
}
#endif
