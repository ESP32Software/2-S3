#pragma once
#include "sdkconfig.h"
#include "system_defs.hpp"

#include <stddef.h> // Standard libraries
#include <stdint.h>
#include <stdbool.h>
#include <queue>
#include <iostream>
#include <sstream>
#include "string.h"

#include "freertos/task.h" // RTOS Libraries
#include "freertos/event_groups.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/semphr.h"

#include <driver/gpio.h> // IDF Libraries
#include <esp_log.h>
#include <esp_err.h>
#include "esp_timer.h"

#include "indication/indication.hpp" // Components

#include "nvs_flash.h"
#include "nvs.h"

class Indication; // Forward declarations

extern "C"
{
    class System
    {
    public:
        static System &getInstance() // Enforce use of System as a singleton object
        {
            static System sysInstance;
            return sysInstance;
        }

        System(const System &) = delete;         // Disable copy constructor
        void operator=(System const &) = delete; // Disable assignment operator

        /* Task Handle Calls */
        xTaskHandle getGenTaskHandle(void);
        xTaskHandle getIOTTaskHandle(void);

        /* Non Volatile Storage */
        bool openNVStorage(const char *, bool = true);
        bool getBooleanFromNVS(const char *, bool *);
        bool getStringFromNVS(const char *, std::string *);
        bool getU8IntegerFromNVS(const char *, uint8_t *);
        bool getU16IntegerFromNVS(const char *, uint16_t *);
        bool saveBooleanToNVS(const char *, bool);
        bool saveStringToNVS(const char *, std::string *);
        bool saveU8IntegerToNVS(const char *, uint8_t);
        bool saveU16IntegerToNVS(const char *, uint16_t);
        void closeNVStorage(bool);

    private:
        System(void); // Creating the singlton object with private construction

        char TAG[5] = "SYS ";

        /* Object Pointers */
        Indication *ind = nullptr;

        /* task handles */
        xTaskHandle taskHandleSystemRun = nullptr;

        static void runMarshaller(void *);
        void run(void); // Handles most main System activites that are not periodic

        /* System GPIO */
        xTaskHandle runTaskHandleSystemGPIO = nullptr;

        static void runGPIOTaskMarshaller(void *);
        void runGPIOTask(void); // Handles GPIO Interrupts on Change Events

        void initGPIOPins(void);
        void initGPIOTask(void);

        /* System Timer */
        esp_timer_handle_t General_timer;
        xTaskHandle xTaskHandleSystemTimer = nullptr;

        static void runGenTimerTaskMarshaller(void *);
        void runGenTimerTask(void); // Handles all Timer related events

        uint8_t SyncEventTimeOut_Counter = 0;

        void initGenTimer(void);
        static void genTimerCallback(void *);

        /* RTOS */
        xTaskHandle taskHandleIOTRUN = nullptr; // Task Handles for notification

        /* State Variables */
        SYS_OP SysOp = SYS_OP::Init;
        SYS_INIT initSysStep = SYS_INIT::Finished;

        /* Command Request Queues */
        QueueHandle_t sysCmdRequestQue = nullptr;   // SYS <--  (Queue is in SYS)
        SYS_CmdRequest *ptrSYSCmdRequest = nullptr; // Structs for Sending/Receiving data to/from System Object
        SYS_Response *ptrSYSResponse = nullptr;

        QueueHandle_t indColorCmdRequestQue = nullptr;

        /* Non Volatile Storage */
        nvs_handle_t nvsHandle = 0;
        void initNVS(void);
        uint16_t retrieveLengthOfStringInNVM(const char *);
        bool retrieveStringWithKeyFromNVS(const char *, char *, size_t *);

        uint8_t TempFlag = 1;

        /* Debug Flags */
        bool showRun = false;
        bool showNVMDebug = false;
        bool showRunCmd = true;
        bool showInit = true;
        bool showIdle = false;
        bool showTimerSeconds = false;
        bool showTimerMinutes = false;
    };
}
