// Copyright (c) 2026 - FluidNC contributors
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Config.h"
#if MAX_N_USB_MSC

#    include "Driver/usb_msc.h"
#    include "Driver/watchdog.h"
#    include "Logging.h"

#    include <Arduino.h>
#    include <USBHost.h>
#    include <USBHostMSC.h>
#    include <USBMSCFS.h>

namespace {
    constexpr const char* mount_point = "/sd";
    constexpr uint32_t    mount_wait_ms = 1000;

    USBMSCFSClass usb_fs;
    bool          fs_mounted = false;
}

bool usb_msc_init() {
    if (!USBHost.begin()) {
        log_error("USB MSC: host initialization failed");
        return false;
    }
    log_info("USB MSC: host initialized");
    return true;
}

std::error_code usb_msc_mount(uint32_t max_files) {
    // The drive may have been removed while this volume was still mounted.
    // Drop the stale VFS/FatFs state before probing again.
    if (fs_mounted && !USBHostMSC.mounted()) {
        usb_fs.end();
        fs_mounted = false;
    }
    if (fs_mounted) {
        return {};
    }

    bool mounted_ok = false;
    {
        // Enumeration and FAT probing can both block on a slow or failing
        // drive. Do not let either one trip the task watchdog.
        WatchdogSuspend wdt_off;
        const uint32_t start = millis();
        while (!USBHostMSC.mounted() && (millis() - start) < mount_wait_ms) {
            delay(5);
        }
        if (USBHostMSC.mounted()) {
            mounted_ok = usb_fs.begin(mount_point, max_files, false);
        }
    }

    if (!USBHostMSC.mounted()) {
        return std::make_error_code(std::errc::no_such_device);
    }
    if (!mounted_ok) {
        return std::make_error_code(std::errc::io_error);
    }

    fs_mounted = true;
    log_verbose("USB MSC: mounted " << USBHostMSC.blockCount() << " x " << USBHostMSC.blockSize() << " byte blocks at " << mount_point);
    return {};
}

void usb_msc_unmount() {
    if (fs_mounted) {
        usb_fs.end();
        fs_mounted = false;
    }
}

#endif
