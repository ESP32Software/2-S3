#include "system.hpp"

extern xSemaphoreHandle semSYSEntry;
//
// We generally handle GPIO interrupts here.  The idea is to route them to the handler which is
// designed for that service.
//
// Right now, we have a tactile switch.  We can use this switch for debugging.  Notice that we debounce the
// switch in software.   We handle switch input by the book -- first catching the ISR and then routing that
// to a queue.
//
#define ESP_INTR_FLAG_DEFAULT 0

#define SWITCH_1 GPIO_NUM_0 // Boot Switch -- GPIO_EN.  This a strapping pin is pulled-up by default

//
// NOTE: Pins GPIO_NUM_19 / GPIO_NUM_20 are reservied for JTAG
//
bool gpio_isr_service_started = false; // We would receive an error if we tried to start this service a second time.
bool blnallowSwitchGPIOinput = true;   // These variables are used for switch input debouncing
QueueHandle_t xQueueGPIOEvents = nullptr;
uint8_t SwitchDebounceCounter = 0;

extern bool blnSwitch1; // Switch variables needed for

void System::initGPIOPins(void)
{
    //
    // At one time, we used to initialize all pins here, but new libraries have been taking that job away.
    //

    //
    // Switches
    //
    gpio_config_t gpioSW1; // GPIO_BOOT_SW
    gpioSW1.pin_bit_mask = 1LL << SWITCH_1;
    gpioSW1.mode = GPIO_MODE_INPUT;
    gpioSW1.pull_up_en = GPIO_PULLUP_ENABLE;
    gpioSW1.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&gpioSW1);
}

//
// This ISR is set apart because tactile switch input needs to be handled with a debouncing algorithm.
//
void IRAM_ATTR GPIOSwitchIsrHandler(void *arg)
{
    if (blnallowSwitchGPIOinput)
    {
        xQueueSendToBackFromISR(xQueueGPIOEvents, &arg, NULL);
        SwitchDebounceCounter = 50; // Reject all input for 1/2 of a second -- counter is running in system_timer
        blnallowSwitchGPIOinput = false;
    }
}

//
// Normal (non-switch) GPIO ISR handling would warrant no delay.
//
void IRAM_ATTR GPIOIsrHandler(void *arg)
{
    xQueueSendToBackFromISR(xQueueGPIOEvents, &arg, NULL);
}

void System::initGPIOTask(void)
{
    // ESP_LOGI(TAG, "InitGPIOTask");
    xQueueGPIOEvents = xQueueCreate(1, sizeof(uint32_t)); // Create a queue to handle gpio events from isr

    if (xQueueGPIOEvents != NULL) // if QueueCreate fails to find memory, don't start ISR handlers
    {
        if (!gpio_isr_service_started)
        {
            if (gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT) == ESP_OK)
            {
                ESP_LOGI(TAG, "Started gpio isr service...");
                gpio_isr_service_started = true;
            }
        }

        gpio_isr_handler_add(SWITCH_1, GPIOSwitchIsrHandler, (void *)SWITCH_1);

        SwitchDebounceCounter = 30;
        blnallowSwitchGPIOinput = true;

        xTaskCreate(runGPIOTaskMarshaller, "SYS::GPIO", 1024 * 3, this, 7, &runTaskHandleSystemGPIO); // (1) Low number indicates low priority task
    }
}

void System::runGPIOTaskMarshaller(void *arg) // This function can be resolved at run time by the compiler.
{
    auto obj = (System *)arg;
    obj->runGPIOTask();

    if (obj->runTaskHandleSystemGPIO != nullptr)
        vTaskDelete(NULL);
}

void System::runGPIOTask(void)
{
    uint32_t io_num;

    while (true)
    {
        if (xQueueReceive(xQueueGPIOEvents, &io_num, pdMS_TO_TICKS(50)))
        {
            // ESP_LOGI(TAG, "xQueueGPIOEvents   io_num  %d", io_num);
            //  ESP_LOGI(TAG, "xQueueGPIOEvents ...SysOp = %x", (uint8_t)SysOp);
            if (SysOp == SYS_OP::Init) // If we haven't finished out our initialization -- discard items in our queue.
                continue;

            switch (io_num)
            {
            case SWITCH_1:
            {
                ESP_LOGI(TAG, "SWITCH_1 triggered ...");

                // ESP_ERROR_CHECK(nvs_flash_erase());
                // ESP_LOGI(TAG, "NVS Erased...");

                if (xSemaphoreTake(semSYSEntry, portMAX_DELAY) == pdTRUE)
                {
                    if (openNVStorage("indication", true) == false)
                    {
                        ESP_LOGE(TAG, "Error, Unable to OpenNVStorage inside restoreVariblesFromNVS");
                        xSemaphoreGive(semSYSEntry);
                        break;
                    }
                }

                uint8_t aValue = 0;
                uint8_t bValue = 0;
                uint8_t cValue = 0;

                if (TempFlag == 1)
                {
                    TempFlag++;
                    aValue = 255;
                    bValue = 255;
                    cValue = 255;
                }
                else if (TempFlag == 2)
                {
                    TempFlag++;
                    aValue = 1;
                    bValue = 1;
                    cValue = 1;
                }
                else if (TempFlag == 3)
                {
                    aValue = 5;
                    bValue = 10;
                    cValue = 50;
                    TempFlag = 1;
                }
                else
                    TempFlag = 1;

                if (saveU8IntegerToNVS("aDefValue", aValue) == false)
                    ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS aDefValue");
                else
                    ESP_LOGW(TAG, "aDefValue is now %d", aValue);

                if (saveU8IntegerToNVS("bDefValue", bValue) == false)
                    ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS bDefValue");
                else
                    ESP_LOGW(TAG, "bDefValue is now %d", bValue);

                if (saveU8IntegerToNVS("cDefValue", cValue) == false)
                    ESP_LOGE(TAG, "Error, Unable to saveU8IntegerToNVS cDefValue");
                else
                    ESP_LOGW(TAG, "cDefValue is now %d", cValue);

                closeNVStorage(true); // Commit changes
                xSemaphoreGive(semSYSEntry);

                // blnSwitch1 = true;
                break;
            }

            default:
                ESP_LOGI(TAG, "Missing Case for io_num  %d...(runGPIOTask)", io_num);
                break;
            }
        }
    }
}
