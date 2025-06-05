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
#include "config.h"
#include "info_config.h"
#include "keyboard.h"
#include "keycodes.h"
#include "keymap_us.h"
#include "modifiers.h"
#include "pointing_device.h"
#include "process_caps_word.h"
#include "process_key_override.h"
#include "process_tap_dance.h"
#include "quantum.h"
#include "quantum_keycodes.h"
#include "rgb_matrix.h"
#include "send_string_keycodes.h"
#include "timer.h"
#include QMK_KEYBOARD_H
#include <cyboard.h>

enum layer_names {
    _ENGRAM,
    _NUM,
    _SYM,
    _NAV
};

enum led_states {
    LAYER_ENGRAM,
    LAYER_NUM,
    LAYER_SYM,
    LAYER_NAV,
    ACTION_CAPS_WORD,
    ACTION_NUM_WORD,
};

void set_led_colors(enum led_states led_state) {
    switch (led_state) {
        case LAYER_ENGRAM:
            rgb_matrix_sethsv(HSV_PURPLE);
            rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
            return;
        case LAYER_NUM:
            rgb_matrix_sethsv(HSV_MAGENTA);
            rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
            return;
        case LAYER_SYM:
            rgb_matrix_sethsv(HSV_CYAN);
            rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
            return;
        case LAYER_NAV:
            rgb_matrix_sethsv(HSV_CORAL);
            rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
            return;
        case ACTION_CAPS_WORD:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_UP_DOWN);
            return;
        case ACTION_NUM_WORD:
            rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
            return;
        default:
            rgb_matrix_sethsv(HSV_PURPLE);
            rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
            return;
    }
}

const uint16_t num_layer_timeout = 5000;
static uint16_t num_layer_idle_timer = 0;
static bool num_layer_word_active = false;

void num_layer_word_on(void) {
    if (num_layer_word_active) {
        return;
    }
    num_layer_word_active = true;
    num_layer_idle_timer = timer_read() + num_layer_timeout;
    layer_on(_NUM);
    set_led_colors(ACTION_NUM_WORD);
}

void num_layer_word_off(void) {
    if (!num_layer_word_active) {
        return;
    }

    layer_off(_NUM);
    num_layer_word_active = false;
    set_led_colors(get_highest_layer(layer_state));
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
        num_layer_word_off();
    }
}

void housekeeping_task_user(void) {
    num_layer_idle_task();
}

enum custom_keycodes {
    // screen shot
    UK_SCSH = SAFE_RANGE,
    // layer clear - used for getting back to the base layer
    UK_LRCL,
    UK_HIDE,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_num_layer_word(keycode, record)) {
        return false;
    }

    switch (keycode) {
        case UK_SCSH:
            SEND_STRING(SS_LGUI(SS_LSFT("4")));
            layer_on(_NAV);
            return false;
        case UK_LRCL:
            if (get_highest_layer(layer_state) > _ENGRAM) {
                layer_clear();
                return false;
            }

            break;
        case UK_HIDE:
            SEND_STRING(SS_LGUI("h"));
            break;
    }

    return true;
}

void dance_one_shot_num_word(tap_dance_state_t *state, void *user_data) {
    switch (state->count) {
        case 1:
            set_oneshot_layer(_NUM, ONESHOT_START);
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

const key_override_t left_curly_brace_override = ko_make_basic(MOD_MASK_SHIFT, KC_LCBR, KC_RCBR);
const key_override_t left_bracket_override = ko_make_basic(MOD_MASK_SHIFT, KC_LBRC, KC_RBRC);
const key_override_t left_paren_override = ko_make_basic(MOD_MASK_SHIFT, KC_LPRN, KC_RPRN);
const key_override_t volup_next_track_override = ko_make_basic(MOD_MASK_SHIFT, KC_VOLU, KC_MNXT);
const key_override_t voldown_prev_track_override = ko_make_basic(MOD_MASK_SHIFT, KC_VOLD, KC_MPRV);

const key_override_t *key_overrides[] = {
    &left_curly_brace_override,
    &left_bracket_override,
    &left_paren_override,
    &volup_next_track_override,
    &voldown_prev_track_override
};

enum tap_dances {
    TD_OSL_NUM_WORD
};

tap_dance_action_t tap_dance_actions[] = {
    [TD_OSL_NUM_WORD] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_one_shot_num_word, dance_one_shot_num_word_reset),
};

#define UK_RST RIGHT_SNIPING_MODE_TOGGLE
#define UK_RDM RIGHT_DRAGSCROLL_MODE
#define UK_SRC LSFT(MS_BTN2)
#define UK_TDNW TD(TD_OSL_NUM_WORD)
#define OSM_HYPR OSM(MOD_HYPR)
#define OSM_MEH OSM(MOD_MEH)
#define OSM_LSFT OSM(MOD_LSFT)
#define OSM_ACTL OSM(MOD_LCTL | MOD_LALT)
#define TT_NAV TT(_NAV)
#define HM_LCTC LCTL_T(KC_C)
#define HM_LALI LALT_T(KC_I)
#define HM_HYPE HYPR_T(KC_E)
#define HM_LGUA LGUI_T(KC_A)
#define HM_RGUH RGUI_T(KC_H)
#define HM_HYPT HYPR_T(KC_T)
#define HM_RALS RALT_T(KC_S)
#define HM_RCTN RCTL_T(KC_N)
#define UK_COPY LGUI(KC_C)
#define UK_PSTE RGUI(KC_V)
#define UK_LOCK LGUI(LCTL(KC_Q))
#define UK_AICH HYPR(KC_TILD)
#define UK_TERM HYPR(KC_1)
#define UK_BRSR HYPR(KC_2)
#define UK_AERO HYPR(KC_0)
#define UK_SYMB OSL(_SYM)


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_ENGRAM] = LAYOUT_let(
        KC_TAB,   KC_B,     KC_Y,     KC_O,      KC_U,     KC_Z,                           KC_Q,      KC_L,     KC_D,     KC_W,     KC_V,     KC_SCLN,
        KC_ESC,   HM_LCTC,  HM_LALI,  HM_HYPE,   HM_LGUA,  KC_COMM,                        KC_DOT,    HM_RGUH,  HM_HYPT,  HM_RALS,  HM_RCTN,  KC_QUOT,
        OSM_MEH,  KC_G,     KC_X,     KC_J,      KC_K,     KC_LPRN,                        KC_LCBR,   KC_R,     KC_M,     KC_F,     KC_P,     KC_SLSH,
                            KC_LEFT,  KC_RIGHT,  QK_LEAD,  UK_SYMB,  UK_BRSR,   TT(_NAV),  UK_SCSH,   KC_ENT,   KC_UP,    KC_DOWN,
                                                 KC_BSPC,  UK_TDNW,  KC_LBRC,   UK_AICH,   OSM_LSFT,  KC_SPC
    ),

    [_NUM] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, KC_PLUS, KC_MINS, KC_SLSH, KC_ASTR, KC_BSLS,
        UK_LRCL, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                              KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    _______,
        _______, _______, _______, _______, _______, _______,                           _______, KC_COMM, KC_DOT,  _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),

    [_SYM] = LAYOUT_let(
        _______, _______, _______, _______, _______, _______,                           _______, _______, _______, _______, _______, KC_BSLS,
        UK_LRCL, KC_GRV,  KC_TILD, KC_EQL,  KC_AMPR, _______,                           _______, KC_PIPE, KC_PLUS, _______, KC_ASTR, _______,
        _______, _______, _______, _______, _______, _______,                           _______, KC_DOT,  KC_SLSH, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______, _______, _______,
                                            _______, _______, _______,         _______, _______, KC_MINS
    ),

    [_NAV] = LAYOUT_let(
        _______, KC_VOLU, KC_VOLD, KC_MPLY, KC_MUTE, _______,                           UK_HIDE, UK_RDM,  _______, _______, _______, _______,
        UK_LRCL, MS_BTN1, MS_BTN2, KC_LSFT, KC_LGUI, _______,                           MS_BTN1, MS_BTN2, _______, _______, _______, _______,
        _______, UK_RST,  _______, _______, _______, _______,                           _______, _______, _______, _______, _______, _______,
                          _______, _______, _______, _______, _______,         _______, _______, _______,  _______, _______,
                                            _______, _______, _______,         _______, _______, _______
    ),
};

void leader_end_user(void) {
    if (leader_sequence_one_key(KC_SPC)) {
        tap_code16(KC_UNDS);
    } else if (leader_sequence_one_key(KC_DOT)) {
        SEND_STRING("->");
    } else if (leader_sequence_one_key(KC_SLSH)) {
        SEND_STRING("./");
    } else if (leader_sequence_one_key(KC_A)) {
        SEND_STRING("&&");
    } else if (leader_sequence_one_key(KC_H)) {
        SEND_STRING("||");
    } else if (leader_sequence_one_key(KC_E)) {
        SEND_STRING("==");
    } else if (leader_sequence_four_keys(KC_B, KC_O, KC_O, KC_T)) {
        reset_keyboard();
    }
}

// returning true from this function will end the leader sequence
// immediately without waiting to timeout
// this means these sequences in this function will immediately resolve
// instead of waiting for the delay
bool leader_add_user(uint16_t keycode) {
    return leader_sequence_one_key(KC_SPC) ||
           leader_sequence_one_key(KC_DOT) ||
           leader_sequence_one_key(KC_A)   ||
           leader_sequence_one_key(KC_H)   ||
           leader_sequence_one_key(KC_E)   ||
           leader_sequence_one_key(KC_SLSH);
}

void pointing_device_init_user(void) {
    charybdis_set_pointer_dragscroll_enabled(true, true);
}

void oneshot_mods_changed_user(uint8_t mods) {
    if (mods & MOD_MASK_SHIFT) {
        set_led_colors(ACTION_CAPS_WORD);
    } else {
        set_led_colors(get_highest_layer(layer_state));
    }
}

void caps_word_set_user(bool active) {
    if (active) {
        set_led_colors(ACTION_CAPS_WORD);
    } else {
        set_led_colors(get_highest_layer(layer_state));
    }
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
    set_led_colors(get_highest_layer(state));
    return state;
}
