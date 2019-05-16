/* --------------------------------------------------------------------------- |
 * INCLUDES & DEFINES ******************************************************** |
 * -------------------------------------------------------------------------- */

/* own header files */
#include "XdkAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

/* system header files */
#include <stdio.h>

/* additional interface header files */
#include "BCDS_CmdProcessor.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "XDK_NoiseSensor.h"
#include "math.h"

/* --------------------------------------------------------------------------- |
 * HANDLES ******************************************************************* |
 * -------------------------------------------------------------------------- */

static CmdProcessor_T * AppCmdProcessor;/**< Handle to store the main Command processor handle to be used by run-time event driven threads */
xTimerHandle acousticHandle = NULL;

/* --------------------------------------------------------------------------- |
 * VARIABLES ***************************************************************** |
 * -------------------------------------------------------------------------- */

const float aku340ConversionRatio = pow(10,(-38/20));

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

float calcSoundPressure(float acousticRawValue){
    return (acousticRawValue/aku340ConversionRatio);
}

static void readAcousticSensor(xTimerHandle xTimer)
{
    (void) xTimer;

    float acousticData;

    if (RETCODE_OK == NoiseSensor_ReadRmsValue(&acousticData,10U)) {
        printf("Sound pressure: %f \r\n", calcSoundPressure(acousticData));
    }
}

static void AppControllerEnable(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    uint32_t timerBlockTime = UINT32_MAX;

    /* Enable necessary modules for the application and check their return values */
    if ( RETCODE_OK == NoiseSensor_Enable()) {
        xTimerStart(acousticHandle,timerBlockTime);
    }
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);
    Retcode_T retcode = RETCODE_OK;

    /* Setup the necessary modules required for the application */
    if (RETCODE_OK == NoiseSensor_Setup(22050U)) {
        uint32_t oneSecondDelay = UINT32_C(1000);
        uint32_t timerAutoReloadOn = UINT32_C(1);

        acousticHandle = xTimerCreate((const char *) "readAcousticSensor", oneSecondDelay,timerAutoReloadOn, NULL, readAcousticSensor);
    }

    retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, UINT32_C(0));
    if (RETCODE_OK != retcode)
    {
        printf("AppControllerSetup : Failed \r\n");
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
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
        assert(0); /* To provide LED indication for the user */
    }
}
