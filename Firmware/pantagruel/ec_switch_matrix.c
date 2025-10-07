/* Copyright 2023 sporkus
 * Copyright 2023 Cipulot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include "ec_switch_matrix.h"
#include "analog.h"
#include "atomic_util.h"
#include "print.h"

/* Pin and port array */
const uint32_t row_pins[]     = EC_MATRIX_ROW_PINS;
const uint8_t  col_channels[] = EC_MATRIX_COL_CHANNELS;
const uint32_t mux_sel_pins[] = MUX_SEL_PINS;
const uint32_t mux_en_pins[] =  MUX_EN_PINS;

static adc_mux adcMux;
static uint16_t ecsm_sw_value[EC_MATRIX_ROWS][EC_MATRIX_COLS];

static ecsm_threshold_t ecsm_thresholds[EC_MATRIX_ROWS][EC_MATRIX_COLS];
static int16_t ecsm_tuning_data[EC_MATRIX_ROWS][EC_MATRIX_COLS];
static uint32_t ecsm_is_tuning = 1e5; // Tunes ec config until this counter reaches 0

/* fancy printing */
const char* red = "\x1b[31m";
const char* reset = "\x1b[0m";

static inline void discharge_capacitor(void) {
    writePinLow(DISCHARGE_PIN);
}

static inline void charge_capacitor(uint8_t row) {
    writePinHigh(DISCHARGE_PIN); // Blocks discharge route
    writePinHigh(row_pins[row]); // Send signal to row
}

static inline void disable_mux(uint8_t i) {
    writePinHigh(mux_en_pins[i]);
}

static inline void disable_mux_all(void) {
    for (int i = 0; i < 2; i++) {
        disable_mux(i);
    }
}

static inline void enable_mux(uint8_t i) {
    writePinLow(mux_en_pins[i]);
}

static inline void select_col(uint8_t col) {
    uint8_t ch = col_channels[col];
    uint8_t active_mux = (ch & 8) ? 1 : 0;

    disable_mux(!active_mux);
    writePin(mux_sel_pins[0], ch & 1);
    writePin(mux_sel_pins[1], ch & 2);
    writePin(mux_sel_pins[2], ch & 4);
    enable_mux(active_mux);
}

/// @brief hardware initialization for row pins
static inline void init_row(void) {
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        setPinOutput(row_pins[i]);
        writePinLow(row_pins[i]);
    }
}

/// @brief hardware initialization for mux
static inline void init_mux(void) {
    for (int i = 0; i < 2; i++) {
        setPinOutput(mux_en_pins[i]);
    }

    for (int idx = 0; idx < 3; idx++) {
        setPinOutput(mux_sel_pins[idx]);
    }
}

void ecsm_config_init(void) {
    eeconfig_read_kb_datablock(&ecsm_config);
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            if (! ecsm_config.configured) {
                // fallback to default values
                ecsm_config.actuation_offset = ACTUATION_OFFSET;
                ecsm_config.release_offset = RELEASE_OFFSET;
                ecsm_config.idle[i][j] = DEFAULT_IDLE;
            }
            ecsm_tuning_data[i][j] = ecsm_config.idle[i][j];
        }
    }
    ecsm_update_thresholds();
}

void ecsm_config_update(void) {
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            ecsm_config.idle[i][j] = ecsm_tuning_data[i][j];
        }
    }

    uprintf("Writing current actuation points to presistent storage\n");
    eeconfig_update_kb_datablock(&ecsm_config);
    ecsm_print_debug();
}

void ecsm_eeprom_clear(void) {
    uprintln("\nClearing EC config");
    ecsm_config.configured = 0;
    ecsm_config.actuation_offset = ACTUATION_OFFSET;
    ecsm_config.release_offset = RELEASE_OFFSET;
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            ecsm_config.idle[i][j] = 0;
        }
    }
    eeconfig_update_kb_datablock(&ecsm_config);
    ecsm_is_tuning = 1e5;
}

void ecsm_ap_inc(void) {
    int16_t max_offset = 400;
    uprintf("\nIncreasing actuation point (less sensitive)\n");
    int16_t offset_diff = ACTUATION_OFFSET - RELEASE_OFFSET;
    ecsm_config.actuation_offset += 15;
    ecsm_config.release_offset += 15;

    if (ecsm_config.actuation_offset > max_offset || ecsm_config.release_offset > max_offset) {
        uprintf("\nActuation point at maximum\n");
        ecsm_config.actuation_offset = max_offset;
        ecsm_config.release_offset = max_offset;

        if (offset_diff > 0) {
            ecsm_config.release_offset = ecsm_config.actuation_offset - offset_diff;
        } else {
            ecsm_config.actuation_offset = ecsm_config.release_offset - offset_diff;
        }
    }

    ecsm_update_thresholds();
    ecsm_config_update();
}

void ecsm_ap_dec(void) {
    int16_t min_offset = 50;
    uprintf("\nDecreasing actuation point (more sensitive)\n");
    int16_t offset_diff = ACTUATION_OFFSET - RELEASE_OFFSET;
    ecsm_config.actuation_offset -= 15;
    ecsm_config.release_offset -= 15;

    if (ecsm_config.actuation_offset < min_offset || ecsm_config.release_offset < min_offset) {
        uprintf("\nActuation point at minimum\n");
        ecsm_config.actuation_offset = min_offset;
        ecsm_config.release_offset = min_offset;

        if (offset_diff > 0) {
            ecsm_config.actuation_offset = ecsm_config.release_offset + offset_diff;
        } else {
            ecsm_config.release_offset = ecsm_config.actuation_offset + offset_diff;
        }
    }

    ecsm_update_thresholds();
    ecsm_config_update();
}


void ecsm_init(void) {
    palSetLineMode(ANALOG_PORT, PAL_MODE_INPUT_ANALOG);
    adcMux = pinToMux(ANALOG_PORT);
    //Dummy call to make sure that adcStart() has been called in the appropriate state
    adc_read(adcMux);

    // Initialize discharge pin as discharge mode
    writePinLow(DISCHARGE_PIN);
    setPinOutputOpenDrain(DISCHARGE_PIN);

    init_row();
    init_mux();
    ecsm_config_init();
}

void ecsm_update_tuning_data(int16_t new_value, uint8_t row, uint8_t col) {
    if (new_value < DEFAULT_IDLE + ACTUATION_OFFSET) {
        float curr = ecsm_tuning_data[row][col];
        float adjusted = curr + ((float)new_value - curr) * 0.02;
        ecsm_tuning_data[row][col] = round(adjusted);
    }
}

void ecsm_update_thresholds(void) {
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            int16_t idle = ecsm_tuning_data[i][j];
            ecsm_thresholds[i][j].actuation =  idle + ecsm_config.actuation_offset;
            ecsm_thresholds[i][j].release = idle + ecsm_config.release_offset;
        }
    }
}

uint16_t ecsm_readkey_raw(uint8_t row, uint8_t col) {
    uint16_t sw_value = 0;

    select_col(col);
    // Set strobe pins to idle state
    writePinLow(row_pins[row]);
    ATOMIC_BLOCK_FORCEON {
        charge_capacitor(row);
        __asm__ __volatile__("nop;nop;nop;\n\t");
        sw_value = adc_read(adcMux);
    }
    // reset sensor
    writePinLow(row_pins[row]);
    discharge_capacitor();
    return sw_value;
}

/// Comparing ADC reading to threshold to update press/release state of key
bool ecsm_update_key(matrix_row_t* current_row, uint8_t row, uint8_t col, uint16_t sw_value) {
    bool current_state = (*current_row >> col) & 1;

    // Press to release
    if (current_state && sw_value < ecsm_thresholds[row][col].release) {
        *current_row &= ~(1 << col);
        return true;
    }

    // Release to press
    if (!current_state && sw_value > ecsm_thresholds[row][col].actuation) {
        *current_row |= (1 << col);
        return true;
    }

    return false;
}

void ecsm_print_matrix(matrix_row_t current_matrix[]) {
    uprintln();
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        uprintf("[ADC readings]: ");
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            bool key_pressed = (current_matrix[i] >> j) & 1;

            if (key_pressed) {
                uprintf("%s%4u%s," , red, ecsm_sw_value[i][j], reset);
            } else {
                uprintf("%4u," , ecsm_sw_value[i][j]);
            }
        }
        uprintln();
    }
    uprintln();
}

void ecsm_print_debug(void) {
    uprintln();
    bool tuned = ecsm_config.configured;

    uprintf("Actuation/release offset: %u, %u\n", ecsm_config.actuation_offset, ecsm_config.release_offset);

    if (!tuned) {
        uprintln("EC config tuning....");
    }

    uprintf("Current idle readings:\n");
    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            if (!tuned) {
                uprintf("%4u  ", ecsm_tuning_data[i][j]);
            } else {
                uprintf("%4u  ", ecsm_config.idle[i][j]);
            }

        }
        uprintln();
    }


    uprintf("\nActuation points:\n");

    for (int i = 0; i < EC_MATRIX_ROWS; i++) {
        for (int j = 0; j < EC_MATRIX_COLS; j++) {
            uprintf("%4u  ", ecsm_thresholds[i][j].actuation);
        }
        uprintln();
    }
    uprintln();
}

// Scan key values and update matrix state
bool ecsm_matrix_scan(matrix_row_t current_matrix[]) {
    bool updated = false;

    for (int col = 0; col < EC_MATRIX_COLS; col++) {
        for (int row = 0; row < EC_MATRIX_ROWS; row++) {
            uint16_t adc = ecsm_readkey_raw(row, col);
            ecsm_sw_value[row][col] = adc;
            updated |= ecsm_update_key(&current_matrix[row], row, col, adc);

            if (! ecsm_config.configured) {
                if (ecsm_is_tuning > 0) {
                    ecsm_is_tuning--;
                }
                ecsm_update_tuning_data(adc, row, col);

                if (ecsm_is_tuning == 0) {
                    uprintln("EC config tuning completed");
                    ecsm_update_thresholds();
                    ecsm_config.configured = true;
                    ecsm_config_update();
                }
            }
        }
    }

    return updated;
}