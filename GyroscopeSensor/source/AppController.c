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
xTimerHandle gyroscopeHandle = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readGyroscope(xTimerHandle xTimer)
{
    (void) xTimer;

    Retcode_T returnValue = RETCODE_FAILURE;

    Gyroscope_XyzData_T bmg160 = {INT32_C(0), INT32_C(0), INT32_C(0)};

        memset(&bmg160, 0, sizeof(CalibratedGyro_DpsData_T));
        returnValue = Gyroscope_readXyzDegreeValue(xdkGyroscope_BMG160_Handle, &bmg160);

        if (RETCODE_OK == returnValue){
            printf("BMG160 Gyroscope Data: %10d mDeg[X] %10d mDeg[Y] %10d mDeg[Z]\n\r",
                (int) bmg160.xAxisData, (int) bmg160.yAxisData, (int) bmg160.zAxisData);
        }
}

static void initGyroscope(void)
{
    Retcode_T returnValue = RETCODE_FAILURE;
    Retcode_T returnBandwidthValue = RETCODE_FAILURE;
    Retcode_T returnRangeValue = RETCODE_FAILURE;

    returnValue = Gyroscope_init(xdkGyroscope_BMG160_Handle);

    if ( RETCODE_OK != returnValue) {
        printf("BMG160 Gyroscope initialization failed\n\r");
    }
    returnBandwidthValue = Gyroscope_setBandwidth(xdkGyroscope_BMG160_Handle, GYROSCOPE_BMG160_BANDWIDTH_116HZ);
    if (RETCODE_OK != returnBandwidthValue) {
        printf("Configuring bandwidth failed \n\r");
    }
    returnRangeValue = Gyroscope_setRange(xdkGyroscope_BMG160_Handle, GYROSCOPE_BMG160_RANGE_500s);
    if (RETCODE_OK != returnRangeValue) {
        printf("Configuring range failed \n\r");
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

    xTimerStart(gyroscopeHandle,timerBlockTime);
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;
    uint32_t timerDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    // Setup of the necessary module
    initGyroscope();

    // Creation and start of the timer
    gyroscopeHandle = xTimerCreate((const char *) "readGyroscope", timerDelay, timerAutoReloadOn, NULL, readGyroscope);
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
