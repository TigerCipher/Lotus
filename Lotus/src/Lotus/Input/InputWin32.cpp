// ------------------------------------------------------------------------------
//
// Lotus
// Copyright 2024 Matthew Rogers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// File Name: InputWin32.cpp
// Date File Created: 09/21/2024
// Author: Matt
//
// ------------------------------------------------------------------------------
#ifdef _WIN64

    #include "InputWin32.h"
    #include "Input.h"

namespace lotus::input
{
namespace
{

constexpr u32 vk_mapping[256] = {
    /* 0x00 */ invalid_id_u32,
    /* 0x01 */ input_code::mouse_left,
    /* 0x02 */ input_code::mouse_right,
    /* 0x03 */ invalid_id_u32,
    /* 0x04 */ input_code::mouse_middle,
    /* 0x05 */ invalid_id_u32,
    /* 0x06 */ invalid_id_u32,
    /* 0x07 */ invalid_id_u32,
    /* 0x08 */ input_code::key_backspace,
    /* 0x09 */ input_code::key_tab,
    /* 0x0A */ invalid_id_u32,
    /* 0x0B */ invalid_id_u32,
    /* 0x0C */ invalid_id_u32,
    /* 0x0D */ input_code::key_return,
    /* 0x0E */ invalid_id_u32,
    /* 0x0F */ invalid_id_u32,
    /* 0x10 */ input_code::key_shift,
    /* 0x11 */ input_code::key_control,
    /* 0x12 */ input_code::key_alt,
    /* 0x13 */ input_code::key_pause,
    /* 0x14 */ input_code::key_capslock,
    /* 0x15 */ invalid_id_u32,
    /* 0x16 */ invalid_id_u32,
    /* 0x17 */ invalid_id_u32,
    /* 0x18 */ invalid_id_u32,
    /* 0x19 */ invalid_id_u32,
    /* 0x1A */ invalid_id_u32,
    /* 0x1B */ input_code::key_escape,
    /* 0x1C */ invalid_id_u32,
    /* 0x1D */ invalid_id_u32,
    /* 0x1E */ invalid_id_u32,
    /* 0x1F */ invalid_id_u32,
    /* 0x20 */ input_code::key_space,
    /* 0x21 */ input_code::key_page_up,
    /* 0x22 */ input_code::key_page_down,
    /* 0x23 */ input_code::key_end,
    /* 0x24 */ input_code::key_home,
    /* 0x25 */ input_code::key_left,
    /* 0x26 */ input_code::key_up,
    /* 0x27 */ input_code::key_right,
    /* 0x28 */ input_code::key_down,
    /* 0x29 */ invalid_id_u32,
    /* 0x2A */ invalid_id_u32,
    /* 0x2B */ invalid_id_u32,
    /* 0x2C */ input_code::key_print_screen,
    /* 0x2D */ input_code::key_insert,
    /* 0x2E */ input_code::key_delete,
    /* 0x2F */ invalid_id_u32,
    /* 0x30 */ input_code::key_0,
    /* 0x31 */ input_code::key_1,
    /* 0x32 */ input_code::key_2,
    /* 0x33 */ input_code::key_3,
    /* 0x34 */ input_code::key_4,
    /* 0x35 */ input_code::key_5,
    /* 0x36 */ input_code::key_6,
    /* 0x37 */ input_code::key_7,
    /* 0x38 */ input_code::key_8,
    /* 0x39 */ input_code::key_9,
    /* 0x3A */ invalid_id_u32,
    /* 0x3B */ invalid_id_u32,
    /* 0x3C */ invalid_id_u32,
    /* 0x3D */ invalid_id_u32,
    /* 0x3E */ invalid_id_u32,
    /* 0x3F */ invalid_id_u32,
    /* 0x40 */ invalid_id_u32,
    /* 0x41 */ input_code::key_a,
    /* 0x42 */ input_code::key_b,
    /* 0x43 */ input_code::key_c,
    /* 0x44 */ input_code::key_d,
    /* 0x45 */ input_code::key_e,
    /* 0x46 */ input_code::key_f,
    /* 0x47 */ input_code::key_g,
    /* 0x48 */ input_code::key_h,
    /* 0x49 */ input_code::key_i,
    /* 0x4A */ input_code::key_j,
    /* 0x4B */ input_code::key_k,
    /* 0x4C */ input_code::key_l,
    /* 0x4D */ input_code::key_m,
    /* 0x4E */ input_code::key_n,
    /* 0x4F */ input_code::key_o,
    /* 0x50 */ input_code::key_p,
    /* 0x51 */ input_code::key_q,
    /* 0x52 */ input_code::key_r,
    /* 0x53 */ input_code::key_s,
    /* 0x54 */ input_code::key_t,
    /* 0x55 */ input_code::key_u,
    /* 0x56 */ input_code::key_v,
    /* 0x57 */ input_code::key_w,
    /* 0x58 */ input_code::key_x,
    /* 0x59 */ input_code::key_y,
    /* 0x5A */ input_code::key_z,
    /* 0x5B */ invalid_id_u32,
    /* 0x5C */ invalid_id_u32,
    /* 0x5D */ invalid_id_u32,
    /* 0x5E */ invalid_id_u32,
    /* 0x5F */ invalid_id_u32,
    /* 0x60 */ input_code::key_numpad_0,
    /* 0x61 */ input_code::key_numpad_1,
    /* 0x62 */ input_code::key_numpad_2,
    /* 0x63 */ input_code::key_numpad_3,
    /* 0x64 */ input_code::key_numpad_4,
    /* 0x65 */ input_code::key_numpad_5,
    /* 0x66 */ input_code::key_numpad_6,
    /* 0x67 */ input_code::key_numpad_7,
    /* 0x68 */ input_code::key_numpad_8,
    /* 0x69 */ input_code::key_numpad_9,
    /* 0x6A */ input_code::key_multiply,
    /* 0x6B */ input_code::key_add,
    /* 0x6C */ invalid_id_u32,
    /* 0x6D */ input_code::key_subtract,
    /* 0x6E */ input_code::key_decimal,
    /* 0x6F */ input_code::key_divide,
    /* 0x70 */ input_code::key_f1,
    /* 0x71 */ input_code::key_f2,
    /* 0x72 */ input_code::key_f3,
    /* 0x73 */ input_code::key_f4,
    /* 0x74 */ input_code::key_f5,
    /* 0x75 */ input_code::key_f6,
    /* 0x76 */ input_code::key_f7,
    /* 0x77 */ input_code::key_f8,
    /* 0x78 */ input_code::key_f9,
    /* 0x79 */ input_code::key_f10,
    /* 0x7A */ input_code::key_f11,
    /* 0x7B */ input_code::key_f12,
    /* 0x7C */ invalid_id_u32,
    /* 0x7D */ invalid_id_u32,
    /* 0x7E */ invalid_id_u32,
    /* 0x7F */ invalid_id_u32,
    /* 0x80 */ invalid_id_u32,
    /* 0x81 */ invalid_id_u32,
    /* 0x82 */ invalid_id_u32,
    /* 0x83 */ invalid_id_u32,
    /* 0x84 */ invalid_id_u32,
    /* 0x85 */ invalid_id_u32,
    /* 0x86 */ invalid_id_u32,
    /* 0x87 */ invalid_id_u32,
    /* 0x88 */ invalid_id_u32,
    /* 0x89 */ invalid_id_u32,
    /* 0x8A */ invalid_id_u32,
    /* 0x8B */ invalid_id_u32,
    /* 0x8C */ invalid_id_u32,
    /* 0x8D */ invalid_id_u32,
    /* 0x8E */ invalid_id_u32,
    /* 0x8F */ invalid_id_u32,
    /* 0x90 */ input_code::key_numlock,
    /* 0x91 */ input_code::key_scrollock,
    /* 0x92 */ invalid_id_u32,
    /* 0x93 */ invalid_id_u32,
    /* 0x94 */ invalid_id_u32,
    /* 0x95 */ invalid_id_u32,
    /* 0x96 */ invalid_id_u32,
    /* 0x97 */ invalid_id_u32,
    /* 0x98 */ invalid_id_u32,
    /* 0x99 */ invalid_id_u32,
    /* 0x9A */ invalid_id_u32,
    /* 0x9B */ invalid_id_u32,
    /* 0x9C */ invalid_id_u32,
    /* 0x9D */ invalid_id_u32,
    /* 0x9E */ invalid_id_u32,
    /* 0x9F */ invalid_id_u32,
    /* 0xA0 */ invalid_id_u32,
    /* 0xA1 */ invalid_id_u32,
    /* 0xA2 */ invalid_id_u32,
    /* 0xA3 */ invalid_id_u32,
    /* 0xA4 */ invalid_id_u32,
    /* 0xA5 */ invalid_id_u32,
    /* 0xA6 */ invalid_id_u32,
    /* 0xA7 */ invalid_id_u32,
    /* 0xA8 */ invalid_id_u32,
    /* 0xA9 */ invalid_id_u32,
    /* 0xAA */ invalid_id_u32,
    /* 0xAB */ invalid_id_u32,
    /* 0xAC */ invalid_id_u32,
    /* 0xAD */ invalid_id_u32,
    /* 0xAE */ invalid_id_u32,
    /* 0xAF */ invalid_id_u32,
    /* 0xB0 */ invalid_id_u32,
    /* 0xB1 */ invalid_id_u32,
    /* 0xB2 */ invalid_id_u32,
    /* 0xB3 */ invalid_id_u32,
    /* 0xB4 */ invalid_id_u32,
    /* 0xB5 */ invalid_id_u32,
    /* 0xB6 */ invalid_id_u32,
    /* 0xB7 */ invalid_id_u32,
    /* 0xB8 */ invalid_id_u32,
    /* 0xB9 */ invalid_id_u32,
    /* 0xBA */ invalid_id_u32,
    /* 0xBB */ invalid_id_u32,
    /* 0xBC */ invalid_id_u32,
    /* 0xBD */ invalid_id_u32,
    /* 0xBE */ invalid_id_u32,
    /* 0xBF */ invalid_id_u32,
    /* 0xC0 */ invalid_id_u32,
    /* 0xC1 */ invalid_id_u32,
    /* 0xC2 */ invalid_id_u32,
    /* 0xC3 */ invalid_id_u32,
    /* 0xC4 */ invalid_id_u32,
    /* 0xC5 */ invalid_id_u32,
    /* 0xC6 */ invalid_id_u32,
    /* 0xC7 */ invalid_id_u32,
    /* 0xC8 */ invalid_id_u32,
    /* 0xC9 */ invalid_id_u32,
    /* 0xCA */ invalid_id_u32,
    /* 0xCB */ invalid_id_u32,
    /* 0xCC */ invalid_id_u32,
    /* 0xCD */ invalid_id_u32,
    /* 0xCE */ invalid_id_u32,
    /* 0xCF */ invalid_id_u32,
    /* 0xD0 */ invalid_id_u32,
    /* 0xD1 */ invalid_id_u32,
    /* 0xD2 */ invalid_id_u32,
    /* 0xD3 */ invalid_id_u32,
    /* 0xD4 */ invalid_id_u32,
    /* 0xD5 */ invalid_id_u32,
    /* 0xD6 */ invalid_id_u32,
    /* 0xD7 */ invalid_id_u32,
    /* 0xD8 */ invalid_id_u32,
    /* 0xD9 */ invalid_id_u32,
    /* 0xDA */ invalid_id_u32,
    /* 0xDB */ invalid_id_u32,
    /* 0xDC */ invalid_id_u32,
    /* 0xDD */ invalid_id_u32,
    /* 0xDE */ invalid_id_u32,
    /* 0xDF */ invalid_id_u32,
    /* 0xE0 */ invalid_id_u32,
    /* 0xE1 */ invalid_id_u32,
    /* 0xE2 */ invalid_id_u32,
    /* 0xE3 */ invalid_id_u32,
    /* 0xE4 */ invalid_id_u32,
    /* 0xE5 */ invalid_id_u32,
    /* 0xE6 */ invalid_id_u32,
    /* 0xE7 */ invalid_id_u32,
    /* 0xE8 */ invalid_id_u32,
    /* 0xE9 */ invalid_id_u32,
    /* 0xEA */ invalid_id_u32,
    /* 0xEB */ invalid_id_u32,
    /* 0xEC */ invalid_id_u32,
    /* 0xED */ invalid_id_u32,
    /* 0xEE */ invalid_id_u32,
    /* 0xEF */ invalid_id_u32,
    /* 0xF0 */ invalid_id_u32,
    /* 0xF1 */ invalid_id_u32,
    /* 0xF2 */ invalid_id_u32,
    /* 0xF3 */ invalid_id_u32,
    /* 0xF4 */ invalid_id_u32,
    /* 0xF5 */ invalid_id_u32,
    /* 0xF6 */ invalid_id_u32,
    /* 0xF7 */ invalid_id_u32,
    /* 0xF8 */ invalid_id_u32,
    /* 0xF9 */ invalid_id_u32,
    /* 0xFA */ invalid_id_u32,
    /* 0xFB */ invalid_id_u32,
    /* 0xFC */ invalid_id_u32,
    /* 0xFD */ invalid_id_u32,
    /* 0xFE */ invalid_id_u32,
    /* 0xFF */ invalid_id_u32,
};

struct modifier_flags
{
    enum flags : u8
    {
        left_shift    = 0x10,
        left_control  = 0x20,
        left_alt      = 0x40,
        right_shift   = 0x01,
        right_control = 0x02,
        right_alt     = 0x04,
    };
};

u8 modifier_keys_state{};

void set_modifier_input(u8 virtual_key, input_code::code code, modifier_flags::flags flags)
{
    if (GetKeyState(virtual_key) < 0)
    {
        set(input_source::keyboard, code, { 1.0f, 0.0f, 0.0f });
        modifier_keys_state |= flags;
    } else if (modifier_keys_state & flags)
    {
        set(input_source::keyboard, code, { 0.0f, 0.0f, 0.0f });
        modifier_keys_state &= ~flags;
    }
}

void set_modifier_inputs(input_code::code code)
{
    if (code == input_code::key_shift)
    {
        set_modifier_input(VK_LSHIFT, input_code::key_left_shift, modifier_flags::left_shift);
        set_modifier_input(VK_RSHIFT, input_code::key_right_shift, modifier_flags::right_shift);
    } else if (code == input_code::key_control)
    {
        set_modifier_input(VK_LCONTROL, input_code::key_left_control, modifier_flags::left_control);
        set_modifier_input(VK_RCONTROL, input_code::key_right_control, modifier_flags::right_control);
    } else if (code == input_code::key_alt)
    {
        set_modifier_input(VK_LMENU, input_code::key_left_alt, modifier_flags::left_alt);
        set_modifier_input(VK_RMENU, input_code::key_right_alt, modifier_flags::right_alt);
    }
}

constexpr vec2 get_mouse_position(LPARAM lparam)
{
    return { (f32) ((i16) (lparam & 0x0000ffff)), (f32) ((i16) (lparam >> 16)) };
}

} // anonymous namespace

HRESULT process_input_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        assert(wparam <= 0xff);
        const input_code::code code{ vk_mapping[wparam & 0xff] };
        if (code != invalid_id_u32)
        {
            set(input_source::keyboard, code, { 1.f, 0.f, 0.f });
            set_modifier_inputs(code);
        }
    }
    break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        assert(wparam <= 0xff);
        const input_code::code code{ vk_mapping[wparam & 0xff] };
        if (code != invalid_id_u32)
        {
            set(input_source::keyboard, code, { 0.f, 0.f, 0.f });
            set_modifier_inputs(code);
        }
    }
    break;
    case WM_MOUSEMOVE:
    {
        const vec2 pos = get_mouse_position(lparam);
        set(input_source::mouse, input_code::mouse_position_x, { pos.x, 0.f, 0.f });
        set(input_source::mouse, input_code::mouse_position_y, { pos.y, 0.f, 0.f });
        set(input_source::mouse, input_code::mouse_position, { pos.x, pos.y, 0.f });
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        SetCapture(hwnd);
        const input_code::code code = msg == WM_LBUTTONDOWN ? input_code::mouse_left
                                    : msg == WM_RBUTTONDOWN ? input_code::mouse_right
                                                            : input_code::mouse_middle;
        const vec2             pos  = get_mouse_position(lparam);
        set(input_source::mouse, code, { pos.x, pos.y, 1.f });
    }
    break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        ReleaseCapture();
        const input_code::code code = msg == WM_LBUTTONUP ? input_code::mouse_left
                                    : msg == WM_RBUTTONUP ? input_code::mouse_right
                                                          : input_code::mouse_middle;
        const vec2             pos  = get_mouse_position(lparam);
        set(input_source::mouse, code, { pos.x, pos.y, 0.f });
    }
    break;
    case WM_MOUSEHWHEEL:
    {
        set(input_source::mouse, input_code::mouse_wheel, { (f32) (GET_WHEEL_DELTA_WPARAM(wparam)), 0.f, 0.f });
    }
    break;
    }
    return S_OK;
}

} // namespace lotus::input

#endif