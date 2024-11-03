#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
// Define a structure for the chip's state
typedef struct {
  pin_t pin_out; // Pin for oxygen output
  pin_t pin_out2; // Pin for CO2 output
  pin_t pin_out3; // Pin for ethylene output
  pin_t pin_out4; // Pin for ammonia output
  pin_t pin_out5; // Pin for SO2 output
  timer_t timer; // Timer for periodic updates
  uint32_t gas_oxygen; // Attribute to store oxygen level
  uint32_t gas_co2; // Attribute to store CO2 level
  uint32_t gas_ethylene; // Attribute to store ethylene level
  uint32_t gas_ammonia; // Attribute to store ammonia level
  uint32_t gas_so2; // Attribute to store SO2 level
} chip_state_t;

// Function to update the analog outputs based on gas levels

static void update_output(void *ctx) {
    // Cast the context to the chip state structure
  
  chip_state_t *chip = (chip_state_t *)ctx;
  
    // Read gas levels using attribute functions
  double oxygen_level = attr_read(chip->gas_oxygen);
  double co2_level = attr_read(chip->gas_co2);
  double ethylene_level = attr_read(chip->gas_ethylene);
  double ammonia_level = attr_read(chip->gas_ammonia);
  double so2_level = attr_read(chip->gas_so2);

  // Write the read levels to the DAC output pins, normalizing to a range of 0-3.3V

  
  pin_dac_write(chip->pin_out, (oxygen_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out2, (co2_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out3, (ethylene_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out4, (ammonia_level / 4095.0) * 3.3);
  pin_dac_write(chip->pin_out5, (so2_level / 4095.0) * 3.3);
}

// Initialize the chip and its components

void chip_init() {

    // Allocate memory for the chip state structure

  
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));
  // Initialize the power pins

  pin_t pin_vcc = pin_init("VCC", INPUT);
  pin_t pin_gnd = pin_init("GND", INPUT);

    // Initialize the analog output pins

  
  chip->pin_out = pin_init("OUT", ANALOG);
  chip->pin_out2 = pin_init("OUT2", ANALOG);
  chip->pin_out3 = pin_init("OUT3", ANALOG);
  chip->pin_out4 = pin_init("OUT4", ANALOG);
  chip->pin_out5 = pin_init("OUT5", ANALOG);

  // Initialize gas attributes with initial values

  
  chip->gas_oxygen = attr_init("gas_oxygen", 0.0);
  chip->gas_co2 = attr_init("gas_co2", 0.0);
  chip->gas_ethylene = attr_init("gas_ethylene", 0.0);
  chip->gas_ammonia = attr_init("gas_ammonia", 0.0);
  chip->gas_so2 = attr_init("gas_so2", 0.0);

  // Timer configuration

  
  timer_config_t config = {
    .callback = update_output,
    .user_data = chip,
  };
  chip->timer = timer_init(&config);
  
  // Start the timer to call the function every 100 ms

  timer_start(chip->timer, 100000, true);

  printf("Gas sensor initialized.\n");
}
