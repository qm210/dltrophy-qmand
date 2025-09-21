//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_UI_H
#define DLTROPHY_QMAND_UI_H


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

static nk_bool unsigned_int_filter(const struct nk_text_edit *edit, nk_rune unicode)
{
    if (unicode >= '0' && unicode <= '9')
        return 1;
    return 0;
}

static nk_bool signed_int_filter(const struct nk_text_edit *edit, nk_rune unicode)
{
    if (unicode == '-' && edit->cursor == 0)
        return 1;
    return unsigned_int_filter(edit, unicode);
}

static bool string_edit(nk_context *ctx, std::string &value, size_t max_len, nk_flags flags = NK_EDIT_DEFAULT)
{
    if (max_len > 1024) {
        throw std::runtime_error("string_edit() currently only has 1024 buffer, change it");
    }
    bool changed = false;
    char buffer[1024];
    int len;
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    len = strlen(buffer);
    if (nk_edit_string(ctx, NK_EDIT_FIELD | flags, buffer, &len, max_len, nk_filter_default)) {
        std::string newValue = std::string(buffer, len);
        changed = value != newValue;
        value = newValue;
    }
    return changed;
}

template <typename T>
static bool int_edit(nk_context *ctx, T &value, bool isUnsigned = true, nk_flags flags = NK_EDIT_DEFAULT)
{
    bool changed = false;
    const int MAX_LEN = 24;
    char buffer[MAX_LEN];
    snprintf(buffer, sizeof(buffer), "%d", value);
    int len = strlen(buffer);
    nk_flags result = nk_edit_string(ctx, NK_EDIT_FIELD | flags,
                                     buffer, &len, sizeof(buffer),
                                     isUnsigned ? unsigned_int_filter : signed_int_filter);
    if (!result) {
        return false;
    }
    if (len < MAX_LEN) {
        buffer[len] = '\0';
    }
    unsigned int newValue = std::strtoul(buffer, nullptr, 10);
    if (newValue <= std::numeric_limits<T>::max() && newValue != value) {
        value = static_cast<T>(newValue);
        changed = true;
    }
    return changed;
}

static int byte_slider(nk_context *ctx, uint8_t& value ) {
    int intValue = static_cast<int>(value);
    int changed = nk_slider_int(ctx, 0, &intValue, 255, 1);
    value = static_cast<uint8_t>(intValue);
    return changed;
}

static void byte_property(nk_context *ctx, const char* name, uint8_t& value ) {
    int intValue = static_cast<int>(value);
    nk_property_int(ctx, name, 0, &intValue, 255, 1, 1);
    value = static_cast<uint8_t>(intValue);
}

static bool checkbox(nk_context *ctx, const char* label, bool& value) {
    auto nk_value = static_cast<nk_bool>(value);
    bool changed = nk_checkbox_label(ctx, label, &nk_value);
    if (changed) {
        value = static_cast<bool>(nk_value);
    }
    return changed;
}

static nk_style_slider copySliderStyleAndAppearInactive(nk_context *ctx, bool isInactive) {
    auto backup = ctx->style.slider;
    if (!isInactive) {
        return backup;
    }
    const auto hide = nk_style_item_color(nk_color{});
    auto inactive_color = backup.bar_filled;
    inactive_color.a = 60;
    ctx->style.slider.bar_filled = inactive_color;
    ctx->style.slider.bar_normal = inactive_color;
    ctx->style.slider.bar_hover = inactive_color;
    ctx->style.slider.cursor_normal = hide;
    ctx->style.slider.cursor_hover = hide;
    ctx->style.slider.inc_button.normal = hide;
    ctx->style.slider.inc_button.hover = hide;
    ctx->style.slider.inc_button.active = hide;
    ctx->style.slider.inc_symbol = NK_SYMBOL_NONE;
    ctx->style.slider.dec_button.normal = hide;
    ctx->style.slider.dec_button.hover = hide;
    ctx->style.slider.dec_button.active = hide;
    ctx->style.slider.dec_symbol = NK_SYMBOL_NONE;
    return backup;
}

static bool parameterWithCheckboxRow(nk_context *ctx, float rowHeight, const char* label, bool& active, uint8_t& parameter) {
    bool changed = false;
    nk_layout_row_begin(ctx, NK_DYNAMIC, rowHeight, 3);
    nk_layout_row_push(ctx, 0.15f);
    changed |= checkbox(ctx, active ? "Apply" : "Ignore", active);
    nk_layout_row_push(ctx, 0.25f);
    nk_label(ctx, label, NK_TEXT_LEFT);
    auto sliderStyle = copySliderStyleAndAppearInactive(ctx, !active);
    nk_layout_row_push(ctx, 0.60f);
    changed |= byte_slider(ctx, parameter);
    nk_layout_row_end(ctx);
    ctx->style.slider = sliderStyle;
    return changed;
}

#endif //DLTROPHY_QMAND_UI_H
