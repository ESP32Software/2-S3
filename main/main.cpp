#include "system.hpp"

extern "C" void app_main(void)
{
    //
    // System Info
    //
    ESP_LOGI("MAIN", "Startup...");
    ESP_LOGI("MAIN", "Free heap memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("MAIN", "IDF version: %s", esp_get_idf_version());

    __attribute__((unused)) auto &sys = System::getInstance(); // Create the system singleton object...

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(300000)); // Main task does very little except handle OTA restart functions when needed.
        ESP_LOGI("MAIN", "Free heap memory: %d bytes", esp_get_free_heap_size());
    }
}