#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "screens.h"
#include "lcd_wrapper.h"
#include "rtc_wrapper.h"
#include "sensors.h"
#include "ir_wrapper.h"

#define DISABLE_LED_FEEDBACK false

extern void set_led(int r, int g, int b);
extern volatile byte btn_state;

enum DisplayState {
    DS_TIME, DS_DATE, DS_TEMP, DS_ALARM,
    DS_SET_HOUR, DS_SET_MINUTE,
    DS_SET_DAY, DS_SET_MONTH, DS_SET_YEAR,
    DS_SET_ALARM_HOUR, DS_SET_ALARM_MINUTE,
    DS_ALARM_RINGING,
    DS_STOPWATCH,
    DS_TIMER, DS_SET_TIMER_MIN, DS_SET_TIMER_SEC
};

static DisplayState current_state = DS_TIME;

static int hours = 0, minutes = 0, seconds = 0;
static int day = 1, month = 1, year = 2026;

static int alarm_hour = 7, alarm_minute = 0;
static bool alarm_enabled = true;
static bool alarm_triggered = false;

static float temperature = 0;
static float humidity = 0;

static unsigned long last_update = 0;
static unsigned long last_blink = 0;
static unsigned long last_buzzer = 0;
static bool blink_state = false;
static bool buzzer_state = false;

static bool btn1_pressed = false, btn2_pressed = false;
static bool btn3_pressed = false, btn4_pressed = false;

static unsigned long sw_start = 0;
static unsigned long sw_elapsed = 0;
static bool sw_running = false;

static int timer_set_min = 0;
static int timer_set_sec = 0;
static unsigned long timer_start = 0;
static unsigned long timer_duration = 0;
static bool timer_running = false;
static bool timer_finished = false;
static unsigned long timer_finish_buzzer = 0;
static bool timer_finish_buzzer_state = false;

#define SNOOZE_MINUTES 5
static bool snooze_active = false;
static unsigned long snooze_start = 0;

static int get_days_in_month(int m, int y) {
    if (m == 2) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) return 29;
        return 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
}

static void alarm_load() {
    alarm_hour = EEPROM.read(EEPROM_ALARM_HOUR);
    alarm_minute = EEPROM.read(EEPROM_ALARM_MINUTE);
    if (alarm_hour > 23) alarm_hour = 7;
    if (alarm_minute > 59) alarm_minute = 0;
}

static void alarm_save() {
    EEPROM.update(EEPROM_ALARM_HOUR, alarm_hour);
    EEPROM.update(EEPROM_ALARM_MINUTE, alarm_minute);
}

static void update_time() {
    if (current_state == DS_ALARM_RINGING) return;
    if (current_state == DS_SET_HOUR || current_state == DS_SET_MINUTE ||
        current_state == DS_SET_DAY  || current_state == DS_SET_MONTH  ||
        current_state == DS_SET_YEAR) return;

    struct dt t = now();
    if (t.minutes != minutes) alarm_triggered = false;
    hours = t.hours;
    minutes = t.minutes;
    seconds = t.seconds;
    day = t.day;
    month = t.month;
    year = t.year;
}

static void check_alarm() {
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

static void do_snooze() {
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


static void handle_mode() {
    switch (current_state) {
        case DS_TIME:      current_state = DS_TEMP; break;
        case DS_TEMP:      current_state = DS_ALARM; break;
        case DS_ALARM:     current_state = DS_STOPWATCH; break;
        case DS_STOPWATCH: current_state = DS_TIMER; break;
        case DS_TIMER:     current_state = DS_TIME; break;
        default: break;
    }
    lcd_clear();
}

static void handle_up() {
    switch (current_state) {
        case DS_SET_HOUR: hours = (hours + 1) % 24; break;
        case DS_SET_MINUTE: minutes = (minutes + 1) % 60; break;
        case DS_SET_DAY: day++; if (day > get_days_in_month(month, year)) day = 1; break;
        case DS_SET_MONTH: month = (month % 12) + 1; break;
        case DS_SET_YEAR: year++; if (year > 2099) year = 2000; break;
        case DS_SET_ALARM_HOUR: alarm_hour = (alarm_hour + 1) % 24; break;
        case DS_SET_ALARM_MINUTE: alarm_minute = (alarm_minute + 1) % 60; break;
        case DS_SET_TIMER_MIN: timer_set_min = (timer_set_min + 1) % 100; break;
        case DS_SET_TIMER_SEC: timer_set_sec = (timer_set_sec + 1) % 60; break;
        case DS_STOPWATCH:
            if (!sw_running) {
                sw_start = millis() - sw_elapsed;
                sw_running = true;
            } else {
                sw_elapsed = millis() - sw_start;
                sw_running = false;
            }
            break;
        case DS_TIMER:
    if (timer_finished) {
        timer_finished = false;
        noTone(BUZZER);
        set_led(0, 1, 0);
    } else if (!timer_running) {
        timer_duration = ((unsigned long)timer_set_min * 60 + timer_set_sec) * 1000UL;
        if (timer_duration > 0) {
            timer_start = millis();
            timer_running = true;
        }
    } else {
        timer_running = false;
    }
    break;
        default: break;
    }
}

static void handle_down() {
    switch (current_state) {
        case DS_SET_HOUR: hours = (hours == 0) ? 23 : hours - 1; break;
        case DS_SET_MINUTE: minutes = (minutes == 0) ? 59 : minutes - 1; break;
        case DS_SET_DAY: day--; if (day < 1) day = get_days_in_month(month, year); break;
        case DS_SET_MONTH: month = (month == 1) ? 12 : month - 1; break;
        case DS_SET_YEAR: year--; if (year < 2000) year = 2099; break;
        case DS_SET_ALARM_HOUR: alarm_hour = (alarm_hour == 0) ? 23 : alarm_hour - 1; break;
        case DS_SET_ALARM_MINUTE: alarm_minute = (alarm_minute == 0) ? 59 : alarm_minute - 1; break;
        case DS_SET_TIMER_MIN: timer_set_min = (timer_set_min == 0) ? 99 : timer_set_min - 1; break;
        case DS_SET_TIMER_SEC: timer_set_sec = (timer_set_sec == 0) ? 59 : timer_set_sec - 1; break;
        case DS_STOPWATCH:
            sw_running = false;
            sw_elapsed = 0;
            lcd_clear();
            break;
        case DS_TIMER:
            timer_running = false;
            timer_finished = false;
            noTone(BUZZER);
            set_led(0, 1, 0);
            lcd_clear();
            break;
        default: break;
    }
}

static void handle_ok() {
    switch (current_state) {
        case DS_TIME:  current_state = DS_SET_HOUR; break;
        case DS_ALARM: current_state = DS_SET_ALARM_HOUR; break;
        case DS_TIMER: current_state = DS_SET_TIMER_MIN; break;
        case DS_SET_HOUR: current_state = DS_SET_MINUTE; break;
        case DS_SET_MINUTE:
            set_time(hours, minutes, 0);
            current_state = DS_SET_DAY;
            break;
        case DS_SET_DAY: current_state = DS_SET_MONTH; break;
        case DS_SET_MONTH: current_state = DS_SET_YEAR; break;
        case DS_SET_YEAR:
            set_datetime(day, month, year, hours, minutes, seconds);
            current_state = DS_TIME;
            break;
        case DS_SET_ALARM_HOUR: current_state = DS_SET_ALARM_MINUTE; break;
        case DS_SET_ALARM_MINUTE:
            alarm_save();
            alarm_triggered = false;
            current_state = DS_ALARM;
            break;
        case DS_SET_TIMER_MIN: current_state = DS_SET_TIMER_SEC; break;
        case DS_SET_TIMER_SEC:
            current_state = DS_TIMER;
            break;
        default: break;
    }
    lcd_clear();
}

void handle_ir_command(uint8_t cmd) {
    if (current_state == DS_ALARM_RINGING) {
        if (cmd == IR_DOWN) {
            ir_restart();
            do_snooze();
        } else {
            do_stop_alarm();
            ir_restart();
        }
        return;
    }

    if (timer_finished) {
        timer_finished = false;
        timer_running = false;
        noTone(BUZZER);
        set_led(0, 1, 0);
        lcd_clear();
        ir_restart();
        return;
    }

    switch (cmd) {
        case IR_MODE: handle_mode(); break;
        case IR_UP:   handle_up();   break;
        case IR_DOWN: handle_down(); break;
        case IR_OK:   handle_ok();   break;
        case IR_STOP:
            if (current_state == DS_STOPWATCH) {
                sw_running = false;
                sw_elapsed = 0;
                lcd_clear();
            } else if (current_state == DS_TIMER) {
                timer_running = false;
                timer_finished = false;
                noTone(BUZZER);
                set_led(0, 1, 0);
                lcd_clear();
            } else {
                do_stop_alarm();
            }
            ir_restart();
            break;
    }
}

static void handle_buttons() {
    bool btn1 = (btn_state & BTN1_PRESSED) != 0;
    bool btn2 = (btn_state & BTN2_PRESSED) != 0;
    bool btn3 = (btn_state & BTN3_PRESSED) != 0;
    bool btn4 = (btn_state & BTN4_PRESSED) != 0;

    if (current_state == DS_ALARM_RINGING) {
    if (btn3 && !btn3_pressed) {
        btn3_pressed = true;
        do_snooze();
    } else if (!btn3) {
        btn3_pressed = false;
    }
    if (btn1 || btn2 || btn4) do_stop_alarm();
    return;
}

    if (btn1 && !btn1_pressed) { btn1_pressed = true; handle_mode(); }
    else if (!btn1) btn1_pressed = false;

    if (btn2 && !btn2_pressed) { btn2_pressed = true; handle_up(); }
    else if (!btn2) btn2_pressed = false;

    if (btn3 && !btn3_pressed) { btn3_pressed = true; handle_down(); }
    else if (!btn3) btn3_pressed = false;

    if (btn4 && !btn4_pressed) { btn4_pressed = true; handle_ok(); }
    else if (!btn4) btn4_pressed = false;
}

static void handle_ir() {
    if (!ir_available()) return;
    uint8_t cmd = ir_get_command();
    ir_resume();
    handle_ir_command(cmd);
}

static void display_time() {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "%02d:%02d:%02d        ", hours, minutes, seconds);
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    sprintf(buf, "%02d/%02d/%04d      ", day, month, year);
    lcd_print(buf);
}

static void display_date() {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "%02d/%02d/%04d      ", day, month, year);
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    sprintf(buf, "%02d:%02d:%02d        ", hours, minutes, seconds);
    lcd_print(buf);
}

static void display_temp() {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "Temp:%dC        ", (int)temperature);
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    sprintf(buf, "Himidity:%d ", (int)humidity);
    lcd_print(buf);
}

static void display_alarm() {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "Alarm +5m %s     ", snooze_active ? "[ON]" : "[OFF]");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    sprintf(buf, "SET: %02d:%02d      ", alarm_hour, alarm_minute);
    lcd_print(buf);
}

static void display_set_time(bool bh, bool bm) {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "SET TIME        ");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    if (!bh || blink_state) sprintf(buf, "%02d", hours);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print(":");
    if (!bm || blink_state) sprintf(buf, "%02d", minutes);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print(":00       ");
}

static void display_set_date(bool bd, bool bm, bool by) {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "SET DATE        ");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    if (!bd || blink_state) sprintf(buf, "%02d", day);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print("/");
    if (!bm || blink_state) sprintf(buf, "%02d", month);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print("/");
    if (!by || blink_state) sprintf(buf, "%04d", year);
    else sprintf(buf, "    ");
    lcd_print(buf);
    lcd_print("   ");
}

static void display_set_alarm(bool bh, bool bm) {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "SET ALARM ");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    if (!bh || blink_state) sprintf(buf, "%02d", alarm_hour);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print(":");
    if (!bm || blink_state) sprintf(buf, "%02d", alarm_minute);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print("          ");
}

static void display_alarm_ringing() {
    char buf[17];
    lcd_set_cursor(0, 0);
    if (blink_state) sprintf(buf, "*** ALARM ***   ");
    else sprintf(buf, "                ");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    sprintf(buf, "OK=off BTN3=+5m ");
    lcd_print(buf);
}

static void display_stopwatch() {
    char buf[17];
    unsigned long elapsed = sw_running ? (millis() - sw_start) : sw_elapsed;

    unsigned long total_sec = elapsed / 1000;
    unsigned long ms = (elapsed % 1000) / 10; // сотые
    unsigned long sw_min = total_sec / 60;
    unsigned long sw_sec = total_sec % 60;

    lcd_set_cursor(0, 0);
    sprintf(buf, "STOPWATCH ");
    lcd_print(buf);

    lcd_set_cursor(1, 0);
    if (sw_min == 0) {
        sprintf(buf, "%02lu.%02lu sec      ", sw_sec, ms);
    } else {
        sprintf(buf, "%02lu:%02lu          ", sw_min, sw_sec);
    }
    lcd_print(buf);
}

static void display_timer() {
    char buf[17];
    lcd_set_cursor(0, 0);

    if (timer_finished) {
        if (blink_state) sprintf(buf, "** DONE! **   ");
        else sprintf(buf, "                ");
        lcd_print(buf);
        lcd_set_cursor(1, 0);
        sprintf(buf, "  00:00         ");
        lcd_print(buf);
        return;
    }

    sprintf(buf, "TIMER  %s       ", timer_running ? "RUN" : " ");
    lcd_print(buf);

    lcd_set_cursor(1, 0);
    if (timer_running) {
        unsigned long elapsed = millis() - timer_start;
        long remaining = (long)timer_duration - (long)elapsed;
        if (remaining < 0) remaining = 0;
        unsigned long rem_sec = remaining / 1000;
        unsigned long rem_min = rem_sec / 60;
        rem_sec = rem_sec % 60;
        sprintf(buf, "%02lu:%02lu          ", rem_min, rem_sec);
    } else {
        sprintf(buf, "%02d:%02d          ", timer_set_min, timer_set_sec);
    }
    lcd_print(buf);
}

static void display_set_timer(bool bm, bool bs) {
    char buf[17];
    lcd_set_cursor(0, 0);
    sprintf(buf, "SET TIMER       ");
    lcd_print(buf);
    lcd_set_cursor(1, 0);
    if (!bm || blink_state) sprintf(buf, "%02d", timer_set_min);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print(":");
    if (!bs || blink_state) sprintf(buf, "%02d", timer_set_sec);
    else sprintf(buf, "  ");
    lcd_print(buf);
    lcd_print("          ");
}

static void update_display() {
    switch (current_state) {
        case DS_TIME:            display_time(); break;
        case DS_DATE:            display_time(); break;
        case DS_TEMP:            display_temp(); break;
        case DS_ALARM:           display_alarm(); break;
        case DS_SET_HOUR:        display_set_time(true, false); break;
        case DS_SET_MINUTE:      display_set_time(false, true); break;
        case DS_SET_DAY:         display_set_date(true, false, false); break;
        case DS_SET_MONTH:       display_set_date(false, true, false); break;
        case DS_SET_YEAR:        display_set_date(false, false, true); break;
        case DS_SET_ALARM_HOUR:  display_set_alarm(true, false); break;
        case DS_SET_ALARM_MINUTE:display_set_alarm(false, true); break;
        case DS_ALARM_RINGING:   display_alarm_ringing(); break;
        case DS_STOPWATCH:       display_stopwatch(); break;
        case DS_TIMER:           display_timer(); break;
        case DS_SET_TIMER_MIN:   display_set_timer(true, false); break;
        case DS_SET_TIMER_SEC:   display_set_timer(false, true); break;
    }
}


enum screen clock_screen() {
    static bool initialized = false;
    if (!initialized) {
        alarm_load();
        struct dt t = now();
        hours = t.hours;
        minutes = t.minutes;
        seconds = t.seconds;
        day = t.day;
        month = t.month;
        year = t.year;
        initialized = true;
    }

    unsigned long current_millis = millis();

    if (current_millis - last_update >= 1000) {
        last_update = current_millis;
        update_time();
        temperature = get_temperature();
        humidity = get_humidity();
    }

    handle_buttons();
    handle_ir();
    check_alarm();

    if (current_state == DS_ALARM_RINGING) {
        if (current_millis - last_buzzer >= 500) {
            last_buzzer = current_millis;
            buzzer_state = !buzzer_state;
            if (buzzer_state) {
                tone(BUZZER, 1000, 400);
                set_led(1, 0, 0);
            } else {
                set_led(0, 0, 0);
            }
        }
    }

    if (timer_running) {
        unsigned long elapsed = current_millis - timer_start;
        if (elapsed >= timer_duration) {
            timer_running = false;
            timer_finished = true;
            timer_finish_buzzer = current_millis;
        }
    }

    if (timer_finished) {
        if (current_millis - timer_finish_buzzer >= 500) {
            timer_finish_buzzer = current_millis;
            timer_finish_buzzer_state = !timer_finish_buzzer_state;
            if (timer_finish_buzzer_state) {
                tone(BUZZER, 800, 400);
                set_led(0, 0, 1);
            } else {
                set_led(0, 0, 0);
            }
        }
        if (btn_state != 0) {
            timer_finished = false;
            noTone(BUZZER);
            set_led(0, 1, 0);
            lcd_clear();
        }
    }

    if (current_millis - last_blink >= 300) {
        last_blink = current_millis;
        blink_state = !blink_state;
    }

    update_display();
    return CLOCK_SCR;
}

enum screen alarm_screen() { return CLOCK_SCR; }
enum screen show_date_screen() { return CLOCK_SCR; }
enum screen show_env_screen() { return CLOCK_SCR; }
enum screen factory_reset_screen() { return CLOCK_SCR; }