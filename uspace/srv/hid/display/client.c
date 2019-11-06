/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server client
 */

#include <adt/list.h>
#include <errno.h>
#include <stdlib.h>
#include "client.h"
#include "display.h"
#include "window.h"

/** Create client.
 *
 * @param display Parent display
 * @param cb Client callbacks
 * @param cb_arg Callback argument
 * @param rclient Place to store pointer to new client.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_client_create(ds_display_t *display, ds_client_cb_t *cb,
    void *cb_arg, ds_client_t **rclient)
{
	ds_client_t *client;

	client = calloc(1, sizeof(ds_client_t));
	if (client == NULL)
		return ENOMEM;

	list_initialize(&client->windows);
	list_initialize(&client->events);
	client->cb = cb;
	client->cb_arg = cb_arg;

	ds_display_add_client(display, client);

	*rclient = client;
	return EOK;
}

/** Destroy client.
 *
 * @param client Client
 */
void ds_client_destroy(ds_client_t *client)
{
	assert(list_empty(&client->windows));
	ds_display_remove_client(client);
	free(client);
}

/** Add window to client.
 *
 * @param client client
 * @param wnd Window
 * @return EOK on success, ENOMEM if there are no free window identifiers
 */
errno_t ds_client_add_window(ds_client_t *client, ds_window_t *wnd)
{
	assert(wnd->client == NULL);
	assert(!link_used(&wnd->lwindows));

	wnd->client = client;
	wnd->id = client->display->next_wnd_id++;
	list_append(&wnd->lwindows, &client->windows);

	return EOK;
}

/** Remove window from client.
 *
 * @param wnd Window
 */
void ds_client_remove_window(ds_window_t *wnd)
{
	list_remove(&wnd->lwindows);
	wnd->client = NULL;
}

/** Find window by ID.
 *
 * @param client Client
 * @param id Window ID
 */
#include <stdio.h>
ds_window_t *ds_client_find_window(ds_client_t *client, ds_wnd_id_t id)
{
	ds_window_t *wnd;

	// TODO Make this faster
	printf("ds_client_find_window: id=0x%lx\n", id);
	wnd = ds_client_first_window(client);
	while (wnd != NULL) {
		printf("ds_client_find_window: wnd=%p wnd->id=0x%lx\n", wnd, wnd->id);
		if (wnd->id == id)
			return wnd;
		wnd = ds_client_next_window(wnd);
	}

	return NULL;
}

/** Get first window in client.
 *
 * @param client Client
 * @return First window or @c NULL if there is none
 */
ds_window_t *ds_client_first_window(ds_client_t *client)
{
	link_t *link = list_first(&client->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lwindows);
}

/** Get next window in client.
 *
 * @param wnd Current window
 * @return Next window or @c NULL if there is none
 */
ds_window_t *ds_client_next_window(ds_window_t *wnd)
{
	link_t *link = list_next(&wnd->lwindows, &wnd->client->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, lwindows);
}

/** Get next event from client event queue.
 *
 * @param client Client
 * @param ewindow Place to store pointer to window receiving the event
 * @param event Place to store event
 * @return Graphic context
 */
errno_t ds_client_get_event(ds_client_t *client, ds_window_t **ewindow,
    display_wnd_ev_t *event)
{
	link_t *link;
	ds_window_ev_t *wevent;

	link = list_first(&client->events);
	if (link == NULL)
		return ENOENT;
	wevent = list_get_instance(link, ds_window_ev_t, levents);

	*ewindow = wevent->window;
	*event = wevent->event;
	free(wevent);
	return EOK;
}

/** Post keyboard event to the client's message queue.
 *
 * @param client Client
 * @param ewindow Window that the message is targetted to
 * @param event Event
 *
 * @return EOK on success or an error code
 */
errno_t ds_client_post_kbd_event(ds_client_t *client, ds_window_t *ewindow,
    kbd_event_t *event)
{
	ds_window_ev_t *wevent;

	wevent = calloc(1, sizeof(ds_window_ev_t));
	if (wevent == NULL)
		return ENOMEM;

	wevent->window = ewindow;
	wevent->event.kbd_event = *event;
	list_append(&wevent->levents, &client->events);

	/* Notify the client */
	// TODO Do not send more than once until client drains the queue
	client->cb->ev_pending(client->cb_arg);

	return EOK;
}

/** @}
 */
