#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "screens.h"
#include "lcd_wrapper.h"
#include "ir_wrapper.h"

extern void set_led(int r, int g, int b);
extern volatile byte btn_state;

int alarm_hour = 7;
int alarm_minute = 0;
bool alarm_enabled = true;
bool alarm_triggered = false;
bool snooze_active = false;
unsigned long snooze_start = 0;

extern int hours;
extern int minutes;
extern int seconds;
extern DisplayState current_state;
extern unsigned long last_buzzer;
extern bool buzzer_state;
extern bool btn1_pressed, btn2_pressed;
extern bool btn3_pressed, btn4_pressed;

#define SNOOZE_MINUTES 5

void alarm_load() {
    alarm_hour = EEPROM.read(EEPROM_ALARM_HOUR);
    alarm_minute = EEPROM.read(EEPROM_ALARM_MINUTE);
    if (alarm_hour > 23) alarm_hour = 7;
    if (alarm_minute > 59) alarm_minute = 0;
}

void alarm_save() {
    EEPROM.update(EEPROM_ALARM_HOUR, alarm_hour);
    EEPROM.update(EEPROM_ALARM_MINUTE, alarm_minute);
}

void check_alarm() {
    if (snooze_active) {
        unsigned long snooze_elapsed = millis() - snooze_start;
        if (snooze_elapsed >= (unsigned long)SNOOZE_MINUTES * 60 * 1000UL) {
            snooze_active = false;
            alarm_triggered = false;
            current_state = DS_ALARM_RINGING;
            last_buzzer = millis();
            buzzer_state = false;
            lcd_clear();
        }
        return;
    }

    if (!alarm_enabled || alarm_triggered) return;
    if (hours == alarm_hour && minutes == alarm_minute && seconds == 0) {
        alarm_triggered = true;
        current_state = DS_ALARM_RINGING;
        last_buzzer = millis();
        buzzer_state = false;
        lcd_clear();
    }
}

void do_stop_alarm() {
    alarm_triggered = true;
    current_state = DS_TIME;
    noTone(BUZZER);
    buzzer_state = false;
    set_led(0, 1, 0);
    lcd_clear();
    btn1_pressed = false;
    btn2_pressed = false;
    btn3_pressed = false;
    btn4_pressed = false;
}

void do_snooze() {
    snooze_active = true;
    snooze_start = millis();
    alarm_triggered = true;
    current_state = DS_TIME;
    noTone(BUZZER);
    buzzer_state = false;
    set_led(0, 1, 0);

    char buf[17];
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("SNOOZE ACTIVE   ");
    lcd_set_cursor(1, 0);
    sprintf(buf, "Ring in %d min   ", SNOOZE_MINUTES);
    lcd_print(buf);
    delay(2000);
    lcd_clear();

    btn1_pressed = false;
    btn2_pressed = false;
    btn3_pressed = false;
    btn4_pressed = false;
}