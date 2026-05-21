#ifndef CONFIG_H
#define CONFIG_H

#define BAUD_RATE 9600

#define DHT_PIN 8
#define DHT_TYPE DHT11

#define RTC_CLK A0
#define RTC_DAT A1
#define RTC_RST A2

#define BUZZER 6

#define LCD_I2C_ADDRESS 0x27
#define LCD_ROWS 2
#define LCD_COLS 16

#define BTN1 7
#define BTN2 10
#define BTN3 11
#define BTN4 12

#define LED_R 13
#define LED_G 2
#define LED_B 3

#define IR_PIN 4
#define IR_MODE  0x45
#define IR_UP    0x18
#define IR_DOWN  0x52
#define IR_OK    0x1C
#define IR_STOP  0x16

#define EEPROM_ALARM_HOUR 0
#define EEPROM_ALARM_MINUTE 1

#define BTN1_PRESSED 0b00000001
#define BTN2_PRESSED 0b00000010
#define BTN3_PRESSED 0b00000100
#define BTN4_PRESSED 0b00001000

#define SENSORS_READ_INTERVAL 60
#define FACTORY_RESET_INTERVAL 3000

#endif