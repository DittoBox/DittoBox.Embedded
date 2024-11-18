#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_out;
  pin_t pin_out2;
  pin_t pin_out3;
  pin_t pin_out4;
  pin_t pin_out5;
  timer_t timer;
  uint32_t gas_oxygen;
  uint32_t gas_co2;
  uint32_t gas_ethylene;
  uint32_t gas_ammonia;
  uint32_t gas_so2;
} chip_state_t;



static void update_output(void *ctx) {
  chip_state_t *chip = (chip_state_t *)ctx;
  double oxygen_level = attr_read(chip->gas_oxygen);
  double co2_level = attr_read(chip->gas_co2);
  double ethylene_level = attr_read(chip->gas_ethylene);
  double ammonia_level = attr_read(chip->gas_ammonia);
  double so2_level = attr_read(chip->gas_so2);

  pin_dac_write(chip->pin_out, (oxygen_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out2, (co2_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out3, (ethylene_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out4, (ammonia_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out5, (so2_level / 4095.0) * 3.3);
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  pin_t pin_vcc = pin_init("VCC", INPUT);
  pin_t pin_gnd = pin_init("GND", INPUT);
  chip->pin_out = pin_init("OUT", ANALOG);
  chip->pin_out2 = pin_init("OUT2", ANALOG);
  chip->pin_out3 = pin_init("OUT3", ANALOG);
  chip->pin_out4 = pin_init("OUT4", ANALOG);
  chip->pin_out5 = pin_init("OUT5", ANALOG);

  chip->gas_oxygen = attr_init("gas_oxygen", 0.0);
  chip->gas_co2 = attr_init("gas_co2", 0.0);
  chip->gas_ethylene = attr_init("gas_ethylene", 0.0);
  chip->gas_ammonia = attr_init("gas_ammonia", 0.0);
  chip->gas_so2 = attr_init("gas_so2", 0.0);

  timer_config_t config = {
    .callback = update_output,
    .user_data = chip,
  };
  chip->timer = timer_init(&config);

  timer_start(chip->timer, 100000, true);

  printf("Gas sensor initialized.\n");
}
