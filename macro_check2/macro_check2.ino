#include <TFT_eSPI.h>
void setup() {
#if defined(_TFT_eSPI_ESP32H_)
  #error MACRO_CHECK: C3_DRIVER_HEADER_ACTIVE
#else
  #error MACRO_CHECK: C3_DRIVER_HEADER_NOT_ACTIVE
#endif
}
void loop() {}
