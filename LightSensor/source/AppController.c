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

/* --------------------------------------------------------------------------- |
 * HANDLES ******************************************************************* |
 * -------------------------------------------------------------------------- */

static CmdProcessor_T * AppCmdProcessor;
xTimerHandle lightSensorHandle  = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readLightSensor(xTimerHandle xTimer)
{
    (void) xTimer;

    Retcode_T returnValue = RETCODE_FAILURE;
    uint32_t max44009 = UINT32_C(0);

        returnValue = LightSensor_readLuxData(xdkLightSensor_MAX44009_Handle, &max44009);

        if (RETCODE_OK == returnValue){
            printf("Light sensor data obtained in milli lux :%d \n\r",(unsigned int) max44009);
        }
}

static void initLightSensor(void)
{
    Retcode_T returnValue = RETCODE_FAILURE;
    Retcode_T returnBrightnessValue = RETCODE_FAILURE;
    Retcode_T returnIntegrationTimeValue = RETCODE_FAILURE;

    returnValue = LightSensor_init(xdkLightSensor_MAX44009_Handle);

    if ( RETCODE_OK != returnValue) {
        printf("MAX44009 Light Sensor initialization failed\n\r");
    }
    returnBrightnessValue = LightSensor_setBrightness(xdkLightSensor_MAX44009_Handle,LIGHTSENSOR_NORMAL_BRIGHTNESS);
    if (RETCODE_OK != returnBrightnessValue) {
        printf("Configuring brightness failed \n\r");
    }
    returnIntegrationTimeValue = LightSensor_setIntegrationTime(xdkLightSensor_MAX44009_Handle,LIGHTSENSOR_200MS);
    if (RETCODE_OK != returnIntegrationTimeValue) {
        printf("Configuring integration time failed \n\r");
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

    xTimerStart(lightSensorHandle,timerBlockTime);
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;
    uint32_t timerDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    // Setup of the necessary module
    initLightSensor();

    // Creation and start of the timer
    lightSensorHandle = xTimerCreate((const char *) "readAmbientLight", timerDelay, timerAutoReloadOn, NULL, readLightSensor);
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
