/*
 * Copyright (c) 2021 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup nav
 * @{
 */
/** @file Navigator.
 *
 * HelenOS file manager.
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "menu.h"
#include "nav.h"

static errno_t navigator_create(const char *, navigator_t **);
static void navigator_destroy(navigator_t *);

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (navigator)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	navigator_t *navigator = (navigator_t *) arg;

	ui_quit(navigator->ui);
}

/** Create navigator.
 *
 * @param display_spec Display specification
 * @param rnavigator Place to store pointer to new navigator
 * @return EOK on success or ane error code
 */
static errno_t navigator_create(const char *display_spec,
    navigator_t **rnavigator)
{
	navigator_t *navigator;
	ui_wnd_params_t params;
	errno_t rc;

	navigator = calloc(1, sizeof(navigator_t));
	if (navigator == NULL)
		return ENOMEM;

	rc = ui_create(display_spec, &navigator->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Navigator";
	params.style &= ~ui_wds_decorated;
	params.placement = ui_wnd_place_full_screen;

	rc = ui_window_create(navigator->ui, &params, &navigator->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(navigator->window, &window_cb, (void *) navigator);

	rc = ui_fixed_create(&navigator->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	ui_window_add(navigator->window, ui_fixed_ctl(navigator->fixed));

	rc = nav_menu_create(navigator, &navigator->menu);
	if (rc != EOK)
		goto error;

	rc = ui_window_paint(navigator->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	*rnavigator = navigator;
	return EOK;
error:
	navigator_destroy(navigator);
	return rc;
}

static void navigator_destroy(navigator_t *navigator)
{
	if (navigator->menu != NULL)
		nav_menu_destroy(navigator->menu);
	if (navigator->window != NULL)
		ui_window_destroy(navigator->window);
	if (navigator->ui != NULL)
		ui_destroy(navigator->ui);
	free(navigator);
}

/** Run navigator on the specified display. */
static errno_t navigator_run(const char *display_spec)
{
	navigator_t *navigator;
	errno_t rc;

	rc = navigator_create(display_spec, &navigator);
	if (rc != EOK)
		return rc;

	ui_run(navigator->ui);

	navigator_destroy(navigator);
	return EOK;
}

static void print_syntax(void)
{
	printf("Syntax: nav [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_CONSOLE_DEFAULT;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i < argc) {
		print_syntax();
		return 1;
	}

	rc = navigator_run(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
