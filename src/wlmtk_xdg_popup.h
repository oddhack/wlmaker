/* ========================================================================= */
/**
 * @file wlmtk_xdg_popup.h
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
#ifndef __WLMTK_XDG_POPUP_H__
#define __WLMTK_XDG_POPUP_H__

#include "toolkit/toolkit.h"

/** Forward declaration. */
struct wlr_xdg_popup;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Creates a popup.
 *
 * @param wlr_xdg_popup_ptr
 * @param window_ptr
 */
void wlmtk_create_popup(
    struct wlr_xdg_popup *wlr_xdg_popup_ptr,
    wlmtk_window_t *window_ptr);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __WLMTK_XDG_POPUP_H__ */
/* == End of wlmtk_xdg_popup.h ============================================= */
