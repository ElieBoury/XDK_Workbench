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
#include "FreeRTOS.h"
#include "BCDS_CmdProcessor.h"
#include "BCDS_Assert.h"

#include "XdkSensorHandle.h"

#include "task.h"
#include "timers.h"

/* --------------------------------------------------------------------------- |
 * HANDLES ******************************************************************* |
 * -------------------------------------------------------------------------- */

static CmdProcessor_T * AppCmdProcessor;
xTimerHandle magnetometerHandle = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readMagnetometer(xTimerHandle xTimer){

    (void) xTimer;
    Retcode_T returnValue = RETCODE_FAILURE;

    /* read and print BMM150 magnetometer data */

    Magnetometer_XyzData_T bmm150 = {INT32_C(0), INT32_C(0), INT32_C(0), INT32_C(0)};

    returnValue = Magnetometer_readXyzTeslaData(xdkMagnetometer_BMM150_Handle, &bmm150);

    if (RETCODE_OK == returnValue) {
    printf("BMM150 Magnetic Data: x =%ld mT y =%ld mT z =%ld mT \n\r",
          (long int) bmm150.xAxisData, (long int) bmm150.yAxisData, (long int) bmm150.zAxisData);
    }
}

static void initMagnetometer(void){

    Retcode_T returnValue = RETCODE_FAILURE;
    Retcode_T returnDataRateValue = RETCODE_FAILURE;
    Retcode_T returnPresetModeValue = RETCODE_FAILURE;

    /* initialize magnetometer */

    returnValue = Magnetometer_init(xdkMagnetometer_BMM150_Handle);

    if(RETCODE_OK != returnValue){
        printf("BMM150 Magnetometer initialization failed \n\r");
    }

    returnDataRateValue = Magnetometer_setDataRate(xdkMagnetometer_BMM150_Handle,
            MAGNETOMETER_BMM150_DATARATE_10HZ);
    if (RETCODE_OK != returnDataRateValue) {
    printf("Configuring data rate failed \n\r");
    }
    returnPresetModeValue = Magnetometer_setPresetMode(xdkMagnetometer_BMM150_Handle,
            MAGNETOMETER_BMM150_PRESETMODE_REGULAR);
    if (RETCODE_OK != returnPresetModeValue) {
    printf("Configuring preset mode failed \n\r");
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

    xTimerStart(magnetometerHandle,timerBlockTime);
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);
    Retcode_T retcode = RETCODE_OK;


    uint32_t oneSecondDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    initMagnetometer();

    magnetometerHandle = xTimerCreate((const char *) "readMagnetometer", oneSecondDelay,timerAutoReloadOn, NULL, readMagnetometer);


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
