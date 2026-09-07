//                            USER DEFINED SETTINGS
//   TFT_eSPI setup for Waveshare ESP32-C5-Touch-LCD-2.8 (ESP32-C5-WROOM-1, ST7789, CST3530 touch)

// ##################################################################################
// Section 1. Driver
// ##################################################################################

#define ST7789_DRIVER

// TFT_eSPI defaults ST7789_DRIVER boards to SPI_MODE3, but this exact panel
// only renders cleanly in SPI_MODE0 (confirmed via a standalone isolation
// test using Adafruit_ST7789, which defaults to MODE0). MODE3 here is the
// root cause of the persistent glitching/corrupted UI drawing.
#define TFT_SPI_MODE SPI_MODE0

#define TFT_RGB_ORDER TFT_RGB

// Native panel resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// If colours are inverted (white shows as black) then uncomment one of the next
// 2 lines, try both options, one of the options should correct the inversion.
#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// ##################################################################################
// Section 2. Pin assignments (Waveshare ESP32-C5-Touch-LCD-2.8 hardware)
// ##################################################################################

#define TFT_MISO -1  // Not connected, display is write-only
#define TFT_MOSI  7
#define TFT_SCLK  6
#define TFT_CS   10  // TFT chip select
#define TFT_DC    9  // Data/command
#define TFT_RST  -1  // No direct reset GPIO - LCD reset is via the onboard CH32V003 IO expander
#define TFT_BL   -1  // No direct backlight GPIO - backlight PWM is via the onboard CH32V003 IO expander

#define TOUCH_CS -1  // CST3530 capacitive touch is I2C-only

// ##################################################################################
// Section 3. Fonts
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
// #define LOAD_FONT6
// #define LOAD_FONT7
// #define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ##################################################################################
// Section 4. SPI speed
// ##################################################################################

// The Waveshare ESP32-C5 LCD is stable at the lower 27 MHz SPI rate used by
// the known-good board reference. Running the ST7789 at 40 MHz can corrupt the
// display data and show as full-screen pixel noise/glitching. Dropped further
// to 10 MHz since Marauder's rapid sequential UI draw calls (unlike a simple
// solid-fill test) still showed glitching/tearing at 20 MHz. Trying a modest
// bump to 16 MHz (well below the observed-bad 20 MHz) to reduce redraw time -
// revert to 10000000 if any tearing/corruption reappears.
// Marauder's rapid sequential UI draw calls are stable at the known-good
// 10 MHz rate on this panel.
#define SPI_FREQUENCY       10000000
#define SPI_READ_FREQUENCY  10000000
#define SPI_TOUCH_FREQUENCY  1000000

