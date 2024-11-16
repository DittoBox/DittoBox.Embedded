#ifndef WOKWI_API_H
#define WOKWI_API_H

#include <stdint.h>
#include <stdbool.h> // Añadir esta línea

typedef uint8_t pin_t;
typedef uint32_t timer_t;

void pin_dac_write(pin_t pin, float value);
pin_t pin_init(const char* name, int mode);
uint32_t attr_init(const char* name, float initial_value);
timer_t timer_init(void* config);
void timer_start(timer_t timer, uint32_t interval, bool repeat);

#endif // WOKWI_API_H