#line 1 "C:\\Users\\wisht\\New folder\\Setsuna\\esp32_marauder\\C5CompatDisplay.h"
#pragma once
#ifndef C5CompatDisplay_h
#define C5CompatDisplay_h

// TFT_eSPI-compatible shim for the Waveshare ESP32-C5-Touch-LCD-2.8.
//
// TFT_eSPI has no real ESP32-C5 SPI driver (only classic ESP32/C3/S3 paths
// exist) and every variant of its transfer path produces corrupted/glitchy
// pixel output on this chip. Adafruit_ST7789 (via the portable Arduino
// SPIClass) renders perfectly cleanly on the exact same hardware, confirmed
// via a standalone isolation test. This header implements just enough of
// TFT_eSPI's public API (as actually used by this codebase) on top of
// Adafruit_ST7789/Adafruit_GFX so Display.h can swap it in for this board
// only, without touching the hundreds of tft.* call sites elsewhere.

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Pulls in TFT_CS/TFT_DC/TFT_RST/TFT_MOSI/TFT_SCLK/SPI_FREQUENCY etc. for
// this board - normally supplied via TFT_eSPI.h's User_Setup_Select.h chain,
// which we're bypassing entirely on this board.
#include "User_Setup_marauder_c5_touch_lcd_28.h"

// MENU_FONT (configs.h) references this GFXfont; TFT_eSPI normally supplies
// it via its own bundled copy since it isn't in the Adafruit_GFX Fonts dir.
#include "../TFT_eSPI/Fonts/GFXFF/FreeMono9pt7b.h"

// ---- Text datum constants (TFT_eSPI naming) ----
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define CL_DATUM 3
#define MC_DATUM 4
#define CC_DATUM 4
#define MR_DATUM 5
#define CR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8
#define L_BASELINE  9
#define C_BASELINE 10
#define R_BASELINE 11

// ---- Colour constants (TFT_eSPI RGB565 values) ----
#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFE19
#define TFT_BROWN       0x9A60
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#define TFT_SKYBLUE     0x867D
#define TFT_VIOLET      0x915C
#define TFT_TRANSPARENT 0x0120

// GFXFF is TFT_eSPI's "use a free/GFX font" font-slot id; 255 means
// "GLCD/no free font active" in TFT_eSPI's `textfont` convention.
#define GFXFF 1

// Single canonical SPI instance shared across the whole program. This MUST
// NOT be a plain file-scope `static` - since this header (and its inline
// methods) get compiled separately into every .cpp that includes it, a
// file-scope static gives each translation unit its OWN SPIClass object.
// The linker then arbitrarily coalesces the inline constructor/init()
// bodies, so the constructor can end up binding Adafruit_SPITFT's internal
// _spi pointer to one TU's copy while init()'s `.begin()` call runs against
// a DIFFERENT TU's copy - meaning the real display SPI bus never actually
// gets initialized (no crash, just a permanently black screen). A
// function-local static in an inline function is guaranteed by the
// standard to be the same single object across all translation units.
inline SPIClass& c5CompatSPI() {
  static SPIClass instance(FSPI);
  return instance;
}

class C5CompatTFT : public Adafruit_ST7789 {
  public:
    C5CompatTFT() : Adafruit_ST7789(&c5CompatSPI(), TFT_CS, TFT_DC, TFT_RST) {}

    // TFT_eSPI's init() takes no args on this board (fixed panel size)
    void init() {
      // ss MUST be -1 here: Adafruit_SPITFT already toggles TFT_CS itself
      // (software CS, held low across multi-byte command+data writes).
      // Passing TFT_CS as the SPI peripheral's hardware-SS pin makes the
      // ESP32 SPI driver ALSO auto-toggle it once per transfer, so the two
      // controllers fight over the same pin - commands get split/corrupted
      // mid-stream, which shows up as a backlit-but-blank/noisy panel.
      c5CompatSPI().begin(TFT_SCLK, -1, TFT_MOSI, -1);
      Adafruit_ST7789::init(TFT_WIDTH, TFT_HEIGHT);
      setSPISpeed(SPI_FREQUENCY);
    }

    // -------- Text datum / padding (TFT_eSPI-specific concepts) --------
    uint8_t getTextDatum() { return _textdatum; }
    void setTextDatum(uint8_t datum) { _textdatum = datum; }
    uint16_t getTextPadding() { return _textpadding; }
    void setTextPadding(uint16_t padding) { _textpadding = padding; }

    // textfont: 255 = GLCD built-in font in use, GFXFF = a setFreeFont() font in use.
    // Marauder code reads this directly (tft.textfont == 255) to branch behaviour.
    uint8_t textfont = 255;

    void setFreeFont(const GFXfont *f) {
      setFont(f);
      textfont = (f == nullptr) ? 255 : GFXFF;
    }

    void setTextFont(uint8_t font) {
      if (font == 1) { setFont(nullptr); textfont = 255; }
    }

    // -------- drawString family (datum-aware, TFT_eSPI-specific) --------
    int16_t drawString(const char *string, int32_t x, int32_t y, uint8_t font = 1) {
      int16_t x1, y1;
      uint16_t w, h;
      getTextBounds((char *)string, 0, 0, &x1, &y1, &w, &h);

      // Note: ML_DATUM/CL_DATUM, MC_DATUM/CC_DATUM, MR_DATUM/CR_DATUM are
      // synonyms (same numeric value) so only one label per value is used.
      int32_t dx = 0, dy = 0;
      switch (_textdatum) {
        case TC_DATUM: dx = -(int32_t)w / 2; break;
        case TR_DATUM: dx = -(int32_t)w; break;
        case ML_DATUM: dy = -(int32_t)h / 2; break;
        case MC_DATUM: dx = -(int32_t)w / 2; dy = -(int32_t)h / 2; break;
        case MR_DATUM: dx = -(int32_t)w; dy = -(int32_t)h / 2; break;
        case BL_DATUM: dy = -(int32_t)h; break;
        case BC_DATUM: dx = -(int32_t)w / 2; dy = -(int32_t)h; break;
        case BR_DATUM: dx = -(int32_t)w; dy = -(int32_t)h; break;
        default: break; // TL_DATUM / baseline variants: no offset
      }

      int32_t drawX = x + dx - x1;
      int32_t drawY = y + dy - y1;

      if (_textpadding > 0) {
        fillRect(x + dx, y + dy, max((int32_t)_textpadding, (int32_t)w), h, _textbgcolor);
      }

      setCursor(drawX, drawY);
      print(string);
      return w;
    }
    int16_t drawString(const String &string, int32_t x, int32_t y, uint8_t font = 1) {
      return drawString(string.c_str(), x, y, font);
    }
    int16_t drawCentreString(const char *string, int32_t x, int32_t y, uint8_t font = 0) {
      uint8_t prevDatum = _textdatum;
      setTextDatum(TC_DATUM);
      int16_t w = drawString(string, x, y);
      setTextDatum(prevDatum);
      return w;
    }
    int16_t drawCentreString(const String &string, int32_t x, int32_t y, uint8_t font = 0) {
      return drawCentreString(string.c_str(), x, y, font);
    }
    int16_t drawRightString(const char *string, int32_t x, int32_t y, uint8_t font = 0) {
      uint8_t prevDatum = _textdatum;
      setTextDatum(TR_DATUM);
      int16_t w = drawString(string, x, y);
      setTextDatum(prevDatum);
      return w;
    }
    int16_t drawRightString(const String &string, int32_t x, int32_t y, uint8_t font = 0) {
      return drawRightString(string.c_str(), x, y, font);
    }

    int16_t textWidth(const char *string, uint8_t font = 0) {
      int16_t x1, y1; uint16_t w, h;
      getTextBounds((char *)string, 0, 0, &x1, &y1, &w, &h);
      return w;
    }
    int16_t textWidth(const String &string, uint8_t font = 0) { return textWidth(string.c_str(), font); }
    int16_t fontHeight(uint8_t font = 0) {
      int16_t x1, y1; uint16_t w, h;
      getTextBounds((char *)"Mg", 0, 0, &x1, &y1, &w, &h);
      return h;
    }

    void setTextColor(uint16_t fg) { _textbgcolor = fg; Adafruit_GFX::setTextColor(fg); }
    void setTextColor(uint16_t fg, uint16_t bg) { _textbgcolor = bg; Adafruit_GFX::setTextColor(fg, bg); }
    void setTextColor(uint16_t fg, uint16_t bg, bool) { setTextColor(fg, bg); }

    void setTextWrap(bool w) { Adafruit_GFX::setTextWrap(w); }
    void setTextWrap(bool wx, bool wy) { Adafruit_GFX::setTextWrap(wx); (void)wy; }

    // -------- drawXBitmap fg/bg overload (TFT_eSPI-specific) --------
    // XBM bit order is LSB-first per byte; draw both fg and bg pixels
    // (Adafruit_GFX's built-in drawXBitmap only draws fg, leaving bg alone).
    void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor) {
      int16_t byteWidth = (w + 7) / 8;
      startWrite();
      for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
          bool bit = (pgm_read_byte(bitmap + j * byteWidth + i / 8) >> (i & 7)) & 1;
          writePixel(x + i, y + j, bit ? fgcolor : bgcolor);
        }
      }
      endWrite();
    }

    // TFT_eSPI-style image push - not used with capacitive touch on this
    // board (touch is via cst3530.h, not tft.getTouch), stub only.
    uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600) { return 0; }

  private:
    uint8_t  _textdatum = TL_DATUM;
    uint16_t _textpadding = 0;
    uint16_t _textbgcolor = TFT_BLACK;
};

/***************************************************************************************
** C5CompatTFTButton - ported from TFT_eSPI's Extensions/Button.(h/cpp), calling only
** the compatible surface implemented by the shim TFT class above.
***************************************************************************************/
class C5CompatTFTButton {
  public:
    C5CompatTFTButton(void) {
      _gfx = nullptr;
      _xd = 0; _yd = 0;
      _textdatum = MC_DATUM;
      _label[9] = '\0';
      currstate = false;
      laststate = false;
    }

    void initButton(C5CompatTFT *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                     uint16_t outline, uint16_t fill, uint16_t textcolor,
                     char *label, uint8_t textsize) {
      initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor, label, textsize);
    }

    void initButtonUL(C5CompatTFT *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                       uint16_t outline, uint16_t fill, uint16_t textcolor,
                       char *label, uint8_t textsize) {
      _x1 = x1; _y1 = y1; _w = w; _h = h;
      _outlinecolor = outline; _fillcolor = fill; _textcolor = textcolor;
      _textsize = textsize; _gfx = gfx;
      strncpy(_label, label, 9);
    }

    void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM) {
      _xd = x_delta; _yd = y_delta; _textdatum = datum;
    }

    void drawButton(bool inverted = false, String long_name = "") {
      uint16_t fill, outline, text;
      if (!inverted) { fill = _fillcolor; outline = _outlinecolor; text = _textcolor; }
      else { fill = _textcolor; outline = _outlinecolor; text = _fillcolor; }

      uint8_t r = min(_w, _h) / 4;
      _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
      _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

      if (_gfx->textfont == 255) {
        _gfx->setCursor(_x1 + (_w / 8), _y1 + (_h / 4));
        _gfx->setTextColor(text);
        _gfx->setTextSize(_textsize);
        _gfx->print(_label);
      } else {
        _gfx->setTextColor(text, fill);
        _gfx->setTextSize(_textsize);

        uint8_t tempdatum = _gfx->getTextDatum();
        _gfx->setTextDatum(_textdatum);
        uint16_t tempPadding = _gfx->getTextPadding();
        _gfx->setTextPadding(0);

        if (long_name == "")
          _gfx->drawString(_label, _x1 + (_w / 2) + _xd, _y1 + (_h / 2) - 4 + _yd);
        else
          _gfx->drawString(long_name, _x1 + (_w / 2) + _xd, _y1 + (_h / 2) - 4 + _yd);

        _gfx->setTextDatum(tempdatum);
        _gfx->setTextPadding(tempPadding);
      }
    }

    bool contains(int16_t x, int16_t y) {
      if (_use_hit_box)
        return ((x >= _hit_x1) && (x < (_hit_x1 + _hit_w)) && (y >= _hit_y1) && (y < (_hit_y1 + _hit_h)));
      return ((x >= _x1) && (x < (_x1 + _w)) && (y >= _y1) && (y < (_y1 + _h)));
    }

    // Lets the tappable area be bigger/positioned differently than the
    // visible drawn button - e.g. a small label bar with a full-height
    // tappable column behind it, to tolerate imprecise/imperfectly-calibrated
    // touch coordinates.
    void setHitBox(int16_t x1, int16_t y1, uint16_t w, uint16_t h) {
      _hit_x1 = x1; _hit_y1 = y1; _hit_w = w; _hit_h = h; _use_hit_box = true;
    }

    void press(bool p) { laststate = currstate; currstate = p; }
    bool isPressed() { return currstate; }
    bool justPressed() { return (currstate && !laststate); }
    bool justReleased() { return (!currstate && laststate); }

  private:
    C5CompatTFT *_gfx;
    int16_t  _x1, _y1;
    int16_t  _xd, _yd;
    uint16_t _w, _h;
    uint8_t  _textsize, _textdatum;
    uint16_t _outlinecolor, _fillcolor, _textcolor;
    char     _label[10];
    bool     currstate, laststate;
    int16_t  _hit_x1 = 0, _hit_y1 = 0;
    uint16_t _hit_w = 0, _hit_h = 0;
    bool     _use_hit_box = false;
};

// Redirect all TFT_eSPI/TFT_eSPI_Button usage in this translation unit to the
// shim classes above. Scoped to files that include this header (Display.h and
// everything downstream), so it never affects any other translation unit -
// including ones where Arduino's library scanner still compiles the real
// TFT_eSPI library due to its textually-included (but #ifdef-guarded-out)
// reference in TDongleDisplay.h. Without this, both classes would share the
// name "TFT_eSPI" across different translation units, and the linker could
// mix their symbols (real ODR violation - this caused a boot crash).
#define TFT_eSPI C5CompatTFT
#define TFT_eSPI_Button C5CompatTFTButton

#endif // C5CompatDisplay_h
