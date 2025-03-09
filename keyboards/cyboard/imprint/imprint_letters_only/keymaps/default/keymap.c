/* Copyright 2023 Cyboard LLC (@Cyboard-DigitalTailor)
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
// clang-format off
#include <stdbool.h>
#include <stdint.h>
#include "action_layer.h"
#include "action_util.h"
#include "caps_word.h"
#include "color.h"
#include "debug.h"
#include "info_config.h"
#include "keyboard.h"
#include "keycodes.h"
#include "keymap_us.h"
#include "modifiers.h"
#include "pointing_device.h"
#include "pointing_device_auto_mouse.h"
#include "process_caps_word.h"
#include "process_tap_dance.h"
#include "quantum.h"
#include "quantum_keycodes.h"
#include "rgb_matrix.h"
#include "timer.h"
#include QMK_KEYBOARD_H
#include <cyboard.h>

enum layer_names {
    _BASE,
    _SYMBOL,
    _NAV
};

const uint16_t num_layer_timeout = 5000;
static uint16_t num_layer_idle_timer = 0;
static bool num_layer_word_active = false;

void num_layer_word_on(void) {
    if (num_layer_word_active) {
        return;
    }
    num_layer_word_active = true;
    num_layer_idle_timer = timer_read() + num_layer_timeout;
    layer_on(_SYMBOL);
}

void num_layer_word_off(void) {
    if (!num_layer_word_active) {
        return;
    }

    layer_off(_SYMBOL);
    num_layer_word_active = false;
}

bool process_num_layer_word(uint16_t keycode, keyrecord_t* record) {
    if (num_layer_word_active) {
        switch (keycode) {
        case KC_1 ... KC_0:
        case KC_COMM:
        case KC_DOT:
        case KC_DEL:
        case KC_BSPC:
        case KC_LSFT:
        case QK_ONE_SHOT_MOD:
        case OSM(MOD_LSFT):
            break;
        default:
            num_layer_word_off();
            break;
        }
    }

    num_layer_idle_timer = record->event.time + num_layer_timeout;
    return true;
}

void num_layer_idle_task(void) {
    if (num_layer_word_active && timer_expired(timer_read(), num_layer_idle_timer)) {
        caps_word_off();
    }
}

void housekeeping_task_user(void) {
    num_layer_idle_task();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_num_layer_word(keycode, record)) {
        return false;
    }
    return true;
}

void dance_one_shot_num_word(tap_dance_state_t *state, void *user_data) {
    switch (state->count) {
        case 1:
            set_oneshot_layer(_SYMBOL, ONESHOT_START);
            break;
        case 2:
            num_layer_word_on();
            break;
    }
}

void dance_one_shot_num_word_reset(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        clear_oneshot_layer_state(ONESHOT_PRESSED);
    }
}

enum tap_dances {
    TD_OSL_NUM_WORD
};

tap_dance_action_t tap_dance_actions[] = {
    [TD_OSL_NUM_WORD] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_one_shot_num_word, dance_one_shot_num_word_reset),
};

#define UK_RST RIGHT_SNIPING_MODE_TOGGLE
#define UK_RDM RIGHT_DRAGSCROLL_MODE
#define UK_SRC LSFT(MS_BTN2)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [_BASE] = LAYOUT_let(
        KC_TAB,        KC_B,          KC_Y,            KC_O,     KC_U,          KC_QUOT,                                KC_COMM,       KC_L,          KC_D,    KC_W,          KC_V,          KC_SLSH,
        KC_ESC,        LCTL_T(KC_C),  LALT_T(KC_I),    KC_E,     LGUI_T(KC_A),  KC_Z,                                   KC_Q,          RGUI_T(KC_H),  KC_T,    RALT_T(KC_S),  RCTL_T(KC_N),  KC_SCLN,
        TG(_SYMBOL),   KC_G,          KC_X,            KC_J,     KC_K,          KC_MINUS,                               KC_DOT,        KC_R,          KC_M,    KC_F,          KC_P,          KC_BSLS,
                                      KC_LEFT,         KC_RIGHT, QK_LEAD,       OSM(MOD_HYPR), KC_MUTE,       KC_VOLU,  OSM(MOD_MEH),  KC_ENT,        KC_UP,   KC_DOWN,
                                                                 KC_BSPC,       TD(TD_OSL_NUM_WORD),  KC_MCTL,       KC_VOLD,  OSM(MOD_LSFT), KC_SPC
    ),

    [_SYMBOL] = LAYOUT_let(
        KC_TILD,    KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                              KC_6,    KC_7,    KC_8,    KC_9,    KC_0, _______,
        KC_GRV,  _______, KC_PLUS,  KC_EQL, KC_RBRC, KC_RPRN,                           KC_LPRN, KC_LBRC, _______, _______, _______, _______,
        _______, _______, _______, _______, KC_RCBR, _______,                           _______, KC_LCBR, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [_NAV] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, MS_BTN1, MS_BTN2,  UK_RST, _______, _______,                            UK_RDM, _______, _______, _______, _______, _______,
        _______, _______,  UK_SRC, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,        MS_BTN1, MS_BTN2, UK_SRC, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [3] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [4] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [5] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [6] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [7] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [8] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [9] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    )
};

void leader_end_user(void) {
    if (leader_sequence_one_key(KC_SPC)) {
        tap_code16(KC_UNDS);
    } else if (leader_sequence_one_key(KC_M)) {
        auto_mouse_toggle();
        layer_invert(_NAV);
    } else if (leader_sequence_one_key(KC_DOT)) {
        SEND_STRING("->");
    } else if (leader_sequence_four_keys(KC_B, KC_O, KC_O, KC_T)) {
        tap_code16(QK_BOOT);
    }
}

bool leader_add_user(uint16_t keycode) {
    if (leader_sequence_one_key(KC_SPC)) {
        return true;
    }
    if (leader_sequence_one_key(KC_M)) {
        return true;
    }
    if (leader_sequence_one_key(KC_DOT)) {
        return true;
    }

    return false;
}

void keyboard_post_init_user(void) {
    pointing_device_set_cpi_on_side(true, 100);
    pointing_device_set_cpi_on_side(false, 1000);
}

void pointing_device_init_user(void) {
    set_auto_mouse_layer(_NAV);
    set_auto_mouse_enable(true);
    charybdis_set_pointer_dragscroll_enabled(true, true);
}

bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        // Keycodes that continue Caps Word, with shift applied.
        case KC_A ... KC_Z:
        case KC_MINS:
            add_weak_mods(MOD_BIT(KC_LSFT));  // Apply shift to next key.
            return true;

        // Keycodes that continue Caps Word, without shifting.
        case KC_1 ... KC_0:
        case KC_BSPC:
        case KC_DEL:
        case QK_LEAD:
        case KC_UNDS:
            return true;
        case KC_SPC:
            return leader_sequence_active();
        default:
            return false;  // Deactivate Caps Word.
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {
        case _SYMBOL:
            rgb_matrix_sethsv(HSV_MAGENTA);
            break;
        case _NAV:
            rgb_matrix_sethsv(HSV_CORAL);
            break;
        default:
            rgb_matrix_sethsv(HSV_PURPLE);
            break;
    }
    return state;
}

