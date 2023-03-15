#include "system.hpp"

extern bool blnallowSwitchGPIOinput;
extern uint8_t SwitchDebounceCounter;

#define TIMER_PERIOD_1kHz 1000 // 1000 microseconds = .001 second  = 1000Hz

auto OneHundredHertz = 10;
auto QuarterSeconds = 25;
auto HalfSecond = 2;
auto OneSecond = 2;
auto FiveSeconds = 5;
auto TenSeconds = 10;
auto OneMinute = 6;
auto FiveMinutes = 5;

void System::initGenTimer(void)
{
    xTaskCreate(runGenTimerTaskMarshaller, "SYS::TIMER", 1024 * 2, this, 5, &xTaskHandleSystemTimer);

    const esp_timer_create_args_t general_timer_args = {
        .callback = &System::genTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "general_timer", // name is optional, but may help identify the timer when debugging
        .skip_unhandled_events = true};

    ESP_ERROR_CHECK(esp_timer_create(&general_timer_args, &General_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(General_timer, TIMER_PERIOD_1kHz));
}

void IRAM_ATTR System::genTimerCallback(void *arg)
{
    // Our priorty here is to exit our ISR with minumum processing so the system can return to normal task servicing quickly.
    // Therefore, we will exercise Deferred Interrupt Processing as often as posible.
    // NOTE: Any high priorty task will essentially run as if it were the ISR itself if there are no other equally high prioirty tasks running.
    //
    auto obj = (System *)arg;
    vTaskNotifyGiveFromISR(obj->xTaskHandleSystemTimer, NULL);
}

void System::runGenTimerTaskMarshaller(void *arg)
{
    auto obj = (System *)arg;
    obj->runGenTimerTask();

    if (obj->xTaskHandleSystemTimer != nullptr)
        vTaskDelete(NULL);
}

void System::runGenTimerTask(void)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // We are using task notification to trigger this routine at 1000hz.
        //
        // 1000hz Processing
        //
        //
        // Periodic Processes by Time
        //
        if (OneHundredHertz > 0)
        {
            if (--OneHundredHertz < 1) // 100Hz Processing here
            {
                // Perform GPIO Switch Debouncing delay here.
                if (SwitchDebounceCounter > 0)
                {
                    if (--SwitchDebounceCounter < 1)
                        blnallowSwitchGPIOinput = true;
                }

                if (QuarterSeconds > 0)
                {
                    if (--QuarterSeconds < 1) // 4Hz Processing here
                    {
                        if (HalfSecond > 0)
                        {
                            if (--HalfSecond < 1) // Halfsecond activities
                            {
                                if (OneSecond > 0)
                                {
                                    if (--OneSecond < 1)
                                    {
                                        if (showTimerSeconds)
                                            ESP_LOGI(TAG, "One Second");

                                        // const int32_t val = 0x11000301; // 1 second heartbeat in red
                                        // const int32_t val = 0x21000301; // 1 second heartbeat in green
                                        // const int32_t val = 0x41000309; // 1 second heartbeat in blue
                                        // const int32_t val = 0x31000309; // 1 second heartbeat in yellow
                                        // const int32_t val = 0x61000309; // 1 second heartbeat in cyan
                                        // const int32_t val = 0x51000309; // 1 second heartbeat in violet
                                        const int32_t val = 0x71000309; // 1 second heartbeat in white

                                        if (indColorCmdRequestQue != nullptr)
                                            xQueueSendToBack(indColorCmdRequestQue, &val, 0);

                                        if (FiveSeconds > 0)
                                        {
                                            if (--FiveSeconds < 1)
                                            {
                                                if (showTimerSeconds)
                                                    ESP_LOGI(TAG, "Five Seconds");

                                                FiveSeconds = 5;
                                            }
                                        }

                                        if (TenSeconds > 0)
                                        {
                                            if (--TenSeconds < 1) // 0.1Hz Processing here
                                            {
                                                if (showTimerSeconds)
                                                    ESP_LOGI(TAG, "Ten Seconds");

                                                if (OneMinute > 0)
                                                {
                                                    if (--OneMinute < 1)
                                                    {
                                                        if (showTimerMinutes)
                                                            ESP_LOGI(TAG, "One Minute");

                                                        if (FiveMinutes > 0)
                                                        {
                                                            if (--FiveMinutes < 1)
                                                            {
                                                                if (showTimerMinutes)
                                                                    ESP_LOGI(TAG, "Five Minutes");

                                                                FiveMinutes = 5;
                                                            }
                                                        }
                                                        OneMinute = 6;
                                                    }
                                                }
                                                TenSeconds = 10;
                                            }
                                        }
                                        OneSecond = 2;
                                    }
                                }
                                HalfSecond = 2;
                            }
                        }
                        QuarterSeconds = 25;
                    }
                }
                OneHundredHertz = 10;
            }
        }
    } // While loop
}
