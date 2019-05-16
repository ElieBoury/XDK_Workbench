/* --------------------------------------------------------------------------- |
 * INCLUDES & DEFINES ******************************************************** |
 * -------------------------------------------------------------------------- */

#include "XdkAppInfo.h"
#undef BCDS_MODULE_ID  // [i] Module ID define before including Basics package
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

#include <stdio.h>
#include "BCDS_CmdProcessor.h"
#include "FreeRTOS.h"
#include "XdkSensorHandle.h"
#include "timers.h"
#include "BCDS_AbsoluteHumidity.h"

/* --------------------------------------------------------------------------- |
 * HANDLES ******************************************************************* |
 * -------------------------------------------------------------------------- */

static CmdProcessor_T * AppCmdProcessor;
xTimerHandle absoluteHumidityHandle = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readAbsoluteHumiditySensor(xTimerHandle xTimer)
{
    (void) xTimer;

    Retcode_T returnDataValue = RETCODE_FAILURE;

    /* Reading of the data */
    float getHumidityData = INT32_C(0);

    returnDataValue = AbsoluteHumidity_readValue(xdkHumiditySensor_Handle, &getHumidityData);

    if (returnDataValue == RETCODE_OK) {
        printf("Absolute humidity: %f g/m3 \n\r", getHumidityData);
    }
    else
    {
        printf("Reading of the absolute humidity data failed! \n\r");
    }
}

static void initAbsoluteHumiditySensor(void)
{
    Retcode_T absoluteHumidityInitReturnValue = RETCODE_FAILURE;

    absoluteHumidityInitReturnValue = AbsoluteHumidity_init(xdkHumiditySensor_Handle);

    if (absoluteHumidityInitReturnValue != RETCODE_OK) {
        printf("Initializing Absolute Humidity Sensor failed \n\r");
    }
}

/* --------------------------------------------------------------------------- |
 * BOOTING- AND SETUP FUNCTIONS ********************************************** |
 * -------------------------------------------------------------------------- */

static void AppControllerEnable(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    uint32_t timerBlockTime = UINT32_MAX;

    xTimerStart(absoluteHumidityHandle ,timerBlockTime);
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;
    uint32_t timerDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    // Setup of the necessary module
    initAbsoluteHumiditySensor();

    // Creation and start of the timer
    absoluteHumidityHandle = xTimerCreate((const char *) "readAbsoluteHumiditySensor", timerDelay, timerAutoReloadOn, NULL, readAbsoluteHumiditySensor);
    retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, UINT32_C(0));

    if (RETCODE_OK != retcode)
    {
        printf("AppControllerSetup : Failed \r\n");
        Retcode_RaiseError(retcode);
        assert(0);
    }
}

void AppController_Init(void * cmdProcessorHandle, uint32_t param2)
{
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;

    if (cmdProcessorHandle == NULL)
    {
        printf("AppController_Init : Command processor handle is NULL \r\n");
        retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_NULL_POINTER);
    }
    else
    {
        AppCmdProcessor = (CmdProcessor_T *) cmdProcessorHandle;
        retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, UINT32_C(0));
    }
    if (RETCODE_OK != retcode)
    {
        Retcode_RaiseError(retcode);
        assert(0);
    }
}
