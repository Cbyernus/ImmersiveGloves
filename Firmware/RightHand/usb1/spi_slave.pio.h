// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// --------- //
// spi_slave //
// --------- //

#define spi_slave_wrap_target 0
#define spi_slave_wrap 6

static const uint16_t spi_slave_program_instructions[] = {
            //     .wrap_target
    0xe03f, //  0: set    x, 31                      
    0x80a0, //  1: pull   block                      
    0x2021, //  2: wait   0 pin, 1                   
    0x2022, //  3: wait   0 pin, 2                   
    0x6001, //  4: out    pins, 1                    
    0x20a2, //  5: wait   1 pin, 2                   
    0x0042, //  6: jmp    x--, 2                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program spi_slave_program = {
    .instructions = spi_slave_program_instructions,
    .length = 7,
    .origin = -1,
};

static inline pio_sm_config spi_slave_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + spi_slave_wrap_target, offset + spi_slave_wrap);
    return c;
}

static inline void spi_slave_init(PIO pio, uint sm, uint offset, uint outpin, uint numberOfOutPins, uint inpin, uint numberOfInPins) {
    pio_sm_config c = spi_slave_program_get_default_config(offset);
    // Map the state machine's OUT pin group to one pin, namely the `outpin`
    // parameter to this function.
    sm_config_set_out_pins(&c, outpin, numberOfOutPins);
	// Map the state machine's IN pin group to one pin, namely the `inpin`
    // parameter to this function.
    sm_config_set_in_pins(&c, inpin);
    // Set this pin's GPIO function (connect PIO to the pad)
    pio_gpio_init(pio, outpin);
    // Set the pin direction to output at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, outpin, numberOfOutPins, true);
	// Set the pin direction to input at the PIO
	pio_sm_set_consecutive_pindirs(pio, sm, inpin, numberOfInPins, false);
    // Load our configuration, and jump to the start of the program
    pio_sm_init(pio, sm, offset, &c);
    // Set the state machine running
    pio_sm_set_enabled(pio, sm, true);
	sm_config_set_out_shift(&c, true, false, 0);
}

#endif

