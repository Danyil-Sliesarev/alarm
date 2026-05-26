#ifndef _SCREENS_H
#define _SCREENS_H

enum DisplayState {
    DS_TIME, DS_DATE, DS_TEMP, DS_ALARM,
    DS_SET_HOUR, DS_SET_MINUTE,
    DS_SET_DAY, DS_SET_MONTH, DS_SET_YEAR,
    DS_SET_ALARM_HOUR, DS_SET_ALARM_MINUTE,
    DS_ALARM_RINGING,
    DS_STOPWATCH,
    DS_TIMER, DS_SET_TIMER_MIN, DS_SET_TIMER_SEC
};

enum screen {
    INIT_SCR,
    CLOCK_SCR,
    ALARM_SCR,
    SHOW_DATE_SCR,
    SHOW_ENV_SCR,
    FACTORY_RESET_SCR
};

void screens_init();
enum screen clock_screen();
enum screen alarm_screen();
enum screen show_date_screen();
enum screen show_env_screen();
enum screen factory_reset_screen();
void handle_ir_command(uint8_t cmd);
void do_stop_alarm();
void do_snooze();

extern DisplayState current_state;
extern int hours, minutes, seconds;
extern unsigned long last_buzzer;
extern bool buzzer_state;
extern bool btn1_pressed, btn2_pressed, btn3_pressed, btn4_pressed;

#endif