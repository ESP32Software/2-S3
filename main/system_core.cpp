#include "system.hpp"

/* External Variables */
xSemaphoreHandle semSYSEntry = NULL;

bool blnSwitch1 = true;

System::System(void)
{
    // Set Object Debug Level
    // Note that this function can not raise log level above the level set using CONFIG_LOG_DEFAULT_LEVEL setting in menuconfig.
    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* SYS Request and Response */
    sysCmdRequestQue = xQueueCreate(1, sizeof(SYS_CmdRequest *)); // SYS <--  (with a size of 1, this behaves like a mailbox)
    ptrSYSCmdRequest = new SYS_CmdRequest();                      // We have only one structure for the incoming Request
    ptrSYSResponse = new SYS_Response();                          // and the outgoing Response

    vSemaphoreCreateBinary(semSYSEntry); // We DO have objects calling back during initialzation so do not lock up the Semaphore

    /* GPIO */
    initGPIOPins(); // Set up all our pin General Purpose Input Output pin definitions
    initGPIOTask(); // Assigning ISRs to pins and starting GPIO Task

    // At this point, all System fundementals are operating -- but we may have very slow items in the system
    // which take a long time to intitialize.  Start the long Initialization processes.
    initSysStep = SYS_INIT::Start;
    SysOp = SYS_OP::Init;
    xTaskCreate(runMarshaller, "SYS::Run", 1024 * 3, this, 5, &taskHandleSystemRun); // For periodic System tasks.
}

//
//
//
xTaskHandle System::getGenTaskHandle(void)
{
    return taskHandleSystemRun;
}

xTaskHandle System::getIOTTaskHandle(void)
{
    return taskHandleIOTRUN;
}
