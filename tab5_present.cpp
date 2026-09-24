// Getting a finished frame onto the Tab5's screen.
//
// The ST7789 build converted and pushed the frame band by band over SPI, and the byte
// swap and the wait for DMA were a real slice of every frame. Here none of that is the
// CPU's work. The P4 has a Pixel Processing Accelerator that scales, rotates and mirrors
// a picture in one transaction, so the whole journey - 640x480 enlarged to 960x720,
// turned a quarter turn for the panel's portrait frame buffer, and centred - is a single
// call, and the cores are free while it runs.
//
// The DSI panel is scanned out continuously from one buffer, so a frame that lands while
// the panel is mid-screen will tear. That is the same bargain the SPI build made.
#include "config.h"
#include "lgfx_setup.h"
#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>
#include <driver/ppa.h>
#include <esp_heap_caps.h>
#include <Arduino.h>
#include <Wire.h>

namespace abyss {
namespace {
ppa_client_handle_t ppa = nullptr;
void* frameBuffer = nullptr;
size_t frameBufferBytes = 0;
// Which way up the landscape picture goes on the portrait panel. Which of the two
// quarter turns is the right one is a property of how the panel is mounted, so it is
// kept adjustable and reported at startup rather than guessed at in a comment.
//
// 270 is what M5GFX's own landscape rotation works out to: at rotation 1 it sends a
// logical (x,y) to frame buffer column 719-y and row x, which is a quarter turn
// clockwise, and the PPA counts its angles the other way round.
ppa_srm_rotation_angle_t rotation = PPA_SRM_ROTATION_ANGLE_270;
}  // namespace

void presentSetRotation(int quarters) {
  switch(((quarters%4)+4)%4) {
    case 0: rotation=PPA_SRM_ROTATION_ANGLE_0; break;
    case 1: rotation=PPA_SRM_ROTATION_ANGLE_90; break;
    case 2: rotation=PPA_SRM_ROTATION_ANGLE_180; break;
    default: rotation=PPA_SRM_ROTATION_ANGLE_270; break;
  }
}
int presentRotation() {
  switch(rotation) {
    case PPA_SRM_ROTATION_ANGLE_0: return 0;
    case PPA_SRM_ROTATION_ANGLE_90: return 1;
    case PPA_SRM_ROTATION_ANGLE_180: return 2;
    default: return 3;
  }
}

namespace {
// The display module's only reset is its power rail, and that rail is latched by a PI4IO
// expander on the internal I2C bus - a separate part, which a CPU reset does not touch.
// So a board that has just been running with the panel up comes back with the latch still
// set, and the bring-up that follows lands on a module that was never off.
//
// M5GFX does pulse the two reset lines, but only for the 10 ms it waits between its two
// register writes, and that is not long enough for the rail to fall. Left alone this
// board comes up in alternate boots: a run that brought the panel up is followed by one
// that cannot find it. Holding the rail down for 150 ms first is what settles it.
constexpr uint8_t Pi4ioAddr=0x43;         // the expander that carries the display rails
constexpr int PinSDA=31, PinSCL=32;       // the internal I2C bus
void writeRegister(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(Pi4ioAddr); Wire.write(reg); Wire.write(value); Wire.endTransmission();
}
void panelPowerDown() {
  Wire.begin(PinSDA,PinSCL,400000);
  writeRegister(0x03,0b01111111);         // IO_DIR: the rails are outputs
  writeRegister(0x0D,0b01111111);         // PULL_SEL
  writeRegister(0x0B,0b01111111);         // PULL_EN
  writeRegister(0x07,0b00000000);         // OUT_H_IM: drive, do not float
  writeRegister(0x05,0b01000110);         // OUT_SET with bit4 (panel) and bit5 (touch) low
  delay(150);
  Wire.end();
}
}  // namespace

bool displayBegin() {
  panelPowerDown();
  auto cfg=M5.config();
  // The USB-A socket is the way in for a pad or a keyboard, and it stays dark unless the
  // board is asked to power it.
  cfg.output_power=true;
  M5.begin(cfg);
  if(M5.Display.width()==0) {
    // The panel was not found. It costs a second to hold the rail down for longer and
    // ask again, and that is cheaper than a board that boots blind every other time.
    Serial.println("LCD: no panel on the first try; cycling the rail again");
    panelPowerDown();
    delay(350);
    M5.Display.init();
  }
  return M5.Display.width()>0;
}

bool presentBegin() {
  auto* raw=M5.Display.panel();
  Serial.printf("PPA: board=%d display=%dx%d panel=%p\n",
                int(M5.getBoard()),M5.Display.width(),M5.Display.height(),(void*)raw);
  auto* panel=static_cast<lgfx::Panel_DSI*>(raw);
  if(!panel) { Serial.println("PPA: no panel"); return false; }
  Serial.printf("PPA: dsi bus=%p\n",(void*)panel->getBusDSI());
  const auto& detail=panel->config_detail();
  frameBuffer=detail.buffer;
  if(!frameBuffer) { Serial.println("PPA: the DSI frame buffer was never handed out"); return false; }
  // Panel_DSI lays the buffer out as one run of panel_height lines of panel_width
  // pixels, so its size follows from the panel and not from buffer_length, which the
  // panel leaves at zero.
  frameBufferBytes=size_t(PanelNativeW)*PanelNativeH*sizeof(uint16_t);
  ppa_client_config_t config={};
  config.oper_type=PPA_OPERATION_SRM;
  config.max_pending_trans_num=1;
  if(ppa_register_client(&config,&ppa)!=ESP_OK) { Serial.println("PPA: client registration failed"); return false; }
  Serial.printf("PPA: frame buffer %p, %u KB, %dx%d -> %dx%d at x=%d rotation=%d\n",
                frameBuffer,unsigned(frameBufferBytes/1024),Width,Height,ViewW,ViewH,ViewX,presentRotation());
  return true;
}

void presentFrame(const uint16_t* frame) {
  if(!ppa || !frameBuffer || !frame) return;
  ppa_srm_oper_config_t op={};
  op.in.buffer=const_cast<uint16_t*>(frame);
  op.in.pic_w=Width;
  op.in.pic_h=Height;
  op.in.block_w=Width;
  op.in.block_h=Height;
  op.in.block_offset_x=0;
  op.in.block_offset_y=0;
  op.in.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  op.out.buffer=frameBuffer;
  op.out.buffer_size=frameBufferBytes;
  op.out.pic_w=PanelNativeW;
  op.out.pic_h=PanelNativeH;
  op.out.srm_cm=PPA_SRM_COLOR_MODE_RGB565;
  // A quarter turn swaps the picture's width and height, so the landscape view fills the
  // portrait buffer exactly: 720 across by 1280 down, corner to corner.
  const bool turned=(rotation==PPA_SRM_ROTATION_ANGLE_90 || rotation==PPA_SRM_ROTATION_ANGLE_270);
  const int placedW=turned?ViewH:ViewW, placedH=turned?ViewW:ViewH;
  op.out.block_offset_x=(PanelNativeW-placedW)/2;
  op.out.block_offset_y=(PanelNativeH-placedH)/2;
  op.rotation_angle=rotation;
  // Two across and one and a half down. That is not the same number on both axes, and it
  // is not meant to be: the camera is widened by exactly this much, so the stretch undoes
  // itself and the world arrives square. See the note in config.h.
  op.scale_x=float(ViewW)/Width;                  // 2.0
  op.scale_y=float(ViewH)/Height;                 // 1.5
  // The panel's frame buffer holds RGB565 in native byte order, the same as the
  // renderer's, so nothing is swapped on the way through.
  op.byte_swap=false;
  op.rgb_swap=false;
  op.alpha_update_mode=PPA_ALPHA_NO_CHANGE;
  op.mode=PPA_TRANS_MODE_BLOCKING;
  esp_err_t err=ppa_do_scale_rotate_mirror(ppa,&op);
  if(err!=ESP_OK) {
    static uint32_t complained=0;
    if(millis()-complained>2000) { complained=millis(); Serial.printf("PPA: transfer failed (%s)\n",esp_err_to_name(err)); }
  }
}
}  // namespace abyss
