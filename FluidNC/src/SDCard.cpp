// Copyright (c) 2018 -	Bart Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#include "Config.h"
#if MAX_N_SDCARD

#    include "SDCard.h"
#    include "Machine/MachineConfig.h"
#    include "Channel.h"
#    include "Report.h"
#    include "Assertion.h"

#    include "Driver/sdspi.h"
#    if MAX_N_USB_MSC
#        include "Driver/usb_msc.h"
#    endif
#    include "SettingsDefinitions.h"
#    include "FluidPath.h"

const EnumItem SDCard::types[] = {
    { SDCard::SDSPI, "sdspi" },
#if MAX_N_USB_MSC
    { SDCard::USB_MSC, "usb_msc" },
#endif
    EnumItem(SDCard::SDSPI),
};

SDCard::SDCard() : _state(State::Idle) {}

void SDCard::init() {
    if (isUSB()) {
#if MAX_N_USB_MSC
        config_ok = usb_msc_init();
#else
        log_error("USB MSC storage is not supported on this platform");
#endif
        return;
    }

    static bool init_message = true;  // used to show messages only once.
    pinnum_t    csPin;
    pinnum_t    csFallback;

    if (_cs.defined()) {
        if (!config->_spi || !config->_spi->defined()) {
            log_error("SD needs SPI defined");
            return;
        }
        if (init_message) {
            log_info("SD Card cs_pin:" << _cs.name() << " detect:" << _cardDetect.name() << " freq:" << _frequency_hz);
            init_message = false;
        }
        _cs.setAttr(Pin::Attr::Output);
        csPin = _cs.getNative(Pin::Capabilities::Output | Pin::Capabilities::Native);
    } else if ((csFallback = sd_fallback_cs->get()) != INVALID_PINNUM) {
        if (!config->_spi || !config->_spi->defined()) {
            log_error("SD needs SPI defined");
            return;
        }
        csPin = csFallback;
        log_info("Using fallback CS pin " << int(csPin));
    } else {
        log_debug("See http://wiki.fluidnc.com/en/config/sd_card#sdfallbackcs-access-sd-without-a-config-file");
        return;
    }

    config_ok = true;

    if (_cardDetect.defined()) {
        _cardDetect.setAttr(Pin::Attr::Input);
        auto cdPin = _cardDetect.getNative(Pin::Capabilities::Input | Pin::Capabilities::Native);
        sd_init_slot(_frequency_hz, csPin, cdPin);
    } else {
        sd_init_slot(_frequency_hz, csPin);
    }
}

std::error_code SDCard::mount(uint32_t max_files) {
    if (isUSB()) {
#if MAX_N_USB_MSC
        return usb_msc_mount(max_files);
#else
        return std::make_error_code(std::errc::function_not_supported);
#endif
    }
    return sd_mount(max_files);
}

void SDCard::unmount() {
    if (isUSB()) {
#if MAX_N_USB_MSC
        usb_msc_unmount();
#endif
        return;
    }
    sd_unmount();
}

void SDCard::afterParse() {
#if !MAX_N_USB_MSC
    if (isUSB()) {
        throw AssertionFailed::create("sdcard type usb_msc requires ESP32-P4");
    }
#endif
}

SDCard::~SDCard() {}
#endif
