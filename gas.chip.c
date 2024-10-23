// gas.chip.c
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_out;
  timer_t timer;
  uint32_t gas_attr;
} chip_state_t;

// Function that updates the analog output
static void update_output(void *ctx) {
  chip_state_t *chip = (chip_state_t *)ctx;
  double gas_level = attr_read(chip->gas_attr);
  float voltage = (gas_level / 4095.0) * 3.3;
  pin_dac_write(chip->pin_out, voltage);
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  pin_t pin_vcc = pin_init("VCC", INPUT);
  pin_t pin_gnd = pin_init("GND", INPUT);
  chip->pin_out = pin_init("OUT", ANALOG);

  // Create the attribute for the gas level
  chip->gas_attr = attr_init("gas_level", 0.0);

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
