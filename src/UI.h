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
//    if (unicode == '-' && edit->cursor == 0)
//        return 1;
    return 0;
}

static void string_edit(nk_context *ctx, std::string &value, size_t max_len)
{
    char buffer[1024];
    int len;
    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    len = strlen(buffer);
    if (nk_edit_string(ctx,
                       NK_EDIT_SIMPLE,
                       buffer,
                       &len,
                       max_len,
                       nk_filter_default)) {
        value.assign(buffer, len);
    }
}

template <typename T>
static void uint_edit(nk_context *ctx, T &value)
{
    char buffer[6];
    int len;
    snprintf(buffer, sizeof(buffer), "%d", value);
    len = strlen(buffer);
    if (nk_edit_string(ctx,
                       NK_EDIT_SIMPLE,
                       buffer,
                       &len,
                       sizeof(buffer),
                       unsigned_int_filter)) {
        unsigned int newValue = std::strtoul(buffer, nullptr, 10);
        if (newValue <= std::numeric_limits<T>::max()) {
            value = static_cast<T>(newValue);
        }
    }
}

static void byte_slider(nk_context *ctx, const char* name, uint8_t& value ) {
    int intValue = static_cast<int>(value);
    nk_property_int(ctx, name, 0, &intValue, 255, 1, 1);
    value = static_cast<uint8_t>(intValue);
}

#endif //DLTROPHY_QMAND_UI_H
