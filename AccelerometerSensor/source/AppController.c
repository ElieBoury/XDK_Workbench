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
#include "BCDS_CalibratedAccel.h"

/* --------------------------------------------------------------------------- |
 * HANDLES ******************************************************************* |
 * -------------------------------------------------------------------------- */
static CmdProcessor_T * AppCmdProcessor;
xTimerHandle calibratedAccelerometerHandle = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readCalibratedAccelerometer(xTimerHandle xTimer)
{
    (void) xTimer;

    CalibratedAccel_Status_T calibrationAccuracy = CALIBRATED_ACCEL_UNRELIABLE;
    Retcode_T calibrationStatus = RETCODE_FAILURE;

    calibrationStatus = CalibratedAccel_getStatus(&calibrationAccuracy);

    if (calibrationAccuracy == CALIBRATED_ACCEL_HIGH && calibrationStatus == RETCODE_OK){
        Retcode_T returnDataValue = RETCODE_FAILURE;

        /* Reading of the data of the calibrated accelerometer */
        CalibratedAccel_XyzMps2Data_T getAccelMpsData = { INT32_C(0), INT32_C(0), INT32_C(0) };
        returnDataValue = CalibratedAccel_readXyzMps2Value(&getAccelMpsData);

        if (returnDataValue == RETCODE_OK){
            printf("Calibrated acceleration: %10f m/s2[X] %10f m/s2[Y] %10f m/s2[Z]\n\r",
                        (float) getAccelMpsData.xAxisData, (float) getAccelMpsData.yAxisData, (float) getAccelMpsData.zAxisData);
        }
    }
}

static void initCalibratedAccelerometer(void)
{
    Retcode_T calibratedAccelInitReturnValue = RETCODE_FAILURE;
    calibratedAccelInitReturnValue = CalibratedAccel_init(xdkCalibratedAccelerometer_Handle);

    if (calibratedAccelInitReturnValue != RETCODE_OK) {
        printf("Initializing Calibrated Accelerometer failed \n\r");
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

    xTimerStart(calibratedAccelerometerHandle,timerBlockTime);
}


static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = RETCODE_OK;
    uint32_t timerDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    // Setup of the necessary module
    initCalibratedAccelerometer();

    // Creation and start of the timer
    calibratedAccelerometerHandle = xTimerCreate((const char *) "readCalibratedAccelerometer", timerDelay, timerAutoReloadOn, NULL, readCalibratedAccelerometer);
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
