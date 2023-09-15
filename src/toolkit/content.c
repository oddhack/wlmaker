/* ========================================================================= */
/**
 * @file content.c
 * Copyright (c) 2023 by Philipp Kaeser <kaeser@gubbe.ch>
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

#include "content.h"

#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_scene.h>
#undef WLR_USE_UNSTABLE

/* == Declarations ========================================================= */

static void element_destroy(wlmtk_element_t *element_ptr);
static struct wlr_scene_node *element_create_scene_node(
    wlmtk_element_t *element_ptr,
    struct wlr_scene_tree *wlr_scene_tree_ptr);
static void element_get_dimensions(
    wlmtk_element_t *element_ptr,
    int *left_ptr,
    int *top_ptr,
    int *right_ptr,
    int *bottom_ptr);

/** Method table for the container's virtual methods. */
const wlmtk_element_impl_t  super_element_impl = {
    .destroy = element_destroy,
    .create_scene_node = element_create_scene_node,
    .get_dimensions = element_get_dimensions,
};

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool wlmtk_content_init(
    wlmtk_content_t *content_ptr,
    const wlmtk_content_impl_t *content_impl_ptr)
{
    BS_ASSERT(NULL != content_ptr);
    BS_ASSERT(NULL != content_impl_ptr);
    BS_ASSERT(NULL != content_impl_ptr->destroy);
    BS_ASSERT(NULL != content_impl_ptr->create_scene_node);
    BS_ASSERT(NULL != content_impl_ptr->get_size);

    memset(content_ptr, 0, sizeof(wlmtk_content_t));

    if (!wlmtk_element_init(&content_ptr->super_element,
                            &super_element_impl)) {
        return false;
    }

    content_ptr->impl_ptr = content_impl_ptr;
    return true;
}

/* ------------------------------------------------------------------------- */
void wlmtk_content_fini(wlmtk_content_t *content_ptr)
{
    wlmtk_element_fini(&content_ptr->super_element);
    content_ptr->impl_ptr = NULL;
}

/* ------------------------------------------------------------------------- */
void wlmtk_content_set_window(
    wlmtk_content_t *content_ptr,
    wlmtk_window_t *window_ptr)
{
    content_ptr->window_ptr = window_ptr;
}

/* ------------------------------------------------------------------------- */
wlmtk_element_t *wlmtk_content_element(wlmtk_content_t *content_ptr)
{
    return &content_ptr->super_element;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Implementation of the superclass wlmtk_element_t::destroy method.
 *
 * Forwards the call to the wlmtk_content_t::destroy method.
 *
 * @param element_ptr
 */
void element_destroy(wlmtk_element_t *element_ptr)
{
    wlmtk_content_t *content_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_content_t, super_element);
    content_ptr->impl_ptr->destroy(content_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Implementation of the superclass wlmtk_element_t::create_scene_node method.
 *
 * Forwards the call to the wlmtk_content_t::create_scene_node method.
 *
 * @param element_ptr
 * @param wlr_scene_tree_ptr
 */
struct wlr_scene_node *element_create_scene_node(
    wlmtk_element_t *element_ptr,
    struct wlr_scene_tree *wlr_scene_tree_ptr)
{
    wlmtk_content_t *content_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_content_t, super_element);
    return content_ptr->impl_ptr->create_scene_node(
        content_ptr, wlr_scene_tree_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Implementation of the element's get_dimensions method: Return dimensions.
 *
 * @param element_ptr
 * @param left_ptr            Leftmost position. May be NULL.
 * @param top_ptr             Topmost position. May be NULL.
 * @param right_ptr           Rightmost position. Ma be NULL.
 * @param bottom_ptr          Bottommost position. May be NULL.
 */
void element_get_dimensions(
    wlmtk_element_t *element_ptr,
    int *left_ptr,
    int *top_ptr,
    int *right_ptr,
    int *bottom_ptr)
{
    if (NULL != left_ptr) *left_ptr = 0;
    if (NULL != top_ptr) *top_ptr = 0;

    wlmtk_content_t *content_ptr = BS_CONTAINER_OF(
        element_ptr, wlmtk_content_t, super_element);
    content_ptr->impl_ptr->get_size(content_ptr, right_ptr, bottom_ptr);
}

/* == Unit tests =========================================================== */

static void test_init_fini(bs_test_t *test_ptr);

const bs_test_case_t wlmtk_content_test_cases[] = {
    { 1, "init_fini", test_init_fini },
    { 0, NULL, NULL }
};

/** dtor for the content under test. */
static void test_destroy_cb(wlmtk_content_t *content_ptr)
{
    wlmtk_content_fini(content_ptr);
}
/** Creates a scene node attached to the tree. */
static struct wlr_scene_node *test_create_scene_node_cb(
    __UNUSED__ wlmtk_content_t *content_ptr,
    struct wlr_scene_tree *wlr_scene_tree_ptr)
{
    struct wlr_scene_buffer *wlr_scene_buffer_ptr = wlr_scene_buffer_create(
        wlr_scene_tree_ptr, NULL);
    return &wlr_scene_buffer_ptr->node;
}
/** returns a fake size. */
static void test_get_size_cb(
    __UNUSED__ wlmtk_content_t *content_ptr,
    int *width_ptr, int *height_ptr)
{
    if (NULL != width_ptr) *width_ptr = 42;
    if (NULL != height_ptr) *height_ptr = 21;
}

/** Method table for the content we're using for test. */
static const wlmtk_content_impl_t test_content_impl = {
    .destroy = test_destroy_cb,
    .create_scene_node = test_create_scene_node_cb,
    .get_size = test_get_size_cb,
};

/* ------------------------------------------------------------------------- */
/** Exercises init() and fini() methods, verifies dtor forwarding. */
void test_init_fini(bs_test_t *test_ptr)
{
    wlmtk_content_t content;
    BS_TEST_VERIFY_TRUE(test_ptr, wlmtk_content_init(
                            &content, &test_content_impl));
    // Also expect the super element to be initialized.
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, content.super_element.impl_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, content.impl_ptr);

    int l, t, r, b;
    wlmtk_element_get_dimensions(&content.super_element, &l, &t, &r, &b);
    BS_TEST_VERIFY_EQ(test_ptr, 0, l);
    BS_TEST_VERIFY_EQ(test_ptr, 0, t);
    BS_TEST_VERIFY_EQ(test_ptr, 42, r);
    BS_TEST_VERIFY_EQ(test_ptr, 21, b);

    content.impl_ptr->destroy(&content);

    // Also expect the super element to be un-initialized.
    BS_TEST_VERIFY_EQ(test_ptr, NULL, content.super_element.impl_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, content.impl_ptr);

}

/* == End of content.c ================================================== */
