/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define VIAL_KEYBOARD_UID {0xC4, 0xCC, 0xB4, 0x76, 0x30, 0x16, 0xFC, 0x70}

#define VIAL_UNLOCK_COMBO_ROWS { 10, 10 }
#define VIAL_UNLOCK_COMBO_COLS { 5, 6 }

#define DYNAMIC_KEYMAP_LAYER_COUNT 10

// -----------------------------------------------------------------------------
// sunaku
// -----------------------------------------------------------------------------

// https://github.com/qmk/qmk_firmware/issues/18511#issuecomment-1264604610
// https://github.com/qmk/qmk_firmware/issues/8990#issuecomment-851055637
#undef SPLIT_USB_DETECT

