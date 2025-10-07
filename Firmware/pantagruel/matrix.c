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

#include "ec_switch_matrix.h"
#include "matrix.h"
#include "print.h"

const uint32_t extra_switch_pins[] =  EXTRA_SWITCH_PINS;

/* matrix state(1:on, 0:off) MAYBE REMOVE?*/
extern matrix_row_t raw_matrix[MATRIX_ROWS]; // raw values
extern matrix_row_t matrix[MATRIX_ROWS];     // debounced values
extern bool ecsm_update_tuning;

void extra_switch_init(void) {
    for (int i = 0; i < EXTRA_SWITCHES; i++) {
        setPinInputHigh(extra_switch_pins[i]);
    }
}

void matrix_init_custom(void) {
    ecsm_init();
    extra_switch_init();
}

bool extra_switches_scan(matrix_row_t current_matrix[]) {
    bool updated = false;
    matrix_row_t prev_row_state = current_matrix[EXTRA_SWITCH_ROW];
    matrix_row_t curr_row_state = 0;

    for (int i = 0; i < EXTRA_SWITCHES; i++) {
        uint8_t mask = 1 << i;
        curr_row_state |= readPin(extra_switch_pins[i]) ? 0 : mask;
    }

    if (curr_row_state != prev_row_state) {
        current_matrix[EXTRA_SWITCH_ROW] = curr_row_state;
        updated = true;
    }

    return updated;
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool updated = ecsm_matrix_scan(current_matrix);
    updated |= extra_switches_scan(current_matrix);

#ifdef CONSOLE_ENABLE
    #ifdef ECSM_DEBUG
    static int cnt = 0;

    if (cnt++ == 500) {
        cnt = 0;
        ecsm_print_debug();
        ecsm_print_matrix(current_matrix);
        matrix_print();
    }
    #endif
#endif

    return updated;
}
