// Minimal isolation test for Waveshare ESP32-C5-Touch-LCD-2.8
// Uses Adafruit_ST7789 (matching the confirmed-working reference project)
// instead of TFT_eSPI, to determine if the glitching is a driver/init
// mismatch or a genuine hardware/wiring fault.

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>

#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  -1
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define IO_EXPANDER_ADDR   0x24
#define IO_EXPANDER_MODE_REG   0x02
#define IO_EXPANDER_OUTPUT_REG 0x03
#define IO_EXPANDER_PWM_REG    0x05
#define IO_EXPANDER_PIN_TP_RST  0
#define IO_EXPANDER_PIN_LCD_RST 1
#define IO_EXPANDER_PIN_PA_CTRL 3

SPIClass tftSPI(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

static uint8_t expanderOutput = 0;

static void expanderWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IO_EXPANDER_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

static void expanderSet(uint8_t pin, bool level) {
  if (level) expanderOutput |= (1 << pin);
  else expanderOutput &= ~(1 << pin);
  expanderWrite(IO_EXPANDER_OUTPUT_REG, expanderOutput);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== C5 display isolation test (Adafruit_ST7789) ===");

  Wire.begin(0, 1, 400000U);

  expanderWrite(IO_EXPANDER_MODE_REG, 0xFF);
  expanderOutput = (1 << IO_EXPANDER_PIN_TP_RST) | (1 << IO_EXPANDER_PIN_LCD_RST) | (1 << IO_EXPANDER_PIN_PA_CTRL);
  expanderWrite(IO_EXPANDER_OUTPUT_REG, expanderOutput);

  Serial.println("Pulsing LCD reset via IO expander...");
  expanderSet(IO_EXPANDER_PIN_LCD_RST, false);
  delay(50);
  expanderSet(IO_EXPANDER_PIN_LCD_RST, true);
  delay(120);

  expanderWrite(IO_EXPANDER_PWM_REG, 255);
  Serial.println("Backlight on.");

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(20000000);
  tft.setRotation(2);

  Serial.println("tft.init() done. Drawing test pattern...");
}

void loop() {
  tft.fillScreen(ST77XX_RED);
  Serial.println("RED");
  delay(2000);

  tft.fillScreen(ST77XX_GREEN);
  Serial.println("GREEN");
  delay(2000);

  tft.fillScreen(ST77XX_BLUE);
  Serial.println("BLUE");
  delay(2000);

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.println("HELLO");
  tft.println("C5");
  Serial.println("TEXT");
  delay(3000);
}
