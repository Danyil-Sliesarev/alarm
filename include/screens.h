#ifndef _SCREENS_H
#define _SCREENS_H

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

#endif