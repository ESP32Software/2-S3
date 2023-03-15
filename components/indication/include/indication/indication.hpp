#pragma once
#include "indication_defs.hpp"

#include <string> // Native Libraries

#include "esp_log.h" // ESP Libraries
#include "led_strip.h"

#include "freertos/FreeRTOS.h" // RTOS Libraries
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "system.hpp"

extern xSemaphoreHandle semIndEntry; // External Variables

class System; // Class Declarations

extern "C"
{
    class Indication
    {
    public:
        Indication(System *, int8_t, int8_t, int8_t);
        ~Indication();

        QueueHandle_t &getColorCmdRequestQueue(void);

    private:
        char TAG[5] = "IND ";
        System *sys = nullptr;

        led_strip_t *pStrip_a;

        uint8_t majorVer;
        uint8_t minorVer;
        uint8_t revVer;

        LED_STATE aState = LED_STATE::AUTO; // This is a Color State
        LED_STATE bState = LED_STATE::AUTO;
        LED_STATE cState = LED_STATE::AUTO;

        bool aState_nvs_dirty = false;
        bool bState_nvs_dirty = false;
        bool cState_nvs_dirty = false;

        bool aState_iot_dirty = false;
        bool bState_iot_dirty = false;
        bool cState_iot_dirty = false;

        uint8_t aCurrValue = 0; // Curent values are not preserves
        uint8_t bCurrValue = 0;
        uint8_t cCurrValue = 0;

        uint8_t aDefaultValue = 5; // Default values are brightness levels
        uint8_t bDefaultValue = 10;
        uint8_t cDefaultValue = 50;

        bool aDefaultValue_nvs_dirty = false;
        bool bDefaultValue_nvs_dirty = false;
        bool cDefaultValue_nvs_dirty = false;

        IND_OP indOP = IND_OP::Run;
        IND_INIT initINDStep = IND_INIT::Finished;
        IND_STATES indStates = IND_STATES::Init;

        TaskHandle_t runTaskIndication = nullptr;

        QueueHandle_t queColorCmdRequests = nullptr; // IND <-- ?? (Request Queue is here)

        bool IsIndicating = false;

        /* NVS Variables*/
        bool blnSaveNVSVariables = false;

        uint8_t clearLEDTargets;
        uint8_t setLEDTargets;

        uint8_t dark_delay;
        uint8_t dark_delay_counter;

        uint8_t first_color_target;
        uint8_t first_color_cycles;

        uint8_t second_color_target;
        uint8_t second_color_cycles;

        uint8_t color_timeout;
        uint8_t color_timeout_counter;

        uint8_t first_color_timeout_counter;
        uint8_t second_color_timeout_counter;

        void startIndication(uint32_t);
        void setAndClearColors(uint8_t, uint8_t);
        void resetIndication(void);

        bool restoreVariblesFromNVS(void);
        bool saveVariblesToNVS(void);
        std::string getStateText(uint8_t);

        static void runMarshaller(void *);
        void run(void);

        /* Debug Flags */
        bool showInitSteps = false;
        bool showRun = true;
        bool showNVSActions = true;
    };
}
