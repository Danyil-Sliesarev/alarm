#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "lcd_wrapper.h"
#include "rtc_wrapper.h"
#include "sensors.h"
#include "screens.h"
#include "ir_wrapper.h"

volatile byte btn_state;

extern void screens_init();
extern void set_led(int r, int g, int b);

int main() {
    init();
    Wire.begin();

    screens_init();
    ir_init();

    enum screen current = CLOCK_SCR;

    for(;;) {
        btn_state = 0;
        if (digitalRead(BTN1) == LOW) btn_state |= BTN1_PRESSED;
        if (digitalRead(BTN2) == LOW) btn_state |= BTN2_PRESSED;
        if (digitalRead(BTN3) == LOW) btn_state |= BTN3_PRESSED;
        if (digitalRead(BTN4) == LOW) btn_state |= BTN4_PRESSED;

        switch(current) {
            case CLOCK_SCR:
                current = clock_screen();
                break;
            case ALARM_SCR:
                current = alarm_screen();
                break;
            case SHOW_DATE_SCR:
                current = show_date_screen();
                break;
            case SHOW_ENV_SCR:
                current = show_env_screen();
                break;
            case FACTORY_RESET_SCR:
                current = factory_reset_screen();
                break;
            default:
                current = CLOCK_SCR;
                break;
        }
    }
}