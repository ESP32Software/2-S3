#include "indication/indication.hpp"
#include "led_strip.h"

extern xSemaphoreHandle semSYSEntry;
xSemaphoreHandle semIndEntry = NULL;

Indication::Indication(System *mySys, int8_t parmMajor, int8_t parmMinor, int8_t parmRev)
{
    sys = mySys;
    majorVer = parmMajor; // We pass in the software version of the project so out LEDs
    minorVer = parmMinor; // can flash this out during startup
    revVer = parmRev;

    vSemaphoreCreateBinary(semIndEntry);
    if (semIndEntry != NULL)
        xSemaphoreTake(semIndEntry, portMAX_DELAY); // Hold the locking semaphore until initialization is complete

    pStrip_a = led_strip_init(LED_RMT_CHANNEL, TRI_COLOR_LED_GPIO, 1); // LED strip initialization with the GPIO and pixels number

    resetIndication();

    queColorCmdRequests = xQueueCreate(3, sizeof(uint32_t)); // We can receive up to 3 indication requests and they will play out in order.

    indOP = IND_OP::Init;
    initINDStep = IND_INIT::Start;
    xTaskCreate(runMarshaller, "IND::Run", 1024 * 3, this, 5, &runTaskIndication); // Low number indicates low priority task
}

Indication::~Indication()
{
    if (queColorCmdRequests != nullptr)
        vQueueDelete(queColorCmdRequests);

    if (semIndEntry != NULL)
        vSemaphoreDelete(semIndEntry);

    if (this->runTaskIndication != nullptr)
        vTaskDelete(NULL);
}

QueueHandle_t &Indication::getColorCmdRequestQueue(void)
{
    return queColorCmdRequests;
}

//
// This is a 2 Color Sequencer service.  It is  used to deliver 1 or 2 numbers with "blinks" of color.
// Colors may be of a single color or composed of several colors (a combination color).
//
// The assumption is that we have a 4 possible LED indicators, ColorA through ColorD.  Any combination of those 4 colors create 1
// combination color.
//
// We may sequence 2 combination (or single) colors to deliver a two number message through flashing the LEDs.   We may flash any color combination in
// the sequence up to 13 pulses.   We may also choose to flash only one color (or color combination) for up to 13 cycles.
//
//
// The 32 bit Task Notification number we receive encodes an full LED indication sequence
//
//
// byte 1 <color1/cycles>  byte 2 <color1/cycles>  byte 3 <color time-out (time on)>  byte 4 <dark delay (time off)>
//
// 1st byte format for FirstColor is   MSBit    0x<Colors><Cycles>	LSBit
// <Colors>   0x1 = ColorA, 0x2 = ColorB, 0x4 = ColorC, 0x8 = ColorD (4 bits in use here)
// <Cycles>   13 possible flashes - 0x01 though 0x0E (1 through 13) Special Command Codes: 0x00 = ON State, 0x0E = AUTO State, 0x0F = OFF State (4 bits in use here)
//
// NOTE: Special Command codes apply to the states of all LEDs in a combination color.
//
// Second byte is the Second Color/Cycles.
// Third byte is color_timeout (on-time) -- how long the color is on
// Fourth byte is dark_delay (off-time)  -- the time the LEDs are off between cycles.
//
// The values of color_timeout and dark_delay are shared between both possible color sequences.
//
// Examples:
//  ColorA  Cycles  ColorB   Cycles   On-Time   Off-Time
//  0x1     1       0x2      2        0x20      0x30
//  0x11222030
//
//  ColorC  Cycles  NoColor  Cycles   On-Time   Off-Time
//  0x4     3       0        0        0x01      0x15
//  0x43000115
//
//  All Colors Cycles  NoColor Cycles On-Time  Off-Time -- Effectively takes all colors and turns them ON continously.
//  0x7        0       0       0      0        0
//
//  All Colors Cycles  NoColor Cycles On-Time  Off-Time -- Effectively takes all colors and turns them OFF completely.
//  0x7        F       0       0      0        0
//
//
//
// PLEASE CALL ON THIS SERVICE LIKE THIS:
//
// const int32_t val = 0x22820919;
// xQueueSendToBack(indCmdRequestQue, &val, 30);
//
//
//
void Indication::startIndication(uint32_t value)
{
    //  std::cout << "IO Value: " << (uint32_t *)value << std::endl;
    first_color_target = (0xF0000000 & value) >> 28; // First Color(s) -- We may see any of the color bits set
    first_color_cycles = (0x0F000000 & value) >> 24; // First Color Cycles

    // ESP_LOGW(TAG, "First Color 0x%02x", first_color_target);
    // ESP_LOGW(TAG, "First Color 0x%02x", first_color_cycles);

    second_color_target = (0x00F00000 & value) >> 20; // Second Color(s)
    second_color_cycles = (0x000F0000 & value) >> 16; // Second Color Cycles

    color_timeout = (0x0000FF00 & value) >> 8; // Time out
    dark_delay = (0x000000FF & value);         // Dark Time

    first_color_timeout_counter = color_timeout;
    second_color_timeout_counter = color_timeout;

    // ESP_LOGI(TAG, "First Color 0x%0X Cycles 0x%0X", first_color_target, first_color_cycles);
    // ESP_LOGI(TAG, "Second Color 0x%0X Cycles 0x%0X", second_color_target, second_color_cycles);
    // ESP_LOGI(TAG, "Color Time 0x%02X", color_timeout);
    // ESP_LOGI(TAG, "Dark Time 0x%02X", dark_delay);

    //
    // Manually turning ON and OFF or AUTO -- the led LEDs happens always in the First Color byte
    //
    if (first_color_cycles == 0x00) // Turn Colors On and exit routine
    {
        ESP_LOGW(TAG, "first_color_cycles == 0x00");

        if (first_color_target & COLORA_Bit)
            aState = LED_STATE::ON;

        if (first_color_target & COLORB_Bit)
            bState = LED_STATE::ON;

        if (first_color_target & COLORC_Bit)
            cState = LED_STATE::ON;

        clearLEDTargets = 0;
        setLEDTargets = (uint8_t)COLORA_Bit | (uint8_t)COLORB_Bit | (uint8_t)COLORC_Bit;
        setAndClearColors(setLEDTargets, clearLEDTargets);
    }
    else if (first_color_cycles == 0x0F) // Turn Colors Off and exit routine
    {
        ESP_LOGW(TAG, "first_color_cycles == 0x0F");

        if (first_color_target & COLORA_Bit)
            aState = LED_STATE::OFF;

        if (first_color_target & COLORB_Bit)
            bState = LED_STATE::OFF;

        if (first_color_target & COLORC_Bit)
            cState = LED_STATE::OFF;

        setLEDTargets = 0;
        clearLEDTargets = (uint8_t)COLORA_Bit | (uint8_t)COLORB_Bit | (uint8_t)COLORC_Bit;
        setAndClearColors(setLEDTargets, clearLEDTargets);
    }
    else if (first_color_cycles == 0x0E) // Turn Colors to AUTO State and exit routine
    {
        ESP_LOGW(TAG, "first_color_cycles == 0x0E");

        if (first_color_target & COLORA_Bit)
            aState = LED_STATE::AUTO;

        if (first_color_target & COLORB_Bit)
            bState = LED_STATE::AUTO;

        if (first_color_target & COLORC_Bit)
            cState = LED_STATE::AUTO;

        setAndClearColors(first_color_target, 0);
    }
    else
    {
        setAndClearColors(first_color_target, 0); // Process normal color display
        first_color_cycles--;
        indStates = IND_STATES::Show_FirstColor;
        IsIndicating = true;
    }
}

void Indication::setAndClearColors(uint8_t SetColors, uint8_t ClearColors)
{
    //
    // ESP_LOGI(TAG, "SetAndClearColors 0x%02X 0x%02X", SetColors, ClearColors); // Debug info
    // ESP_LOGI(TAG, "Colors Curr/Default  %d/%d   %d/%d   %d/%d", aCurrValue, aDefaultValue, bCurrValue, bDefaultValue, cCurrValue, cDefaultValue);
    //
    if (ClearColors & COLORA_Bit) // Seeing the bit to Clear this color
    {
        if (aState == LED_STATE::ON)
            aCurrValue = aDefaultValue; // Don't turn off this value because our stat is ON
        else
            aCurrValue = 0; // Otherwide, do turn it off.

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue); // Green and Blue and just set again to their current values
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10)); // If we dont' yield, the LED library doesn't have time to service the LED bus between successive led_strip calls.
    }

    if (ClearColors & COLORB_Bit)
    {
        if (bState == LED_STATE::ON)
            bCurrValue = bDefaultValue;
        else
            bCurrValue = 0;

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue);
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (ClearColors & COLORC_Bit)
    {
        if (cState == LED_STATE::ON)
            cCurrValue = cDefaultValue;
        else
            cCurrValue = 0;

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue);
        cCurrValue = 0;
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (SetColors & COLORA_Bit) // Setting the bit to Set this color
    {
        if (aState == LED_STATE::OFF) // Set the color if the state is set above OFF
            aCurrValue = 0;           // Don't allow any value to be displayed on the LED
        else
            aCurrValue = aDefaultValue; // State is either AUTO or ON.

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue); // Again, Green and Blue and just set again to their current values
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (SetColors & COLORB_Bit)
    {
        if (bState == LED_STATE::OFF)
            bCurrValue = 0;
        else
            bCurrValue = bDefaultValue;

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue);
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (SetColors & COLORC_Bit)
    {
        if (cState == LED_STATE::OFF)
            cCurrValue = 0;
        else
            cCurrValue = cDefaultValue;

        pStrip_a->set_pixel(pStrip_a, 0, aCurrValue, bCurrValue, cCurrValue);
        pStrip_a->refresh(pStrip_a, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // ESP_LOGW(TAG, "Red   State/Value  %d/%d", (int)aState, aCurrValue);
    // ESP_LOGW(TAG, "Green State/Value  %d/%d", (int)bState, bCurrValue);
    // ESP_LOGW(TAG, "Blue  State/Value  %d/%d", (int)cState, cCurrValue);
    // ESP_LOGW(TAG, "---------------------------------------------------");
}

void Indication::resetIndication()
{
    first_color_target = 0;
    first_color_cycles = 0;

    second_color_target = 0;
    second_color_cycles = 0;

    dark_delay = 0;
    dark_delay_counter = 0;

    color_timeout = 0;
    color_timeout_counter = 0;

    indStates = IND_STATES::Init;
    IsIndicating = false;
}

void Indication::runMarshaller(void *arg)
{
    auto obj = (Indication *)arg;
    obj->run();

    if (obj->runTaskIndication != nullptr)
        vTaskDelete(NULL);
}

void Indication::run(void)
{
    uint8_t cycles = 0;
    uint32_t value = 0;
    //
    // Set Object Debug Level
    // Note that this function can not raise log level above the level set using CONFIG_LOG_DEFAULT_LEVEL setting in menuconfig.
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    //
    TickType_t startTime;
    TickType_t waitTime = 100;

    while (true)
    {
        switch (indOP)
        {
        case IND_OP::Run:
        {
            if (IsIndicating)
            {
                waitTime = 1;
                startTime = xTaskGetTickCount();
                vTaskDelayUntil(&startTime, waitTime);

                switch (indStates)
                {
                case IND_STATES::Init:
                {
                    break;
                }

                case IND_STATES::Show_FirstColor:
                {
                    if (first_color_timeout_counter > 0)
                    {
                        if (--first_color_timeout_counter < 1)
                        {
                            setAndClearColors(0, first_color_target);
                            indStates = IND_STATES::FirstColor_Dark;

                            if (first_color_cycles == 0)
                            {
                                dark_delay_counter = (dark_delay * 2);
                                indStates = IND_STATES::SecondColor_Dark;
                            }
                            else
                            { // If we are finished with the first color and we don't have a second color add extra delay time.
                                if ((first_color_cycles < 1) && (second_color_target == 0))
                                    dark_delay_counter = 3 * dark_delay;
                                else
                                    dark_delay_counter = dark_delay;
                            }
                        }
                    }
                    break;
                }

                case IND_STATES::Show_SecondColor:
                {
                    if (second_color_timeout_counter > 0)
                    {
                        if (--second_color_timeout_counter < 1)
                        {
                            setAndClearColors(0, second_color_target);
                            indStates = IND_STATES::SecondColor_Dark;

                            if (second_color_cycles < 1) // If we are finished with the second color -- add extra delay time
                                dark_delay_counter = 3 * dark_delay;
                            else
                                dark_delay_counter = dark_delay;
                        }
                    }
                    break;
                }

                case IND_STATES::FirstColor_Dark:
                case IND_STATES::SecondColor_Dark:
                {
                    if (dark_delay_counter > 0)
                    {
                        if (--dark_delay_counter < 1)
                        {
                            if (first_color_cycles > 0)
                            {
                                first_color_cycles--;
                                setAndClearColors(first_color_target, 0); // Turn on the LED
                                first_color_timeout_counter = color_timeout;
                                indStates = IND_STATES::Show_FirstColor;
                            }
                            else if ((second_color_cycles > 0) && (indStates == IND_STATES::SecondColor_Dark))
                            {
                                second_color_cycles--;
                                setAndClearColors(second_color_target, 0); // Turn on the LED
                                second_color_timeout_counter = color_timeout;
                                indStates = IND_STATES::Show_SecondColor;
                            }
                            else
                            {
                                indStates = IND_STATES::Final;
                            }
                        }
                    }

                    break;
                }

                case IND_STATES::Final:
                {
                    resetIndication(); // Resetting all the indicator variables
                    break;
                }
                }

                break;
            }
            else // When we are not indicating -- we are waiting for new indication requests
            {
                if (xQueueReceive(queColorCmdRequests, &value, portMAX_DELAY)) // We can afford to wait here for requests
                    startIndication(value);
            }
            break;
        }

        case IND_OP::Init:
        {
            switch (initINDStep)
            {
            case IND_INIT::Start:
            {
                ESP_LOGI(TAG, "Initalization Start");

                initINDStep = IND_INIT::ColorA_On;
                cycles = majorVer;
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            }

            case IND_INIT::ColorA_On:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 1  - Set ColorA");

                setAndClearColors((uint8_t)COLORA_Bit, 0);

                vTaskDelay(pdMS_TO_TICKS(100));
                initINDStep = IND_INIT::ColorA_Off;
                break;
            }

            case IND_INIT::ColorA_Off:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 2  - Clear ColorA");

                setAndClearColors(0, (uint8_t)COLORA_Bit);

                vTaskDelay(pdMS_TO_TICKS(150));

                if (cycles > 0)
                    cycles--;

                if (cycles > 0)
                {
                    initINDStep = IND_INIT::ColorA_On;
                }
                else
                {
                    cycles = minorVer;
                    initINDStep = IND_INIT::ColorB_On;
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                break;
            }

            case IND_INIT::ColorB_On:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 3  - Set ColorB");

                setAndClearColors((uint8_t)COLORB_Bit, 0);

                vTaskDelay(pdMS_TO_TICKS(100));
                initINDStep = IND_INIT::ColorB_Off;
                break;
            }

            case IND_INIT::ColorB_Off:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 4  - Clear ColorB");

                setAndClearColors(0, (uint8_t)COLORB_Bit);

                vTaskDelay(pdMS_TO_TICKS(150));

                if (cycles > 0)
                    cycles--;

                if (cycles > 0)
                {
                    initINDStep = IND_INIT::ColorB_On;
                }
                else
                {
                    cycles = revVer;
                    initINDStep = IND_INIT::ColorC_On;
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                break;
            }

            case IND_INIT::ColorC_On:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 5  - Set ColorC");

                setAndClearColors((uint8_t)COLORC_Bit, 0);

                vTaskDelay(pdMS_TO_TICKS(100));
                initINDStep = IND_INIT::ColorC_Off;
                break;
            }

            case IND_INIT::ColorC_Off:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 6  - Clear ColorC");

                setAndClearColors(0, (uint8_t)COLORC_Bit);

                vTaskDelay(pdMS_TO_TICKS(150));

                if (cycles > 0)
                    cycles--;

                if (cycles > 0)
                {
                    initINDStep = IND_INIT::ColorC_On;
                }
                else
                {
                    initINDStep = IND_INIT::Restore_Settings;
                    vTaskDelay(pdMS_TO_TICKS(250));
                }
                break;
            }

            case IND_INIT::Restore_Settings:
            {
                if (showInitSteps)
                    ESP_LOGI(TAG, "Step 1  - Restore_Settings");

                if (restoreVariblesFromNVS() == false)
                    ESP_LOGE(TAG, "ERROR  restoreVariblesFromNVS");

                // We just restored all the Color State...
                // Now we need to act on them to put the LEDs any restrictive states as needed...

                if (aState == LED_STATE::ON)
                    setAndClearColors(COLORA_Bit, 0);
                else if (aState == LED_STATE::OFF)
                    setAndClearColors(0, COLORA_Bit);

                if (bState == LED_STATE::ON)
                    setAndClearColors(COLORB_Bit, 0);
                else if (aState == LED_STATE::OFF)
                    setAndClearColors(0, COLORB_Bit);

                if (cState == LED_STATE::ON)
                    setAndClearColors(COLORC_Bit, 0);
                else if (cState == LED_STATE::OFF)
                    setAndClearColors(0, COLORC_Bit);

                initINDStep = IND_INIT::Finished;
                break;
            }

            case IND_INIT::Finished:
            {
                ESP_LOGI(TAG, "Initialization Finished");
                indOP = IND_OP::Run;
                xSemaphoreGive(semIndEntry); // Let anyone waiting know that our Initialization is complete
                break;
            }
            }
            break;
        }
        }
    }
}

/* NVS Routines */
bool Indication::restoreVariblesFromNVS(void)
{
    if (showNVSActions)
        ESP_LOGI(TAG, "restoreVariblesFromNVS");

    if (sys == nullptr)
        return false;

    if (xSemaphoreTake(semSYSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (sys->openNVStorage("indication", true) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to OpenNVStorage inside restoreVariblesFromNVS");
            xSemaphoreGive(semSYSEntry);
            return false;
        }
    }

    /* Restore Color States */
    std::string key = "aState";
    uint8_t int8Val = 0;

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val == 0) // If the value has never been saved, set it to AUTO and mark it dirty
        {
            int8Val = (uint8_t)aState;
            aState_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions)
            ESP_LOGI(TAG, "aState is %s", getStateText((int)aState).c_str());
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    key = "bState";

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val == 0)
        {
            int8Val = (uint8_t)bState;
            bState_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions)
            ESP_LOGI(TAG, "bState is %s", getStateText((int)bState).c_str());
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    key = "cState";

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val == 0)
        {
            int8Val = (uint8_t)cState;
            cState_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions)
            ESP_LOGI(TAG, "cState is %s", getStateText((int)cState).c_str());
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    /* Restore Color Values */
    key = "aDefValue"; // Color A Default Value

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val > 0)             // Do not allow the Color Value to be set lower than 1
            aDefaultValue = int8Val; // The key idea is that we are restoring a saved default value.
        else                         // Value is zero which is unacceptable.
        {
            aDefaultValue = 1;
            aDefaultValue_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions) // If we don't have something stored, then we still have the program's default value.
            ESP_LOGI(TAG, "Color A Value Default %d", aDefaultValue);
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    key = "bDefValue";

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val > 0)
            bDefaultValue = int8Val;
        else
        {
            bDefaultValue = 1;
            bDefaultValue_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions)
            ESP_LOGI(TAG, "Color B Value Default %d", bDefaultValue);
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    key = "cDefValue";

    if (sys->getU8IntegerFromNVS(key.c_str(), &int8Val))
    {
        if (int8Val > 0)
            cDefaultValue = int8Val;
        else
        {
            cDefaultValue = 1;
            cDefaultValue_nvs_dirty = true;
            blnSaveNVSVariables = true;
        }

        if (showNVSActions)
            ESP_LOGI(TAG, "Color C Value Default %d", cDefaultValue);
    }
    else
    {
        ESP_LOGE(TAG, "getU8IntegerFromNVS failed on key %s", key.c_str());
        sys->closeNVStorage(false); // No changes
        xSemaphoreGive(semSYSEntry);
        return false;
    }

    sys->closeNVStorage(false); // No commit to any changes
    xSemaphoreGive(semSYSEntry);
    return true;
}

bool Indication::saveVariblesToNVS(void)
{
    if (showNVSActions)
        ESP_LOGW(TAG, "saveVariblesToNVS");

    if (sys == nullptr)
        return false;

    if (xSemaphoreTake(semSYSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (sys->openNVStorage("indication", true) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to OpenNVStorage inside saveVariblesToNVS");
            xSemaphoreGive(semSYSEntry);
            return false;
        }
    }

    //
    // Save Color States
    //
    if (aState_nvs_dirty)
    {
        if (showNVSActions)
            ESP_LOGW(TAG, "aState ................ %d", (int)aState);

        if (sys->saveU8IntegerToNVS("aState", (uint8_t)aState) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS aState");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        aState_nvs_dirty = false;
    }

    if (bState_nvs_dirty)
    {
        if (showNVSActions)
            ESP_LOGW(TAG, "bState ................ %d", (int)bState);

        if (sys->saveU8IntegerToNVS("bState", (uint8_t)bState) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS bState");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        bState_nvs_dirty = false;
    }

    if (cState_nvs_dirty)
    {
        if (showNVSActions)
            ESP_LOGW(TAG, "cState ................ %d", (int)cState);

        if (sys->saveU8IntegerToNVS("cState", (uint8_t)cState) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS cState");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        cState_nvs_dirty = false;
    }

    //
    // Save Default Color Values
    //
    if (aDefaultValue_nvs_dirty)
    {
        if (showNVSActions)
            ESP_LOGW(TAG, "aDefaultValue ................ %d", aDefaultValue);

        if (sys->saveU8IntegerToNVS("aDefValue", aDefaultValue) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS aDefaultValue");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        aDefaultValue_nvs_dirty = false;
    }

    if (bDefaultValue_nvs_dirty)
    {
        ESP_LOGW(TAG, "bDefaultValue ................ %d", bDefaultValue);

        if (sys->saveU8IntegerToNVS("bDefValue", bDefaultValue) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS bDefaultValue");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        bDefaultValue_nvs_dirty = false;
    }

    if (cDefaultValue_nvs_dirty)
    {
        if (showNVSActions)
            ESP_LOGW(TAG, "cDefaultValue ................ %d", cDefaultValue);

        if (sys->saveU8IntegerToNVS("cDefValue", cDefaultValue) == false)
        {
            ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS cDefaultValue");
            sys->closeNVStorage(false); // Discard changes
            xSemaphoreGive(semSYSEntry);
            return false;
        }
        cDefaultValue_nvs_dirty = false;
    }

    blnSaveNVSVariables = false;
    sys->closeNVStorage(true); // Commit changes
    xSemaphoreGive(semSYSEntry);
    return true;
}

std::string Indication::getStateText(uint8_t colorState)
{
    switch (colorState)
    {
    case 0:
        return "NONE";
    case 1:
        return "OFF";
    case 2:
        return "AUTO";
    case 3:
        return "ON";
    default:
        return "UNKNOWN";
    }
}