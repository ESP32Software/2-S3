#include "system.hpp"

/* Non Volatile Storage */
void System::initNVS()
{
    auto err = nvs_flash_init();

    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_LOGI(TAG, "********** Erasing NVS for use. **********");
        ESP_ERROR_CHECK(nvs_flash_erase()); // NVS partition was truncated and needs to be erased
        err = nvs_flash_init();             // Retry nvs_flash_init

        if (err != ESP_OK)
            ESP_LOGI(TAG, "Error(%s) Initializeing NVS!", esp_err_to_name(err));
    }
    ESP_ERROR_CHECK(err);
}

bool System::openNVStorage(const char *name_space, bool blnReadWrite)
{
    esp_err_t rc;

    if (blnReadWrite)
        rc = nvs_open(name_space, NVS_READWRITE, &nvsHandle);
    else
        rc = nvs_open(name_space, NVS_READONLY, &nvsHandle);

    // We will always fail on the first READONLY attempt.  READWRITE is required when namespace is new.
    if (rc == ESP_ERR_NVS_NOT_FOUND)
    {
        if (showNVMDebug)
            ESP_LOGW(TAG, "ESP_ERR_NVS_NOT_FOUND, creating new NVS space...");
        rc = nvs_open(name_space, NVS_READWRITE, &nvsHandle);

        if (blnReadWrite == false)
        {
            nvs_close(nvsHandle);
            rc = nvs_open(name_space, NVS_READONLY, &nvsHandle); // Re-open as requested
        }
    }
    else if (rc != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS ERROR - openNVStorage rc = 0x%04X", rc);
        nvsHandle = 0;
        return false;
    }

    return true;
}

bool System::getBooleanFromNVS(const char *key, bool *blnValue)
{
    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc;
        uint8_t val;

        rc = nvs_get_u8(nvsHandle, key, &val);

        if (rc == ESP_OK)
        {
            if (val == 1)
                *blnValue = true;
            else if (val == 0)
                *blnValue = false;
            else
                ESP_LOGE(TAG, "No value stored in Boolean with key of %s", key);
        }
        else
        {
            ESP_LOGE(TAG, "No value stored in Boolean with key of %s", key);
            return false;
        }
    }

    if (showNVMDebug)
    {
        if (*blnValue)
            ESP_LOGW(TAG, "getBooleanFromNVS  Sending value of 'true'");
        else
            ESP_LOGW(TAG, "getBooleanFromNVS  Sending value of 'false'");
    }

    return true;
}

bool System::getStringFromNVS(const char *key, std::string *strValue)
{
    if (showNVMDebug)
        ESP_LOGW(TAG, "getStringFromNVS passed in a key of %s", key);

    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        size_t length = retrieveLengthOfStringInNVM(key);

        if (length > 0)
        {
            char *value = (char *)malloc(length);

            if (retrieveStringWithKeyFromNVS(key, value, &length))
                strValue->append(value);

            if (showNVMDebug)
                ESP_LOGW(TAG, "getStringFromNVS  Sending value of %s", strValue->c_str());
            free(value);
        }
    }

    return true;
}

bool System::getU8IntegerFromNVS(const char *key, uint8_t *intValue)
{
    if (showNVMDebug)
        ESP_LOGW(TAG, "getU8IntegerFromNVS passed in a key of %s", key);

    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc = nvs_get_u8(nvsHandle, key, intValue);

        if (rc != ESP_OK)
        {
            ESP_LOGW(TAG, "No value stored as u8int with key of %s", key);
            *intValue = 0;
        }
        if (showNVMDebug)
            ESP_LOGW(TAG, "Value restored is %d", *intValue);
    }
    return true;
}

bool System::getU16IntegerFromNVS(const char *key, uint16_t *intValue)
{
    if (showNVMDebug)
        ESP_LOGW(TAG, "getU16IntegerFromNVS passed in a key of %s", key);

    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc = nvs_get_u16(nvsHandle, key, intValue);

        if (rc != ESP_OK)
        {
            ESP_LOGW(TAG, "No value stored as u16int with key of %s", key);
            *intValue = 0;
        }
    }
    return true;
}

bool System::saveBooleanToNVS(const char *key, bool blnVal)
{
    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc;
        uint8_t val;

        if (showNVMDebug)
        {
            if (blnVal)
                ESP_LOGW(TAG, "saveBooleanAsString key/value provided %s/true", key);
            else
                ESP_LOGW(TAG, "saveBooleanAsString key/value provided %s/false", key);
        }

        if (blnVal)
            val = 0;
        else
            val = 1;

        rc = nvs_set_u8(nvsHandle, key, val);

        if (rc != ESP_OK)
        {
            ESP_LOGE(TAG, "Error, Unable to saveBooleanToNVS.  esp_err_t code = %s", esp_err_to_name(rc));
            return false;
        }
        return true;
    }
}

bool System::saveStringToNVS(const char *key, std::string *strValue)
{
    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc;

        if (showNVMDebug)
            ESP_LOGW(TAG, "saveStringToNVS is key/value %s %s", key, strValue->c_str());

        rc = nvs_set_str(nvsHandle, key, strValue->c_str());

        if (rc != ESP_OK)
        {
            ESP_LOGE(TAG, "saveStringToNVS failed esp_err_t code = %s", esp_err_to_name(rc));
            return false;
        }
        else
        {
            if (showNVMDebug)
                ESP_LOGW(TAG, "Saved key/value %s / %s", key, strValue->c_str()); // Debug print statements
            return true;
        }
    }
}

bool System::saveU8IntegerToNVS(const char *key, uint8_t intValue)
{
    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc;

        if (showNVMDebug)
            ESP_LOGW(TAG, "saveU8IntegerToNVS is key/value %s %d", key, intValue);

        rc = nvs_set_u8(nvsHandle, key, intValue);

        if (rc != ESP_OK)
        {
            ESP_LOGE(TAG, "saveU8IntegerToNVS failed esp_err_t code = %s", esp_err_to_name(rc));
            return false;
        }
        else
        {
            if (showNVMDebug)
                ESP_LOGW(TAG, "Saved key/value %s / %d", key, intValue); // Debug print statements
            return true;
        }
    }
}

bool System::saveU16IntegerToNVS(const char *key, uint16_t intValue)
{
    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        esp_err_t rc;

        if (showNVMDebug)
            ESP_LOGW(TAG, "saveU16IntegerToNVS is key/value %s %d", key, intValue);

        rc = nvs_set_u16(nvsHandle, key, intValue);

        if (rc != ESP_OK)
        {
            ESP_LOGE(TAG, "saveU16IntegerToNVS failed esp_err_t code = %s", esp_err_to_name(rc));
            return false;
        }
        else
        {
            if (showNVMDebug)
                ESP_LOGW(TAG, "Saved key/value %s / %d", key, intValue); // Debug print statements
            return true;
        }
    }
}

uint16_t System::retrieveLengthOfStringInNVM(const char *key)
{
    esp_err_t rc;
    size_t required_size = 0;

    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        rc = nvs_get_str(nvsHandle, key, NULL, &required_size);
        if (showNVMDebug)
            ESP_LOGI(TAG, "retrieveLengthOfStringInNVM key = %s is of size %d", key, required_size);

        if (rc != ESP_OK)
        {
            if (rc == ESP_ERR_NVS_NOT_FOUND)
                ESP_LOGI(TAG, "VALUE NOT FOUND IN NVM");
            else
                ESP_LOGI(TAG, "Error in  retrieveLengthOfStringInNVM code = %s", esp_err_to_name(rc));
        }
    }
    if (showNVMDebug)
        ESP_LOGW(TAG, "retrieveLengthOfStringInNVM required_size is %d", required_size);
    return required_size;
}

bool System::retrieveStringWithKeyFromNVS(const char *key, char *value, size_t *valLength)
{
    esp_err_t rc;

    if (nvsHandle == 0)
    {
        ESP_LOGE(TAG, "You must openNVStorage() first!");
        return false;
    }
    else
    {
        if (showNVMDebug)
            ESP_LOGI(TAG, "retrieveStringWithKeyFromNVS requires %d size to store value of %s", *valLength, key);
        rc = nvs_get_str(nvsHandle, key, value, valLength);

        if (rc == ESP_OK)
        {
            if (showNVMDebug)
                ESP_LOGW(TAG, "Retrieved %s from NVS of %s", key, value); // Debug print statements
        }
        else
        {
            ESP_LOGE(TAG, "nvs_get_str failed for some reasone...");
            return false;
        }
    }

    return true;
}

void System::closeNVStorage(bool blnCommit)
{
    esp_err_t rc;

    if (nvsHandle != 0)
    {
        if (blnCommit)
        {
            rc = nvs_commit(nvsHandle);

            if (rc != ESP_OK)
                ESP_LOGI(TAG, "Error(%s) committing to NVS!", esp_err_to_name(rc));
        }
        nvs_close(nvsHandle);
        nvsHandle = 0;
    }
}
