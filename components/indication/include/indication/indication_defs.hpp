#pragma once

#include <stdint.h>

#define TRI_COLOR_LED_GPIO 48 // GPIO 48 for ESP32-S3 built-in addressable LED
#define LED_RMT_CHANNEL 0

//
// No Command Request or Response structure here.   All messages are currently received by task notification.
//

//
// Class Operations
//
enum LED_BITS
{
    COLORA_Bit = 0x01,
    COLORB_Bit = 0x02,
    COLORC_Bit = 0x04,
};

enum class LED_STATE : uint8_t
{
    NONE, // Zero value here helps catch an error of empty value in nvs
    OFF,
    AUTO,
    ON,
};

enum class IND_OP : uint8_t // Primary Operations
{
    Run,
    Init,
};

enum class IND_INIT : uint8_t
{
    Start,
    ColorA_On,
    ColorA_Off,
    ColorB_On,
    ColorB_Off,
    ColorC_On,
    ColorC_Off,
    Restore_Settings,
    Finished,
};

enum class IND_STATES : uint8_t
{
    Init,
    Show_FirstColor,
    FirstColor_Dark,
    Show_SecondColor,
    SecondColor_Dark,
    Final
};
