/*
 * Copyright (C) Bosch Connected Devices and Solutions GmbH.
 * All Rights Reserved. Confidential.
 *
 * Distribution only to people who need to know this information in
 * order to do their job.(Need-to-know principle).
 * Distribution to persons outside the company, only if these persons
 * signed a non-disclosure agreement.
 * Electronic transmission, e.g. via electronic mail, must be made in
 * encrypted form.
 */
/*----------------------------------------------------------------------------*/

/**
 *  @file
 *
 *  @brief Configuration header for the AppController.c file.
 *
 */

/* header definition ******************************************************** */
#ifndef APPCONTROLLER_H_
#define APPCONTROLLER_H_

/* local interface declaration ********************************************** */
#include "XDK_Utils.h"

/* local type and macro definitions */

/* local module global variable declarations */

/* local inline function definitions */

/* local type and macro definitions */

/* local type and macro definitions */

/**
 * APP_CONTROLLER_BLE_DEVICE_NAME is the BLE device name.
 */
#define APP_CONTROLLER_BLE_DEVICE_NAME              "BCDS_Virtual_Sensor"

/**
 * Enable the high priority data service. Should not be used in conjunction with
 * the  sensor characteristic notifications.
 */
#define ENABLE_HIGH_PRIO_DATA_SERVICE               1

#if (ENABLE_HIGH_PRIO_DATA_SERVICE == 1)

/**
 * ENABLE_EXTERNAL_LEM_SENSOR is used to Enable/Disable the External LEM Sensor integration
 */
#define ENABLE_EXTERNAL_LEM_SENSOR                  1

/**
 * ENABLE_EXTERNAL_MAX_SENSOR is used to Enable/Disable the External temperature Sensor integration
 */
#define ENABLE_EXTERNAL_MAX_SENSOR                  1

#else /* (ENABLE_HIGH_PRIO_DATA_SERVICE == 1) */

/** Enable accelerometer sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_ACCELEROMETER_NOTIFICATIONS          1

/** Enable gyro sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_GYRO_NOTIFICATIONS                   1

/** Enable magnetormeter sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_MAGNETOMETER_NOTIFICATIONS           1

/** Enable environment sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS     1

/** Enable light sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_LIGHT_SENSOR_NOTIFICATIONS     1

/** Enable noise sensor characteristic notifications. Should not be used in conjunction with
 * the high priority data service
 */
#define ENABLE_NOISE_SENSOR_NOTIFICATIONS     1

#endif /* (ENABLE_HIGH_PRIO_DATA_SERVICE == 1) */

/**
 * APP_TEMPERATURE_OFFSET_CORRECTION is the Temperature sensor offset correction value (in mDegC).
 * Unused if APP_SENSOR_TEMP is false.
 * This is the Self heating, temperature correction factor.
 */
#define APP_TEMPERATURE_OFFSET_CORRECTION   (-3459)

/**
 * APP_CURRENT_RATED_TRANSFORMATION_RATIO is the current rated transformation ratio.
 * Unused if APP_SENSOR_CURRENT is false.
 * This will vary from one external LEM sensor to another.
 * @note :Rated transformation ratio is 44.44 for 75Ampere split core current transformer
 */
#define APP_CURRENT_RATED_TRANSFORMATION_RATIO      (44.44f)

/* local function prototype declarations */

/**
 * @brief Gives control to the Application controller.
 *
 * @param[in] cmdProcessorHandle
 * Handle of the main command processor which shall be used based on the application needs
 *
 * @param[in] param2
 * Unused
 */
void AppController_Init(void * cmdProcessorHandle, uint32_t param2);

#endif /* APPCONTROLLER_H_ */

/** ************************************************************************* */
