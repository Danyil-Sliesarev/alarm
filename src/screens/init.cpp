#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "context.h"
#include "screens.h"


/**
 * Initialization screen.
 *
 * This state is responsible for initializing the hardware components of the alarm clock, such as the LCD display, the serial communication, and the I2C communication. After initialization, it transitions to the clock screen.
 *
 * Feel free to extend the initialization screen with additional functionality, such as initializing additional hardware components, if you decide to implement additional features.
 */
enum screen init_screen(struct context *ctx){
  init();

  // i2c init
  Wire.begin();

  // serial init
  Serial.begin(BAUD_RATE);
  while(!Serial);
  Serial.println("> Init Screen");

  return CLOCK_SCR;
}
