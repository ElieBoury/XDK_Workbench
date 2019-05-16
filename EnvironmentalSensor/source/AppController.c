/*----------------------------------------------------------------------------*/

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
xTimerHandle environmentalHandle = NULL;

/* --------------------------------------------------------------------------- |
 * EXECUTING FUNCTIONS ******************************************************* |
 * -------------------------------------------------------------------------- */

static void readEnvironmental(xTimerHandle xTimer)
{
    (void) xTimer;

    Retcode_T returnValue = RETCODE_FAILURE;

    /* read and print BME280 environmental sensor data */

    Environmental_Data_T bme280 = { INT32_C(0), UINT32_C(0), UINT32_C(0) };

    returnValue = Environmental_readData(xdkEnvironmental_BME280_Handle, &bme280);

    if ( RETCODE_OK == returnValue) {
        printf("BME280 Environmental Data : p =%ld Pa T =%ld mDeg h =%ld %%rh\n\r",
        (long int) bme280.pressure, (long int) bme280.temperature, (long int) bme280.humidity);
    }
}

// Function that initializes the Environmental sensor with the BME280 handler and with additional presettings
static void initEnvironmental(void)
{
    Retcode_T returnValue = RETCODE_FAILURE;
    Retcode_T returnOverSamplingValue = RETCODE_FAILURE;
    Retcode_T returnFilterValue = RETCODE_FAILURE;

    /* initialize environmental sensor */

    returnValue = Environmental_init(xdkEnvironmental_BME280_Handle);
    if ( RETCODE_OK != returnValue) {
        printf("BME280 Environmental Sensor initialization failed\n\r");
    }

    returnOverSamplingValue = Environmental_setOverSamplingPressure(xdkEnvironmental_BME280_Handle,ENVIRONMENTAL_BME280_OVERSAMP_2X);
    if (RETCODE_OK != returnOverSamplingValue) {
        printf("Configuring pressure oversampling failed \n\r");
    }

    returnFilterValue = Environmental_setFilterCoefficient(xdkEnvironmental_BME280_Handle,ENVIRONMENTAL_BME280_FILTER_COEFF_2);
    if (RETCODE_OK != returnFilterValue) {
        printf("Configuring pressure filter coefficient failed \n\r");
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

    xTimerStart(environmentalHandle,timerBlockTime);
}

static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);
    Retcode_T retcode = RETCODE_OK;

    uint32_t oneSecondDelay = UINT32_C(1000);
    uint32_t timerAutoReloadOn = UINT32_C(1);

    initEnvironmental();

    environmentalHandle = xTimerCreate((const char *) "readEnvironmental", oneSecondDelay,timerAutoReloadOn, NULL, readEnvironmental);

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

/** ************************************************************************* */
