/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * USB transfer transaction structures (implementation).
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str_error.h>
#include <usb/debug.h>

#include "endpoint.h"
#include "bus.h"

#include "usb_transfer_batch.h"

/** Create a batch on given endpoint.
 */
usb_transfer_batch_t *usb_transfer_batch_create(endpoint_t *ep)
{
	assert(ep);

	bus_t *bus = endpoint_get_bus(ep);
	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, batch_create);

	if (!ops) {
		usb_transfer_batch_t *batch = calloc(1, sizeof(usb_transfer_batch_t));
		usb_transfer_batch_init(batch, ep);
		return batch;
	}

	return ops->batch_create(ep);
}

/** Initialize given batch structure.
 */
void usb_transfer_batch_init(usb_transfer_batch_t *batch, endpoint_t *ep)
{
	assert(ep);
	endpoint_add_ref(ep);
	batch->ep = ep;
}

/** Resolve resetting toggle.
 *
 * @param[in] batch Batch structure to use.
 */
int usb_transfer_batch_reset_toggle(usb_transfer_batch_t *batch)
{
	assert(batch);

	if (batch->error != EOK || batch->toggle_reset_mode == RESET_NONE)
		return EOK;

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " resets %s",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch),
	    batch->toggle_reset_mode == RESET_ALL ? "all EPs toggle" : "EP toggle");

	return bus_reset_toggle(endpoint_get_bus(batch->ep), batch->target, batch->toggle_reset_mode);
}

/** Destroy the batch.
 *
 * @param[in] batch Batch structure to use.
 */
void usb_transfer_batch_destroy(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	bus_t *bus = endpoint_get_bus(batch->ep);
	const bus_ops_t *ops = BUS_OPS_LOOKUP(bus->ops, batch_destroy);

	endpoint_del_ref(batch->ep);

	if (ops) {
		usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " destroying.\n",
		    batch, USB_TRANSFER_BATCH_ARGS(*batch));
		ops->batch_destroy(batch);
	}
	else {
		usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " disposing.\n",
		    batch, USB_TRANSFER_BATCH_ARGS(*batch));
		free(batch);
	}
}

/** Finish a transfer batch: call handler, destroy batch, release endpoint.
 *
 * Call only after the batch have been scheduled && completed!
 *
 * @param[in] batch Batch structure to use.
 */
void usb_transfer_batch_finish(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	usb_log_debug2("Batch %p " USB_TRANSFER_BATCH_FMT " finishing.\n",
	    batch, USB_TRANSFER_BATCH_ARGS(*batch));

	if (batch->on_complete) {
		const int err = batch->on_complete(batch->on_complete_data, batch->error, batch->transfered_size);
		if (err)
			usb_log_warning("batch %p failed to complete: %s",
			    batch, str_error(err));
	}

	usb_transfer_batch_destroy(batch);
}

/** Finish a transfer batch as an aborted one.
 *
 * @param[in] batch Batch structure to use.
 */
void usb_transfer_batch_abort(usb_transfer_batch_t *batch)
{
	assert(batch);
	assert(batch->ep);

	batch->error = EAGAIN;
	usb_transfer_batch_finish(batch);
}

/**
 * @}
 */
