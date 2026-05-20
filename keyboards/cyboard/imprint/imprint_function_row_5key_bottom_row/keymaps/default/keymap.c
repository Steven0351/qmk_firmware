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

enum platforms {
    PLATFORM_LINUX,
    PLATFORM_MAC,
    PLATFORM_WIN,
};

enum platforms current_platform = PLATFORM_LINUX;

enum layer_names {
    _ENGRAM,
    _GAMING,
    _SYM,
    _ACT,
    _NAV,
    _TMUX,
};

enum led_states {
    LAYER_DEFAULT,
    LAYER_SYM,
    LAYER_ACT,
    LAYER_NAV,
    ACTION_CAPS_WORD,
    ACTION_NUM_WORD,
};

void set_led_colors(enum led_states led_state) {
    switch (led_state) {
    case LAYER_DEFAULT:
        switch (current_platform) {
        case PLATFORM_MAC:
        case PLATFORM_LINUX:
            rgb_matrix_sethsv_noeeprom(HSV_PURPLE);
            break;
        case PLATFORM_WIN:
            rgb_matrix_sethsv_noeeprom(HSV_ORANGE);
            break;
        }

        rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
        return;
    case LAYER_SYM:
    case LAYER_ACT:
        rgb_matrix_sethsv_noeeprom(HSV_RED);
        rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
        return;
    case LAYER_NAV:
        rgb_matrix_sethsv_noeeprom(HSV_CORAL);
        rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
        return;
    case ACTION_CAPS_WORD:
        rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_UP_DOWN);
        return;
    case ACTION_NUM_WORD:
        rgb_matrix_mode_noeeprom(RGB_MATRIX_GRADIENT_LEFT_RIGHT);
        return;
    default:
        rgb_matrix_sethsv_noeeprom(HSV_GREEN);
        rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
        return;
    }
}

void toggle_platform(void) {
    switch (current_platform) {
    case PLATFORM_LINUX:
        current_platform = PLATFORM_WIN;
        break;
    case PLATFORM_WIN:
        current_platform = PLATFORM_MAC;
        break;
    case PLATFORM_MAC:
        current_platform = PLATFORM_LINUX;
        break;
    }

    set_led_colors(LAYER_DEFAULT);
}

enum custom_keycodes {
    // screen shot
    UK_SCS = SAFE_RANGE,
    // layer clear - used for getting back to the base layer
    UK_LRCL,
    UK_HIDE,
    UK_TTY,
    UK_TOG,
    UK_RARR,
    UK_DSLS,
    UK_LAND,
    UK_LORE,
    UK_LEQ,
    UK_MOD,
    UK_LINX,
    UK_MAC,
    UK_WIN
};

bool is_tmux(void) {
    return get_highest_layer(layer_state) == _TMUX;
}

const uint16_t num_layer_timeout = 5000;
uint16_t num_layer_idle_timer = 0;
bool num_layer_word_active = false;

void num_layer_word_on(void) {
    if (num_layer_word_active) {
        return;
    }
    num_layer_word_active = true;
    num_layer_idle_timer = timer_read() + num_layer_timeout;
    layer_on(_SYM);
    set_led_colors(ACTION_NUM_WORD);
}

void num_layer_word_off(void) {
    if (!num_layer_word_active) {
        return;
    }

    layer_off(_SYM);
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
        num_layer_word_off();
    }
}

void housekeeping_task_user(void) {
    num_layer_idle_task();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_num_layer_word(keycode, record)) return false;
    
    // Just apply the tmux prefixes we use for the platform before processing
    if (is_tmux()) {
        switch (current_platform) {
        case PLATFORM_LINUX:
        case PLATFORM_MAC:
            register_code(KC_LALT);
            register_code(KC_LSFT);
            register_code(KC_LCTL);
            register_code(KC_A);

            unregister_code(KC_LALT);
            unregister_code(KC_LSFT);
            unregister_code(KC_LCTL);
            unregister_code(KC_A);
            break;
        case PLATFORM_WIN:
            register_code(KC_LALT);
            register_code(KC_COMM);

            unregister_code(KC_LALT);
            unregister_code(KC_COMM);
            break;
        }
    }

    if (record->event.pressed) {
        switch (keycode) {
        case UK_TTY:
            break;

        case UK_TOG:
            break;

        case UK_RARR:
            if (record->event.pressed) {
                send_string("->");
            }
            return false;

        case UK_DSLS:
            SEND_STRING("./");
            return false;

        case UK_LAND:
            SEND_STRING("&&");
            return false;

        case UK_LORE:
            SEND_STRING("||");
            return false;
        case UK_LEQ:
            SEND_STRING("==");
            return false;

        case UK_MOD:
            SEND_STRING("::");
            return false;

        case UK_LINX:
            current_platform = PLATFORM_LINUX;
            set_led_colors(LAYER_DEFAULT);
            return false;

        case UK_MAC:
            current_platform = PLATFORM_MAC;
            set_led_colors(LAYER_DEFAULT);
            return false;

        case UK_WIN:
            current_platform = PLATFORM_WIN;
            set_led_colors(LAYER_DEFAULT);
            return false;

        case UK_SCS:
            switch (current_platform) {
            case PLATFORM_MAC:
                SEND_STRING(SS_LGUI(SS_LSFT("4")));
                break;
            case PLATFORM_LINUX:
                SEND_STRING(SS_LGUI(SS_LSFT("s")));
                break;
            case PLATFORM_WIN:
                SEND_STRING(SS_LGUI(SS_LSFT("s")));
                break;
            }

            layer_on(_NAV);
            return false;
 
        case UK_LRCL:
            if (get_highest_layer(layer_state) > _ENGRAM) {
                layer_clear();
                return false;
            }

            break;
 
        case UK_HIDE:
            switch (current_platform) {
            case PLATFORM_LINUX:
                SEND_STRING(SS_LGUI("q"));
                break;
            case PLATFORM_MAC:
                SEND_STRING(SS_LGUI("h"));
                break;
            case PLATFORM_WIN:
                SEND_STRING(SS_LGUI("d"));
                break;
            }

            return false;
        }
    }

    return true;
}

void pointing_device_init_user(void) {
    charybdis_set_pointer_dragscroll_enabled(true, true);
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
    default:
        return false;  // Deactivate Caps Word.
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    set_led_colors(get_highest_layer(state));
    return state;
}

void keyboard_post_init_user(void) {
    set_led_colors(LAYER_DEFAULT);
}

void dance_one_shot_num_word(tap_dance_state_t *state, void *user_data) {
    switch (state->count) {
        case 1:
            set_oneshot_layer(_SYM, ONESHOT_START);
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
    TD_OSL_NUM_WORD,
};

tap_dance_action_t tap_dance_actions[] = {
    [TD_OSL_NUM_WORD] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, dance_one_shot_num_word, dance_one_shot_num_word_reset),
};


// INFO: aliases to make the keymap not ugly

#define UK_RST RIGHT_SNIPING_MODE_TOGGLE
#define UK_RDM RIGHT_DRAGSCROLL_MODE
#define UK_SRC LSFT(MS_BTN2)
#define OS_GUI OSM(MOD_GUI)
#define UK_SLS OSM(MOD_LSFT)

// homerow mods
#define HM_LCC LCTL_T(KC_C)
#define HM_LAI LALT_T(KC_I)
#define HM_HYE HYPR_T(KC_E)
#define HM_LGA LGUI_T(KC_A)
#define HM_RGH RGUI_T(KC_H)
#define HM_HYT HYPR_T(KC_T)
#define HM_RAS RALT_T(KC_S)
#define HM_RCN RCTL_T(KC_N)

#define UK_TMX TT(_TMUX)
#define UK_LBR KC_LBRC
#define UK_RBR KC_RBRC
#define UK_LCB KC_LCBR
#define UK_RCB KC_RCBR
#define UK_NAV TT(_NAV)
#define UK_SYM OSL(_SYM)
#define UK_ACT OSL(_ACT)
#define UK_LED QK_LEAD
#define UK_COM KC_COMM
#define UK_LPR KC_LPRN
#define UK_RPR KC_RPRN
#define UK_BSP KC_BSPC
#define ______ _______
#define UK_MIN KC_MINS
#define UK_LCL KC_LCTL
#define UK_LAL KC_LALT
#define UK_GAM DF(_GAMING)
#define UK_ENG DF(_ENGRAM)
#define UK_RGT KC_RIGHT
#define UK_LFT KC_LEFT
#define UK_TDN TD(TD_OSL_NUM_WORD)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [_ENGRAM] = LAYOUT_fun_full_bottom_row(
        ______,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                                         KC_6,    KC_VOLU, KC_VOLD, KC_MUTE, KC_0,    KC_MINS,
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                                        KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,
        KC_TAB,  KC_B,    KC_Y,    KC_O,    KC_U,    KC_Z,                                         KC_Q,    KC_L,    KC_D,    KC_W,    KC_V,    KC_SCLN,
        KC_ESC,  HM_LCC,  HM_LAI,  HM_HYE,  HM_LGA,  UK_COM,                                       KC_DOT,  HM_RGH,  HM_HYT,  HM_RAS,  HM_RCN,  KC_QUOT,
        UK_TMX,  KC_G,    KC_X,    KC_J,    KC_K,    UK_LPR,                                       UK_RPR,  KC_R,    KC_M,    KC_F,    KC_P,    KC_SLSH,
        ______,  UK_LFT,  UK_LBR,  UK_RBR,  UK_RGT,   UK_ACT, UK_SYM, UK_GAM,   UK_NAV, UK_SLS, KC_ENT,   KC_UP,   UK_LCB,  UK_RCB,  KC_DOWN,  ______,
                                                      UK_BSP, UK_TDN, UK_TOG,   UK_SCS, UK_SLS, KC_SPC
    ),

    [_GAMING] = LAYOUT_fun_full_bottom_row(
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,    ______, ______, UK_ENG,   ______, ______, ______,    ______,  ______,  ______,  ______,  ______,
                                                       ______, ______, ______,   ______, ______, ______
    ),

    [_SYM] = LAYOUT_fun_full_bottom_row(
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                                         KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSLS,
        UK_LRCL, KC_TILD, KC_GRV,  KC_EQL,  ______,  ______,                                       ______,  KC_PIPE, KC_PLUS, KC_UNDS, KC_DQUO, ______,
        ______,  KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,                                      KC_CIRC, KC_AMPR, KC_ASTR, KC_SLSH, KC_QUES, ______,
        ______,  ______,  ______,  ______,  ______,    ______, ______, ______,   ______, ______, ______,    ______,  ______,  ______,  ______,  ______,
                                                       ______, ______, ______,   ______, ______, UK_MIN
    ),

    [_ACT] = LAYOUT_fun_full_bottom_row(
        ______,  ______,  ______,  ______,  ______,  ______,                                       UK_LINX, UK_MAC,  UK_WIN,  ______,  ______,  QK_BOOT,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  UK_MOD,
        UK_LRCL, ______,  ______,  UK_LEQ,  ______,  ______,                                       UK_RARR, UK_LORE, ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  UK_LAND, ______,  ______,  ______,  UK_DSLS,
        ______,  ______,  ______,  ______,  ______,    ______, ______, ______,   ______, ______, ______,    ______,  ______,  ______,  ______,  ______,
                                                       ______, ______, ______,   ______, ______, KC_UNDS
    ),

    [_NAV] = LAYOUT_fun_full_bottom_row(
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       UK_HIDE, UK_RDM,  ______,  ______,  ______,  ______,
        UK_LRCL, ______,  ______,  ______,  ______,  ______,                                       MS_BTN1, MS_BTN2, ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,    ______, ______, ______,   ______, ______, ______,    ______,  ______,  ______,  ______,  ______,
                                                       ______, ______, ______,   ______, ______, ______
    ),

    [_TMUX] = LAYOUT_fun_full_bottom_row(
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        UK_LRCL, ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,  ______,                                       ______,  ______,  ______,  ______,  ______,  ______,
        ______,  ______,  ______,  ______,  ______,    ______, ______, ______,   ______, ______, ______,    ______,  ______,  ______,  ______,  ______,
                                                       ______, ______, ______,   ______, ______, ______
    ),
};
