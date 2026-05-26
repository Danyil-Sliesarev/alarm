#include <Arduino.h>
#include <IRremote.hpp>
#include "config.h"
#include "ir_wrapper.h"

void ir_init() {
    IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
}

bool ir_available() {
    return IrReceiver.decode();
}

uint8_t ir_get_command() {
    return IrReceiver.decodedIRData.command;
}

void ir_resume() {
    IrReceiver.resume();
}

void ir_restart() {
    IrReceiver.end();
    delay(100);
    IrReceiver.begin(IR_PIN, DISABLE_LED_FEEDBACK);
}