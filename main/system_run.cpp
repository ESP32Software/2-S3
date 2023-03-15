#include "system.hpp"

extern xSemaphoreHandle semWifiEntry;

void System::runMarshaller(void *arg)
{
    System *obj = (System *)arg;
    obj->run();

    if (obj->taskHandleSystemRun == nullptr)
        return;

    auto temp = obj->taskHandleSystemRun;
    obj->taskHandleSystemRun = nullptr;
    vTaskDelete(temp);
}

void System::run(void)
{
    //
    // Our Run operation concerns itself with being connected, having the correct time, and handling incoming commands.
    //
    while (true)
    {
        switch (SysOp)
        {
        case SYS_OP::Run:
        {

            if (xQueuePeek(sysCmdRequestQue, &ptrSYSCmdRequest, pdMS_TO_TICKS(30))) // We cycle through here and look for incoming mail box command requests
            {
                switch (ptrSYSCmdRequest->RequestedCmd)
                {
                case SYS_CMD::NONE:
                {
                    break;
                }
                }

                xQueueReceive(sysCmdRequestQue, &ptrSYSCmdRequest, pdMS_TO_TICKS(30));
            }

            vTaskDelay(pdMS_TO_TICKS(50)); // We always need to allow the OS to free up time for other tasks.

            if (showRun)
                ESP_LOGI(TAG, "run()");
            break;
        }

            //
            // Initialization happens during program startup
            //
        case SYS_OP::Init:
        {
            switch (initSysStep)
            {
            case SYS_INIT::Start:
            {
                ESP_LOGI(TAG, "System Initialization");
                initSysStep = SYS_INIT::InitNVS;
                [[fallthrough]];
            }

            case SYS_INIT::InitNVS:
            {
                initNVS();
                initSysStep = SYS_INIT::CreateIND;
                break;
            }

            case SYS_INIT::RestoreSysVaribles:
            {
                initSysStep = SYS_INIT::CreateIND;
                break;
            }

            case SYS_INIT::CreateIND:
            {
                if (showInit)
                    ESP_LOGI(TAG, "Step 1  - CreateIND");

                if (ind == nullptr)
                    ind = new Indication(this, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_REVISION);

                if (ind != nullptr)
                {
                    initSysStep = SYS_INIT::WaitOnIND;
                    if (showInit)
                        ESP_LOGI(TAG, "Step 2  - WaitOnIND");
                }
                break;
            }

            case SYS_INIT::WaitOnIND:
            {
                if (xSemaphoreTake(semIndEntry, 5) == pdTRUE)
                {
                    indColorCmdRequestQue = ind->getColorCmdRequestQueue();
                    xSemaphoreGive(semIndEntry);
                    ESP_LOGI(TAG, "BEFORE IDLE - Free heap memory: %d bytes", esp_get_free_heap_size());
                    initSysStep = SYS_INIT::Finished;
                }
                break;
            }

            case SYS_INIT::Finished:
            {
                ESP_LOGI(TAG, "Initialization Finished");
                initGenTimer(); // Starting General Task and Timer
                SysOp = SYS_OP::Run;
                break;
            }
            }

            break;
        }

        case SYS_OP::Error:
        {
            ESP_LOGE(TAG, "Error...");
            vTaskDelay(pdMS_TO_TICKS(15000));
            SysOp = SYS_OP::Run;
            break;
        }
        }
    }
}
