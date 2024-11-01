// gas.chip.c
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_out;
  timer_t timer;
  uint32_t gas_oxygen;
  uint32_t gas_co2;
  uint32_t gas_ethylene;
  uint32_t gas_ammonia;
  uint32_t gas_so2;
} chip_state_t;

// Function that updates the analog output
static void update_output(void *ctx) {
  chip_state_t *chip = (chip_state_t *)ctx;
  double oxygen_level = attr_read(chip->gas_oxygen);
  double co2_level = attr_read(chip->gas_co2);
  double ethylene_level = attr_read(chip->gas_ethylene);
  double ammonia_level = attr_read(chip->gas_ammonia);
  double so2_level = attr_read(chip->gas_so2);

  // Example: Calculate a combined voltage or handle each gas type separately
  float voltage = ((oxygen_level + co2_level + ethylene_level + ammonia_level + so2_level) / (5 * 4095.0)) * 3.3;
  pin_dac_write(chip->pin_out, voltage);
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  pin_t pin_vcc = pin_init("VCC", INPUT);
  pin_t pin_gnd = pin_init("GND", INPUT);
  chip->pin_out = pin_init("OUT", ANALOG);

  // Create the attributes for each gas type
  chip->gas_oxygen = attr_init("gas_oxygen", 0.0);
  chip->gas_co2 = attr_init("gas_co2", 0.0);
  chip->gas_ethylene = attr_init("gas_ethylene", 0.0);
  chip->gas_ammonia = attr_init("gas_ammonia", 0.0);
  chip->gas_so2 = attr_init("gas_so2", 0.0);

  // Configure the timer to update the output
  timer_config_t config = {
    .callback = update_output,
    .user_data = chip,
};
  chip->timer = timer_init(&config);

  // Start timer every 100 ms
  timer_start(chip->timer, 100000, true);

  printf("Gas sensor initialized.\n");
}