#ifndef _IR_WRAPPER_H
#define _IR_WRAPPER_H

void ir_init();
void ir_handle();
uint8_t ir_get_command();
bool ir_available();
void ir_resume();
void ir_restart();

#endif