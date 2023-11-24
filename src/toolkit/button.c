/* ========================================================================= */
/**
 * @file button.c
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

#include "button.h"

#include "gfxbuf.h"

#include <libbase/libbase.h>

#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_scene.h>
#undef WLR_USE_UNSTABLE

/* == Declarations ========================================================= */

static bool _wlmtk_button_element_pointer_motion(
    wlmtk_element_t *element_ptr,
    double x, double y,
    uint32_t time_msec);
static bool _wlmtk_button_element_pointer_button(
    wlmtk_element_t *element_ptr,
    const wlmtk_button_event_t *button_event_ptr);
static void _wlmtk_button_element_pointer_leave(
    wlmtk_element_t *element_ptr);

static void apply_state(wlmtk_button_t *button_ptr);

/* == Data ================================================================= */

/** Virtual method table for the button's element super class. */
static const wlmtk_element_vmt_t button_element_vmt = {
    .pointer_motion = _wlmtk_button_element_pointer_motion,
    .pointer_button = _wlmtk_button_element_pointer_button,
    .pointer_leave = _wlmtk_button_element_pointer_leave,
};

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool wlmtk_button_init(
    wlmtk_button_t *button_ptr,
    const wlmtk_button_impl_t *button_impl_ptr)
{
    BS_ASSERT(NULL != button_ptr);
    memset(button_ptr, 0, sizeof(wlmtk_button_t));
    BS_ASSERT(NULL != button_impl_ptr);
    memcpy(&button_ptr->impl, button_impl_ptr, sizeof(wlmtk_button_impl_t));

    if (!wlmtk_buffer_init(&button_ptr->super_buffer)) {
        wlmtk_button_fini(button_ptr);
        return false;
    }
    button_ptr->orig_super_element_vmt = wlmtk_element_extend(
        &button_ptr->super_buffer.super_element,
        &button_element_vmt);

    return true;
}

/* ------------------------------------------------------------------------- */
void wlmtk_button_fini(wlmtk_button_t *button_ptr)
{
    if (NULL != button_ptr->pressed_wlr_buffer_ptr) {
        wlr_buffer_unlock(button_ptr->pressed_wlr_buffer_ptr);
        button_ptr->pressed_wlr_buffer_ptr = NULL;
    }
    if (NULL != button_ptr->released_wlr_buffer_ptr) {
        wlr_buffer_unlock(button_ptr->released_wlr_buffer_ptr);
        button_ptr->released_wlr_buffer_ptr = NULL;
    }

    wlmtk_buffer_fini(&button_ptr->super_buffer);
}

/* ------------------------------------------------------------------------- */
void wlmtk_button_set(
    wlmtk_button_t *button_ptr,
    struct wlr_buffer *released_wlr_buffer_ptr,
    struct wlr_buffer *pressed_wlr_buffer_ptr )
{
    if (NULL == released_wlr_buffer_ptr) {
        BS_ASSERT(NULL == pressed_wlr_buffer_ptr);
    } else {
        BS_ASSERT(released_wlr_buffer_ptr->width ==
                  pressed_wlr_buffer_ptr->width);
        BS_ASSERT(released_wlr_buffer_ptr->height ==
                  pressed_wlr_buffer_ptr->height);
    }

    if (NULL != button_ptr->released_wlr_buffer_ptr) {
        wlr_buffer_unlock(button_ptr->released_wlr_buffer_ptr);
    }
    button_ptr->released_wlr_buffer_ptr = wlr_buffer_lock(
        released_wlr_buffer_ptr);

    if (NULL != button_ptr->pressed_wlr_buffer_ptr) {
        wlr_buffer_unlock(button_ptr->pressed_wlr_buffer_ptr);
    }
    button_ptr->pressed_wlr_buffer_ptr = wlr_buffer_lock(
        pressed_wlr_buffer_ptr);

    apply_state(button_ptr);
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** See @ref wlmtk_element_vmt_t::pointer_motion. */
bool _wlmtk_button_element_pointer_motion(
    wlmtk_element_t *element_ptr,
    double x, double y,
    uint32_t time_msec)
{
    wlmtk_button_t *button_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_button_t, super_buffer.super_element);

    button_ptr->orig_super_element_vmt.pointer_motion(
        element_ptr, x, y, time_msec);
    button_ptr->pointer_inside = true;
    apply_state(button_ptr);
    return true;
}

/* ------------------------------------------------------------------------- */
/** See @ref wlmtk_element_vmt_t::pointer_button. */
bool _wlmtk_button_element_pointer_button(
    wlmtk_element_t *element_ptr,
    const wlmtk_button_event_t *button_event_ptr)
{
    wlmtk_button_t *button_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_button_t, super_buffer.super_element);

    if (button_event_ptr->button != BTN_LEFT) return false;

    switch (button_event_ptr->type) {
    case WLMTK_BUTTON_DOWN:
        button_ptr->pressed = true;
        apply_state(button_ptr);
        break;

    case WLMTK_BUTTON_UP:
        button_ptr->pressed = false;
        apply_state(button_ptr);
        break;

    case WLMTK_BUTTON_CLICK:
        if (NULL != button_ptr->impl.clicked) {
            button_ptr->impl.clicked(button_ptr);
        }
        break;

    default:
        break;
    }

    return true;
}

/* ------------------------------------------------------------------------- */
/** See @ref wlmtk_element_vmt_t::pointer_leave. */
void _wlmtk_button_element_pointer_leave(
    wlmtk_element_t *element_ptr)
{
    wlmtk_button_t *button_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_button_t, super_buffer.super_element);

    button_ptr->orig_super_element_vmt.pointer_leave(element_ptr);
    button_ptr->pointer_inside = false;
    apply_state(button_ptr);
}

/* ------------------------------------------------------------------------- */
/** Sets the appropriate texture for the button. */
void apply_state(wlmtk_button_t *button_ptr)
{
    if (button_ptr->pointer_inside && button_ptr->pressed) {
        wlmtk_buffer_set(
            &button_ptr->super_buffer,
            button_ptr->pressed_wlr_buffer_ptr);
    } else {
        wlmtk_buffer_set(
            &button_ptr->super_buffer,
            button_ptr->released_wlr_buffer_ptr);
    }
}

/* == Unit tests =========================================================== */

static void test_create_destroy(bs_test_t *test_ptr);
static void test_press_release(bs_test_t *test_ptr);
static void test_press_release_outside(bs_test_t *test_ptr);
static void test_press_right(bs_test_t *test_ptr);

/** Test case definition. */
const bs_test_case_t wlmtk_button_test_cases[] = {
    { 1, "create_destroy", test_create_destroy },
    { 1, "press_release", test_press_release },
    { 1, "press_release_outside", test_press_release_outside },
    { 1, "press_right", test_press_right },
    { 0, NULL, NULL }
};

/** Test outcome: Whether 'clicked' was called. */
static bool fake_button_got_clicked = false;

/** Fake destructor. */
static void fake_button_destroy(__UNUSED__ wlmtk_button_t *button_ptr) {}
/** Fake 'clicked' handler. */
static void fake_button_clicked(__UNUSED__ wlmtk_button_t *button_ptr) {
    fake_button_got_clicked = true;
}

/** Virtual method table of fake button. */
const wlmtk_button_impl_t fake_button_impl = {
    .destroy = fake_button_destroy,
    .clicked = fake_button_clicked,
};

/* ------------------------------------------------------------------------- */
/** Exercises @ref wlmtk_button_init and @ref wlmtk_button_fini. */
void test_create_destroy(bs_test_t *test_ptr)
{
    wlmtk_button_t button;

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_button_init(&button,  &fake_button_impl));
    wlmtk_button_fini(&button);
}

/* ------------------------------------------------------------------------- */
/** Tests button pressing & releasing. */
void test_press_release(bs_test_t *test_ptr)
{
    wlmtk_button_t button;
    BS_ASSERT(wlmtk_button_init(&button,  &fake_button_impl));

    struct wlr_buffer *p_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    struct wlr_buffer *r_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    wlmtk_button_set(&button, r_ptr, p_ptr);

    wlmtk_button_event_t event = { .button = BTN_LEFT, .time_msec = 42 };
    wlmtk_element_t *element_ptr = &button.super_buffer.super_element;
    fake_button_got_clicked = false;

    // Initial state: released.
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_motion(element_ptr, 0, 0, 41));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    // Button down: pressed.
    event.type = WLMTK_BUTTON_DOWN;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, p_ptr);

    // Pointer leaves the area: released.
    wlmtk_element_pointer_leave(element_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    // Pointer re-enters the area: pressed.
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_motion(element_ptr, 0, 0, 41));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, p_ptr);

    // Button up: released.
    event.type = WLMTK_BUTTON_UP;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    event.type = WLMTK_BUTTON_CLICK;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, fake_button_got_clicked);

    wlr_buffer_drop(r_ptr);
    wlr_buffer_drop(p_ptr);
    wlmtk_button_fini(&button);
}

/* ------------------------------------------------------------------------- */
/** Tests button when releasing outside the pointer focus. */
void test_press_release_outside(bs_test_t *test_ptr)
{
    wlmtk_button_t button;
    BS_ASSERT(wlmtk_button_init(&button,  &fake_button_impl));

    struct wlr_buffer *p_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    struct wlr_buffer *r_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    wlmtk_button_set(&button, r_ptr, p_ptr);

    wlmtk_button_event_t event = { .button = BTN_LEFT, .time_msec = 42 };
    wlmtk_element_t *element_ptr = &button.super_buffer.super_element;

    // Enter the ara. Released.
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_motion(element_ptr, 0, 0, 41));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    // Button down: pressed.
    event.type = WLMTK_BUTTON_DOWN;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, p_ptr);

    // Pointer leaves the area: released.
    wlmtk_element_pointer_leave(element_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    // Button up, outside the area. Then, re-enter: Still released.
    event.type = WLMTK_BUTTON_UP;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_motion(element_ptr, 0, 0, 41));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    wlr_buffer_drop(r_ptr);
    wlr_buffer_drop(p_ptr);
    wlmtk_button_fini(&button);
}

/* ------------------------------------------------------------------------- */
/** Tests button when releasing outside the pointer focus. */
void test_press_right(bs_test_t *test_ptr)
{
    wlmtk_button_t button;
    BS_ASSERT(wlmtk_button_init(&button,  &fake_button_impl));

    struct wlr_buffer *p_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    struct wlr_buffer *r_ptr = bs_gfxbuf_create_wlr_buffer(1, 1);
    wlmtk_button_set(&button, r_ptr, p_ptr);

    wlmtk_button_event_t event = {
        .button = BTN_RIGHT, .type = WLMTK_BUTTON_DOWN, .time_msec = 42,
    };
    wlmtk_element_t *element_ptr = &button.super_buffer.super_element;

    // Enter the ara. Released.
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        wlmtk_element_pointer_motion(element_ptr, 0, 0, 41));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    // Right button down: Not claimed, and remains released.
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        wlmtk_element_pointer_button(element_ptr, &event));
    BS_TEST_VERIFY_EQ(test_ptr, button.super_buffer.wlr_buffer_ptr, r_ptr);

    wlr_buffer_drop(r_ptr);
    wlr_buffer_drop(p_ptr);
    wlmtk_button_fini(&button);
}

/* == End of button.c ====================================================== */
