#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "lcd_wrapper.h"
#include "rtc_wrapper.h"
#include "sensors.h"
#include "screens.h"

extern volatile byte btn_state;

void led_init() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, LOW);
}

void set_led(int r, int g, int b) {
    digitalWrite(LED_R, r);
    digitalWrite(LED_G, g);
    digitalWrite(LED_B, b);
}

void screens_init() {
    Serial.begin(BAUD_RATE);

    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    pinMode(BTN3, INPUT_PULLUP);
    pinMode(BTN4, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);

    led_init();
    lcd_init();
    clock_init();
    sensors_init();

    set_led(0, 1, 0);

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("  Alarm clock");
    lcd_set_cursor(1, 0);
    lcd_print("  Starting...");
    delay(2000);
    lcd_clear();
}