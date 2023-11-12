/* ========================================================================= */
/**
 * @file titlebar_title.c
 *
 * @copyright
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "titlebar_title.h"

#include "buffer.h"
#include "gfxbuf.h"
#include "primitives.h"

#define WLR_USE_UNSTABLE
#include <wlr/interfaces/wlr_buffer.h>
#undef WLR_USE_UNSTABLE

/* == Declarations ========================================================= */

/** State of the title bar's title. */
struct _wlmtk_titlebar_title_t {
    /** Superclass: Buffer. */
    wlmtk_buffer_t            super_buffer;

    /** The drawn title, when focussed. */
    struct wlr_buffer         *focussed_wlr_buffer_ptr;
    /** The drawn title, when blurred. */
    struct wlr_buffer         *blurred_wlr_buffer_ptr;
};

static void title_buffer_destroy(wlmtk_buffer_t *buffer_ptr);
static void title_set_activated(
    wlmtk_titlebar_title_t *titlebar_title_ptr,
    bool activated);
struct wlr_buffer *title_create_buffer(
    bs_gfxbuf_t *gfxbuf_ptr,
    unsigned position,
    unsigned width,
    uint32_t text_color,
    const wlmtk_titlebar_style_t *style_ptr);

/* == Data ================================================================= */

/** Buffer implementation for title of the title bar. */
static const wlmtk_buffer_impl_t title_buffer_impl = {
    .destroy = title_buffer_destroy
};

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
wlmtk_titlebar_title_t *wlmtk_titlebar_title_create(void)
{
    wlmtk_titlebar_title_t *titlebar_title_ptr = logged_calloc(
        1, sizeof(wlmtk_titlebar_title_t));
    if (NULL == titlebar_title_ptr) return NULL;

    if (!wlmtk_buffer_init(
            &titlebar_title_ptr->super_buffer,
            &title_buffer_impl)) {
        wlmtk_titlebar_title_destroy(titlebar_title_ptr);
        return NULL;
    }

    return titlebar_title_ptr;
}

/* ------------------------------------------------------------------------- */
void wlmtk_titlebar_title_destroy(wlmtk_titlebar_title_t *titlebar_title_ptr)
{
    wlr_buffer_drop_nullify(&titlebar_title_ptr->focussed_wlr_buffer_ptr);
    wlr_buffer_drop_nullify(&titlebar_title_ptr->blurred_wlr_buffer_ptr);
    wlmtk_buffer_fini(&titlebar_title_ptr->super_buffer);
    free(titlebar_title_ptr);
}

/* ------------------------------------------------------------------------- */
bool wlmtk_titlebar_title_redraw(
    wlmtk_titlebar_title_t *titlebar_title_ptr,
    bs_gfxbuf_t *focussed_gfxbuf_ptr,
    bs_gfxbuf_t *blurred_gfxbuf_ptr,
    int position,
    int width,
    bool activated,
    const wlmtk_titlebar_style_t *style_ptr)
{
    BS_ASSERT(focussed_gfxbuf_ptr->width == blurred_gfxbuf_ptr->width);
    BS_ASSERT(style_ptr->height == focussed_gfxbuf_ptr->height);
    BS_ASSERT(style_ptr->height == blurred_gfxbuf_ptr->height);
    BS_ASSERT(position <= (int)focussed_gfxbuf_ptr->width);
    BS_ASSERT(position + width <= (int)focussed_gfxbuf_ptr->width);

    struct wlr_buffer *focussed_wlr_buffer_ptr = title_create_buffer(
        focussed_gfxbuf_ptr, position, width,
        style_ptr->focussed_text_color, style_ptr);
    struct wlr_buffer *blurred_wlr_buffer_ptr = title_create_buffer(
        blurred_gfxbuf_ptr, position, width,
        style_ptr->blurred_text_color, style_ptr);

    if (NULL == focussed_wlr_buffer_ptr ||
        NULL == blurred_wlr_buffer_ptr) {
        wlr_buffer_drop_nullify(&focussed_wlr_buffer_ptr);
        wlr_buffer_drop_nullify(&blurred_wlr_buffer_ptr);
        return false;
    }

    wlr_buffer_drop_nullify(&titlebar_title_ptr->focussed_wlr_buffer_ptr);
    titlebar_title_ptr->focussed_wlr_buffer_ptr = focussed_wlr_buffer_ptr;
    wlr_buffer_drop_nullify(&titlebar_title_ptr->blurred_wlr_buffer_ptr);
    titlebar_title_ptr->blurred_wlr_buffer_ptr = blurred_wlr_buffer_ptr;

    title_set_activated(titlebar_title_ptr, activated);
    return true;
}

/* ------------------------------------------------------------------------- */
void wlmtk_titlebar_title_set_activated(
    wlmtk_titlebar_title_t *titlebar_title_ptr,
    bool activated)
{
    title_set_activated(titlebar_title_ptr, activated);
}

/* ------------------------------------------------------------------------- */
wlmtk_element_t *wlmtk_titlebar_title_element(
    wlmtk_titlebar_title_t *titlebar_title_ptr)
{
    return &titlebar_title_ptr->super_buffer.super_element;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** Dtor. Forwards to @ref wlmtk_titlebar_title_destroy. */
void title_buffer_destroy(wlmtk_buffer_t *buffer_ptr)
{
    wlmtk_titlebar_title_t *titlebar_title_ptr = BS_CONTAINER_OF(
        buffer_ptr, wlmtk_titlebar_title_t, super_buffer);
    wlmtk_titlebar_title_destroy(titlebar_title_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Sets whether the title is drawn focussed (activated) or blurred.
 *
 * @param titlebar_title_ptr
 * @param activated
 */
void title_set_activated(
    wlmtk_titlebar_title_t *titlebar_title_ptr,
    bool activated)
{
    wlmtk_buffer_set(
        &titlebar_title_ptr->super_buffer,
        activated ?
        titlebar_title_ptr->focussed_wlr_buffer_ptr :
        titlebar_title_ptr->blurred_wlr_buffer_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Creates a WLR buffer with the title's texture, as specified.
 *
 * @param gfxbuf_ptr
 * @param position
 * @param width
 * @param text_color
 * @param style_ptr
 *
 * @return A pointer to a `struct wlr_buffer` with the texture.
 */
struct wlr_buffer *title_create_buffer(
    bs_gfxbuf_t *gfxbuf_ptr,
    unsigned position,
    unsigned width,
    uint32_t text_color,
    const wlmtk_titlebar_style_t *style_ptr)
{
    struct wlr_buffer *wlr_buffer_ptr = bs_gfxbuf_create_wlr_buffer(
        width, style_ptr->height);
    if (NULL == wlr_buffer_ptr) return NULL;

    bs_gfxbuf_copy_area(
        bs_gfxbuf_from_wlr_buffer(wlr_buffer_ptr),
        0, 0,
        gfxbuf_ptr,
        position, 0,
        width, style_ptr->height);

    cairo_t *cairo_ptr = cairo_create_from_wlr_buffer(wlr_buffer_ptr);
    if (NULL == cairo_ptr) {
        wlr_buffer_drop(wlr_buffer_ptr);
        return NULL;
    }
    wlmaker_primitives_draw_bezel_at(
        cairo_ptr, 0, 0, width, style_ptr->height, 1.0, true);
    wlmaker_primitives_draw_window_title(
        cairo_ptr, "Title", text_color);
    cairo_destroy(cairo_ptr);

    return wlr_buffer_ptr;
}

/* == Unit tests =========================================================== */

static void test_title(bs_test_t *test_ptr);

const bs_test_case_t wlmtk_titlebar_title_test_cases[] = {
    { 1, "title", test_title },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
/** Tests title drawing. */
void test_title(bs_test_t *test_ptr)
{
    const wlmtk_titlebar_style_t style = {
        .focussed_text_color = 0xffc0c0c0,
        .blurred_text_color = 0xff808080,
        .height = 22,
    };
    bs_gfxbuf_t *focussed_gfxbuf_ptr = bs_gfxbuf_create(120, 22);
    bs_gfxbuf_t *blurred_gfxbuf_ptr = bs_gfxbuf_create(120, 22);
    bs_gfxbuf_clear(focussed_gfxbuf_ptr, 0xff2020c0);
    bs_gfxbuf_clear(blurred_gfxbuf_ptr, 0xff404040);

    wlmtk_titlebar_title_t *titlebar_title_ptr = wlmtk_titlebar_title_create();
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, titlebar_title_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_titlebar_title_redraw(
            titlebar_title_ptr,
            focussed_gfxbuf_ptr, blurred_gfxbuf_ptr, 10, 90, true, &style));

    BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(
        test_ptr,
        bs_gfxbuf_from_wlr_buffer(titlebar_title_ptr->focussed_wlr_buffer_ptr),
        "toolkit/title_focussed.png");
    BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(
        test_ptr,
        bs_gfxbuf_from_wlr_buffer(titlebar_title_ptr->blurred_wlr_buffer_ptr),
        "toolkit/title_blurred.png");

    // We had started as "activated", verify that's correct.
    wlmtk_buffer_t *super_buffer_ptr = &titlebar_title_ptr->super_buffer;
    BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(
        test_ptr,
        bs_gfxbuf_from_wlr_buffer(super_buffer_ptr->wlr_buffer_ptr),
        "toolkit/title_focussed.png");

    // De-activated the title. Verify that was propagated.
    title_set_activated(titlebar_title_ptr, false);
    BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(
        test_ptr,
        bs_gfxbuf_from_wlr_buffer(super_buffer_ptr->wlr_buffer_ptr),
        "toolkit/title_blurred.png");

    // Redraw with shorter width. Verify that's still correct.
    wlmtk_titlebar_title_redraw(
        titlebar_title_ptr, focussed_gfxbuf_ptr, blurred_gfxbuf_ptr,
        10, 70, false, &style);
    BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(
        test_ptr,
        bs_gfxbuf_from_wlr_buffer(super_buffer_ptr->wlr_buffer_ptr),
        "toolkit/title_blurred_short.png");

    wlmtk_element_destroy(wlmtk_titlebar_title_element(titlebar_title_ptr));
    bs_gfxbuf_destroy(focussed_gfxbuf_ptr);
    bs_gfxbuf_destroy(blurred_gfxbuf_ptr);
}

/* == End of titlebar_title.c ============================================== */