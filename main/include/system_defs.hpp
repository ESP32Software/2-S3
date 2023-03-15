#pragma once

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 2
#define APP_VERSION_REVISION 3

#define SWITCH_1 GPIO_NUM_0 // Boot Switch -- GPIO_EN.  This a strapping pin is pulled-up by default

#include "freertos/FreeRTOS.h" // RTOS Libraries
#include "freertos/queue.h"
//
// Run - This is our primary loop where we service periodic tasks.  We handle SNTP and Task Notifications.  Task Notifications
//       are simple flags sent between tasks which are fundemental to the system - like connected states.
//
// Init - This is our initialization loop.  We don't allow Run or Command loops to run until initialization is complete.  Once
//        complete, we don't enter intialization again.
//
enum class SYS_OP : uint8_t // System Operations
{
    Run,
    Init,
    Error,
};

enum class SYS_INIT : uint8_t // System Initialization states
{
    Start,
    InitNVS,
    RestoreSysVaribles,
    CreateIND,
    WaitOnIND,
    Finished,
};

//
// System Command are not currently in use.
//
enum class SYS_CMD : uint8_t // System Commands states
{
    NONE,
};

//
// System Command Queue format.  This shows how we might receive and respond to system commands.
// NOT IN USE AT THIS TIME
//
struct SYS_CmdRequest
{
    QueueHandle_t QueueToSendResponse; // 4 bytes.   If NULL, no response will be sent.
    SYS_CMD RequestedCmd;
    void *data;
};

struct SYS_Response
{
    SYS_CMD ResponseCmd;
    void *data;
};