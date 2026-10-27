/* PicoDualProbe CMSIS-DAP vendor transport for minichlink. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "minichlink.h"

typedef struct {
    struct ProgrammerStructBase base;
    libusb_context *usb;
    libusb_device_handle *handle;
} zero_ctx;

static int command(zero_ctx *ctx, uint8_t op, uint8_t reg, uint32_t *value) {
    uint8_t request[7] = {0x80, op, reg, 0, 0, 0, 0}, response[64];
    int sent = 0, received = 0;
    int length = op == 3 ? 7 : op == 2 ? 3 : 2;
    if (op == 3) for (int i = 0; i < 4; i++) request[3+i] = *value >> (8*i);
    int rc = libusb_bulk_transfer(ctx->handle, 0x04, request, length, &sent, 1000);
    if (rc || sent != length) return -1;
    rc = libusb_bulk_transfer(ctx->handle, 0x85, response, sizeof(response), &received, 1000);
    if (rc || received != (op == 2 ? 6 : 2) || response[0] != 0x80 || response[1]) return -1;
    if (op == 2) *value = (uint32_t)response[2] | (uint32_t)response[3]<<8 | (uint32_t)response[4]<<16 | (uint32_t)response[5]<<24;
    return 0;
}
static int read_reg(void *dev, uint8_t reg, uint32_t *value) { return command(dev, 2, reg, value); }
static int write_reg(void *dev, uint8_t reg, uint32_t value) { return command(dev, 3, reg, &value); }
static int flush(void *dev) { (void)dev; return 0; }
static int cleanup(void *dev) {
    zero_ctx *ctx = dev;
    command(ctx, 1, 0, NULL);
    libusb_release_interface(ctx->handle, 0);
    libusb_close(ctx->handle);
    libusb_exit(ctx->usb);
    free(ctx->base.internal);
    free(ctx);
    return 0;
}
void *TryInit_Zero(const init_hints_t *hints) {
    zero_ctx *ctx = calloc(1, sizeof(*ctx));
    libusb_device **devices = NULL;
    int matches = 0;
    if (!ctx || libusb_init(&ctx->usb) != 0) { free(ctx); return NULL; }
    ssize_t count = libusb_get_device_list(ctx->usb, &devices);
    for (ssize_t i = 0; i < count; i++) {
        struct libusb_device_descriptor d;
        libusb_device_handle *h = NULL;
        unsigned char serial[256];
        if (libusb_get_device_descriptor(devices[i], &d) || d.idVendor != 0x2e8a || d.idProduct != 0x000c || libusb_open(devices[i], &h)) continue;
        int n = libusb_get_string_descriptor_ascii(h, d.iSerialNumber, serial, sizeof(serial));
        if (n < 0 || (hints->programmer_serial_number && strcmp((char *)serial, hints->programmer_serial_number))) { libusb_close(h); continue; }
        matches++;
        if (ctx->handle) libusb_close(h); else ctx->handle = h;
    }
    if (devices) libusb_free_device_list(devices, 1);
    if (matches != 1 || libusb_claim_interface(ctx->handle, 0)) {
        fprintf(stderr, "Select exactly one accessible PicoDualProbe Zero using -l SERIAL.\n");
        if (ctx->handle) libusb_close(ctx->handle);
        libusb_exit(ctx->usb); free(ctx); return NULL;
    }
    /* A previous client may have exited with a reply still queued. Wait for
       the endpoint to go quiet before matching replies to new commands. */
    unsigned char stale[64];
    int received, drained = 0, rc;
    do {
        rc = libusb_bulk_transfer(ctx->handle, 0x85, stale, sizeof(stale), &received, 200);
        if (rc == 0 && ++drained > 32) break;
    } while (rc == 0);
    if (rc != LIBUSB_ERROR_TIMEOUT) {
        fprintf(stderr, "Could not drain stale CMSIS-DAP replies.\n");
        libusb_release_interface(ctx->handle, 0); libusb_close(ctx->handle);
        libusb_exit(ctx->usb); free(ctx); return NULL;
    }
    int start_op = hints->specific_programmer && !strcmp(hints->specific_programmer, "zero203") ? 4 : 0;
    if (command(ctx, start_op, 0, NULL)) {
        fprintf(stderr, "Unable to enter CH32 debug mode; disconnect any CMSIS-DAP session.\n");
        libusb_release_interface(ctx->handle, 0); libusb_close(ctx->handle);
        libusb_exit(ctx->usb); free(ctx); return NULL;
    }
    MCF.ReadReg32 = read_reg; MCF.WriteReg32 = write_reg;
    MCF.FlushLLCommands = flush; MCF.Exit = cleanup;
    return ctx;
}
