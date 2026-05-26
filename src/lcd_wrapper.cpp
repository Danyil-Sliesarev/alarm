#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "lcd_wrapper.h"

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);

void lcd_init() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
}

void lcd_clear() { lcd.clear(); }

void lcd_set_cursor(int y, int x) { lcd.setCursor(x, y); }

void lcd_print(char* text) { lcd.print(text); }

void lcd_print_at(int y, int x, char* text) {
    lcd.setCursor(x, y);
    lcd.print(text);
}

void lcd_backlight(bool state) {
    if (state) lcd.backlight();
    else lcd.noBacklight();
}