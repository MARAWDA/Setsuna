// Touch calibration test for Waveshare ESP32-C5-Touch-LCD-2.8.
// Draws a crosshair target at each screen corner + center; on every touch,
// prints the RAW touch-controller coordinates over serial and draws a dot at
// wherever the code currently thinks you touched (using the same case-2
// rotation formula as the real firmware). Compare dot position to your
// actual finger position to figure out the correct mapping.

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>

#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS   10
#define TFT_DC    9
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

#define CST3530_ADDR             0x58
#define CST3530_DATA_REG         0xD0070000UL
#define CST3530_END_READ_REG     0xD00002ABUL

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

static bool cst3530Read(uint32_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(CST3530_ADDR);
  Wire.write((uint8_t)((reg >> 24) & 0xFF));
  Wire.write((uint8_t)((reg >> 16) & 0xFF));
  Wire.write((uint8_t)((reg >> 8) & 0xFF));
  Wire.write((uint8_t)(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom((int)CST3530_ADDR, (int)len);
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.available() ? Wire.read() : 0;
  return true;
}

static bool readTouch(uint16_t *raw_x, uint16_t *raw_y) {
  uint8_t buf[9];
  if (!cst3530Read(CST3530_DATA_REG, buf, 9)) return false;
  if ((buf[3] & 0x0F) == 0x00) {
    cst3530Read(CST3530_END_READ_REG, NULL, 0);
    return false;
  }
  *raw_x = (uint16_t)(((buf[7] & 0x0F) << 8) | buf[4]);
  *raw_y = (uint16_t)(((buf[7] & 0xF0) << 4) | buf[5]);
  cst3530Read(CST3530_END_READ_REG, NULL, 0);
  return true;
}

void drawTargets() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  struct { int x, y; const char* label; } corners[] = {
    {10, 10, "TL"},
    {TFT_WIDTH - 10, 10, "TR"},
    {10, TFT_HEIGHT - 10, "BL"},
    {TFT_WIDTH - 10, TFT_HEIGHT - 10, "BR"},
    {TFT_WIDTH / 2, TFT_HEIGHT / 2, "MID"},
  };
  for (auto &c : corners) {
    tft.drawFastHLine(c.x - 6, c.y, 12, ST77XX_YELLOW);
    tft.drawFastVLine(c.x, c.y - 6, 12, ST77XX_YELLOW);
    tft.setCursor(c.x - 8, c.y + 8);
    tft.print(c.label);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== C5 touch calibration ===");

  Wire.begin(0, 1, 400000U);
  expanderWrite(IO_EXPANDER_MODE_REG, 0xFF);
  expanderOutput = (1 << IO_EXPANDER_PIN_TP_RST) | (1 << IO_EXPANDER_PIN_LCD_RST) | (1 << IO_EXPANDER_PIN_PA_CTRL);
  expanderWrite(IO_EXPANDER_OUTPUT_REG, expanderOutput);

  expanderSet(IO_EXPANDER_PIN_LCD_RST, false);
  delay(50);
  expanderSet(IO_EXPANDER_PIN_LCD_RST, true);
  delay(120);
  expanderSet(IO_EXPANDER_PIN_TP_RST, false);
  delay(100);
  expanderSet(IO_EXPANDER_PIN_TP_RST, true);
  delay(500);
  expanderWrite(IO_EXPANDER_PWM_REG, 255);

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(10000000);
  tft.setRotation(2);

  drawTargets();
  Serial.println("Tap each yellow crosshair (TL, TR, BL, BR, MID) one at a time.");
  Serial.println("Reporting RAW touch-controller coordinates for each tap:");
}

void loop() {
  uint16_t raw_x, raw_y;
  if (readTouch(&raw_x, &raw_y)) {
    Serial.printf("RAW touch: x=%u y=%u\n", raw_x, raw_y);
    tft.fillCircle(map(raw_x, 0, 240, 0, TFT_WIDTH), map(raw_y, 0, 320, 0, TFT_HEIGHT), 3, ST77XX_RED);
    delay(150);
  }
}
