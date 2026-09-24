#pragma once
#ifndef ABYSS_ENABLE_USB_GAMEPAD
#define ABYSS_ENABLE_USB_GAMEPAD 1
#endif
// Render pixels per panel pixel. The world is drawn at UpScale times the panel and box
// filtered on the way out, so a higher resolution panel only changes PanelW/PanelH.
//
// This is the Tab5 build. The world is drawn at 640x480 - UpScale 2 over the 320x240
// interface - and the P4's pixel accelerator scales that by 1.5 and turns it a
// quarter turn into the 720x1280 DSI frame buffer, landing as 960x720 centred on the
// 1280x720 screen. The interface stays 320x240 so the lettering and the title card
// keep the size they were drawn at; going to 640x480 here would halve them. All
// the art - title card, zone cards, HUD font, monument and species thumbnails - is
// authored in panel coordinates and drawn through hudBlit/hudText, which multiply by
// UpScale; the baked thumbnails in monument_shots.h and species_shots.h were rendered
// from a 640x480 frame and so have the detail for it already.
//
// Set to 1 to draw the world at 320x240 instead: a quarter of the pixels to fill, and
// the accelerator enlarges by 3 rather than 1.5 to reach the same 960x720.
#ifndef ABYSS_UPSCALE
#define ABYSS_UPSCALE 2
#endif
static_assert(ABYSS_UPSCALE == 1 || ABYSS_UPSCALE == 2,
              "ABYSS_UPSCALE must be 1 or 2");
namespace abyss {
constexpr int PanelW = 320, PanelH = 240;           // the interface every layout is written in
constexpr int UpScale = ABYSS_UPSCALE;
constexpr int Width = PanelW * UpScale;             // the frame the renderer works in
constexpr int Height = PanelH * UpScale;
constexpr int TileRows = 12;                        // band height, in render rows
constexpr int TileCount = Height / TileRows;
constexpr float Near = 0.4f, Far = 90.0f;
// How far it is possible to see inside the undersea city. The five seas each carry their
// own figure in world_data.h; the city is not one of them and keeps its own here, because
// it is the one place that is lit rather than merely less dark, and a lit city that stops
// at a wall of water a few streets away does not read as a city at all. It is also what
// the culling uses, so raising it is what decides how much of the place gets drawn.
//
// 220 metres is the whole of it. What the city asks for stops climbing at about 180 -
// past that there is simply no more city - so this is as far as the figure is worth
// taking, and the cost of taking it is in the frame rate rather than in memory.
#ifndef ABYSS_CITY_VISIBILITY
#define ABYSS_CITY_VISIBILITY 220
#endif
constexpr float CityVisibility = ABYSS_CITY_VISIBILITY;
// The panel is 1280x720 and the frame is 640x480, so on its way out the frame is
// stretched by two across and one and a half down. Rather than let that squash the
// world, the camera is widened to match: the vertical field is exactly what it always
// was, and the horizontal one is opened out to 16:9, so the stretch puts everything back
// square again and the pilot simply sees more sea to either side. No more pixels are
// drawn for it, so a frame costs what it always did.
constexpr int ScreenW = 1280, ScreenH = 720;
constexpr float Focal = Height * 16.0f / 15.0f;          // 512, as Width*0.80f gave at 4:3
constexpr float FocalX = Focal * (float(Width) / Height) * (float(ScreenH) / ScreenW);
// A frame pixel is four across to three down once it reaches the panel. Anything that has
// to come out round - a bubble, the survey map - is drawn this much narrower than tall.
constexpr float PixelAspect = (float(ScreenH) * Width) / (float(Height) * ScreenW);
// Interface columns per character. The face is baked to match, so this is the one number
// that has to move if the lettering is ever rebuilt at a different width.
constexpr int HudAdvance = 5;
// A panel that takes over the screen is laid out as the 4:3 picture it was designed for
// and centred, rather than spread across the whole width: a list of names does not get
// better for being a third wider, and the touch controls need somewhere to be that is not
// on top of it. 240 columns by 240 rows reaches the panel as 960x720.
constexpr int BoxW = (ScreenH * 4 / 3) * PanelW / ScreenW;
constexpr int BoxX = (PanelW - BoxW) / 2;
// How many triangles the world can be made of, and how many can be waiting to be drawn
// at once. Both were close enough to their old ceilings to fall over them.
//
// Walking the city on a ten metre grid, looking eight ways from every point of it, the
// most the projection ever held was 21,218. The old budget was 22,000, and a budget met
// to within four per cent is one that a creature swimming into the wrong street will
// exceed - which is what it did. The scene buffer was no better placed: the open sea
// peaks at about 72,600 triangles against an old 80,000, and the rebuilt viperfish had
// just pushed that figure up.
//
// Both arrays are in PSRAM, which has twenty megabytes spare, so the headroom costs two
// and a half of them and nothing else.
#ifndef ABYSS_MAX_PROJECTED
#define ABYSS_MAX_PROJECTED 28000
#endif
#ifndef ABYSS_MAX_TRIANGLES
#define ABYSS_MAX_TRIANGLES 96000
#endif
constexpr int MaxTriangles = ABYSS_MAX_TRIANGLES, MaxProjected = ABYSS_MAX_PROJECTED;
constexpr int WorldSize = 640, WorldMin = -320;
constexpr int ChunkSize = 32, ChunksAcross = WorldSize / ChunkSize;
constexpr int ChunkCount = ChunksAcross * ChunksAcross;
constexpr float TourSeconds = 866.0f;
}
