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

#pragma once

#include "quantum.h"
#include "matrix.h"

typedef struct
{
    uint16_t actuation; // threshold for key release
    uint16_t release;   // threshold for key press
} ecsm_threshold_t;

typedef struct
{
    bool configured;
    int16_t actuation_offset;
    int16_t release_offset;
    int16_t idle[MATRIX_ROWS][MATRIX_COLS];
} ecsm_config_t;

ecsm_config_t ecsm_config;

/// @brief Set default actuation points from presistent storage 
void ecsm_config_init(void);

/// @brief Increment acutation point depth/travel distance
void ecsm_ap_inc(void);

/// @brief Decrement acutation point depth/travel distance
void ecsm_ap_dec(void);

/// @brief Writes current acutation points to presistent storage 
void ecsm_config_update(void);

/// @brief hardware initialization for EC matrix
void ecsm_init(void);

/// @brief clears all ec config from persistent storage 
void ecsm_eeprom_clear(void);

bool ecsm_matrix_scan(matrix_row_t current_matrix[]);

/// Read capacitive sensor raw ADC value
uint16_t ecsm_readkey_raw(uint8_t row, uint8_t col);

bool ecsm_update_key(matrix_row_t *current_row, uint8_t row, uint8_t col, uint16_t sw_value);
void ecsm_update_tuning_data(int16_t new_data, uint8_t, uint8_t col);

/// @brief Update actuation points based on idle readings and configured offset
void ecsm_update_thresholds(void);

/// @brief Print ADC readings
void ecsm_print_matrix(matrix_row_t current_matrix[]);

/// @brief Print saved config 
void ecsm_print_debug(void);
