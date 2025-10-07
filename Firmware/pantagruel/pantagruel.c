#include "pantagruel.h"

extern ecsm_config_t ecsm_config;

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch(keycode) {
            case EC_AP_I:
                ecsm_ap_inc();
                return false;
            case EC_AP_D:
                ecsm_ap_dec();
                return false;
            case EC_CLR:
                ecsm_eeprom_clear();
                return false;
        }
    }

    return process_record_user(keycode, record);
};


void keyboard_post_init_kb(void) {
    #ifdef ECSM_TUNE_ON_BOOT
        ecsm_config.configured = 0;
        eeconfig_update_kb_datablock(&ecsm_config);
    #endif
    keyboard_post_init_user();
}
