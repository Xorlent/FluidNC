// Copyright (c) 2018 -	Bart Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#pragma once

#include "Configuration/Configurable.h"
#include "EnumItem.h"
#include "WebUI/Authentication.h"
#include "Pin.h"
#include "Error.h"

#include <cstdint>
#include <system_error>

class SDCard : public Configuration::Configurable {
public:
    enum class State : uint8_t {
        Idle          = 0,
        NotPresent    = 1,
        Busy          = 2,
        BusyUploading = 3,
        BusyParsing   = 4,
        BusyWriting   = 5,
        BusyReading   = 6,
    };

    // Numeric values are stable because they are part of the config format.
    enum Type : uint32_t {
        SDSPI   = 0,
        USB_MSC = 1,
    };
    static const EnumItem types[];

private:
    State _state;
    Pin   _cardDetect;
    Pin   _cs;

    uint32_t _frequency_hz = 8000000;  // Set to nonzero to override the default
    uint32_t _type         = SDSPI;

public:
    SDCard();
    SDCard(const SDCard&)            = delete;
    SDCard& operator=(const SDCard&) = delete;

    void afterParse() override;

    const char* filename();
    bool        config_ok = false;

    // Initializes the selected backend.
    void init();

    // Mount/unmount the logical /sd volume. Mount is lazy, like the SD card.
    std::error_code mount(uint32_t max_files = 2);
    void            unmount();

    bool isUSB() const { return _type == USB_MSC; }

    // Configuration handlers.
    void group(Configuration::HandlerBase& handler) override {
        // @config type
        // @default sdspi
        // @ignore_drift YAML token is sdspi; the C++ initializer is the SDSPI enum constant.
        // Storage backend for the logical /sd volume. sdspi uses an SPI SD
        // card; usb_msc uses a FAT32/FAT16 USB flash drive on supported
        // ESP32-P4 boards. The two backends are mutually exclusive; cs_pin,
        // card_detect_pin and frequency_hz apply only to sdspi.
        handler.item("type", _type, types);

        // @config cs_pin
        // @default NO_PIN
        // @pin_attributes output
        // SPI chip-select pin for the SD card. Must be a native MCU pin with output
        // capability. Required (non-NO_PIN) for the SD card to function -- an spi: section
        // must also be configured.
        handler.item("cs_pin", _cs);

        // @config card_detect_pin
        // @default NO_PIN
        // @pin_attributes input
        // Optional card-detect switch input. Purely informational -- shown in the startup
        // log, with no other feature attached to it.
        handler.item("card_detect_pin", _cardDetect);

        // @config frequency_hz
        // @default 8000000
        // @tuning typical
        // SPI clock speed used for the SD card. Try a lower value if the card has
        // consistent read/write problems.
        handler.item("frequency_hz", _frequency_hz, 400000, 20000000);
    }

    ~SDCard();
};
