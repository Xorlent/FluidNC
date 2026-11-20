#pragma once

#include <cstdint>
#include <system_error>

// USB mass-storage host backend for the logical /sd volume.
// Only built where MAX_N_USB_MSC is 1.
bool usb_msc_init();

// Mounts the current USB drive at /sd. Returns an error if no drive is present
// or the filesystem cannot be mounted. max_files is the VFS open-file limit.
std::error_code usb_msc_mount(uint32_t max_files = 2);

void usb_msc_unmount();
