#pragma once
// M5Stack Tab5 (ESP32-P4). The panel is a 720x1280 MIPI-DSI module - ST7121, ST7123 or
// ILI9881C depending on the batch - and M5GFX already knows how to tell them apart and
// which lane rate and porches each one needs. None of that is repeated here.
//
// The screen is held in landscape, so the frame the renderer produces has to be turned a
// quarter turn on its way to the panel's portrait frame buffer, and enlarged from 640x480
// to 960x720 to fill the height. Both of those, and the byte swap the panel wants, are
// one transaction on the P4's Pixel Processing Accelerator; see tab5_present.cpp.
#include <M5Unified.h>
#include "config.h"

// The renderer's frame, scaled and turned into the DSI frame buffer. Returns false until
// presentBegin has found the frame buffer.
namespace abyss {
// Brings the panel up: power-cycles the display rail, starts M5Unified, and retries once
// if the panel was not found. Returns false if the screen never came up.
bool displayBegin();
// Call once, after displayBegin(). Locates the DSI frame buffer and claims a PPA client.
bool presentBegin();
// Hand over one finished 640x480 frame. Blocks until the accelerator has placed it.
void presentFrame(const uint16_t* frame);
// Quarter turns clockwise on the way to the panel. Which one is right depends on how the
// module is mounted, so it is settable at run time while the board is on the bench.
void presentSetRotation(int quarters);
int presentRotation();
// Where the picture sits on the screen, in landscape coordinates. It is the whole of it:
// the frame is stretched across the full width and the camera is widened to suit, so no
// margins are left over and the touch controls sit on the picture itself.
constexpr int ViewW = ScreenW, ViewH = ScreenH;
constexpr int ViewX = 0, ViewY = 0;
// The panel's own geometry, before rotation.
constexpr int PanelNativeW = 720, PanelNativeH = 1280;
}
