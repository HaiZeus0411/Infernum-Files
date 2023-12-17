/*
Copyright 2022 ojthetiny

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "infernum.h"

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = {
    {
        // Key Matrix to LED Index
        {21, 0, 1, 2, 3, 4, 5, 6, 7, 8 },
        {NO_LED, NO_LED, NO_LED, NO_LED, 15, 14, NO_LED, NO_LED, NO_LED, NO_LED},
        {20, 19, 18, 17, 16, 13, 12, 11, 10, 9},
        {NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED}
    }, {
        // LED Index to Physical Position
        {32, 15}, {51, 15}, {69, 15}, { 88, 15}, {137, 15}, {157, 15}, {175, 15}, {195, 15}, {215, 15}, // UNDERGLOW
        {220, 45}, {200, 45}, {180, 45}, { 160, 45}, { 142, 45}, // UNDERGLOW
        {140, 29}, {83, 29}, // UNDERGLOW
        {81, 45}, {62, 45}, {44, 45}, {26, 45}, {7, 45}, // UNDERGLOW
        {12, 15} // UNDERGLOW
    }, {
        // LED Index to Flag
        2, 2, 2, 2, 2, 2, 2, 2, 2, // UNDERGLOW
        2, 2, 2, 2, 2, // UNDERGLOW
        2, 2, // UNDERGLOW
        2, 2, 2, 2, 2, // UNDERGLOW
        2 // UNDERGLOW
    }
};
#endif