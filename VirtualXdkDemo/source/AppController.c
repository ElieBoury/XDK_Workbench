/*
* Licensee agrees that the example code provided to Licensee has been developed and released by Bosch solely as an example to be used as a potential reference for application development by Licensee. 
* Fitness and suitability of the example code for any use within application developed by Licensee need to be verified by Licensee on its own authority by taking appropriate state of the art actions and measures (e.g. by means of quality assurance measures).
* Licensee shall be responsible for conducting the development of its applications as well as integration of parts of the example code into such applications, taking into account the state of the art of technology and any statutory regulations and provisions applicable for such applications. Compliance with the functional system requirements and testing there of (including validation of information/data security aspects and functional safety) and release shall be solely incumbent upon Licensee. 
* For the avoidance of doubt, Licensee shall be responsible and fully liable for the applications and any distribution of such applications into the market.
* 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are 
* met:
* 
*     (1) Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer. 
* 
*     (2) Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.  
*     
*     (3)The name of the author may not be used to
*     endorse or promote products derived from this software without
*     specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*/
/*----------------------------------------------------------------------------*/

/**
 * @ingroup APPS_LIST
 *
 * @defgroup VIRTUAL_XDK_DEMO VirtualXdkDemo
 * @{
 *
 * @brief Demo Application for Virtual XDK mobile application
 *
 * @details In this application, XDK is presented in a playful fashion on a mobile device that is connected to XDK via BLE.
 * Interacting with the physical XDK will manipulate the virtual XDK.
 *
 * The web links to get the Android/iOS App is given below:<br>
 * <b> Android App: </b> <a href="https://play.google.com/store/apps/details?id=com.bosch.VirtualXdk">Click_Here</a> <br>
 * <b> iOS App: </b> <a href="https://itunes.apple.com/de/app/virtual-xdk/id1120568347?mt=8">Click_Here</a> <br>
 * For further documentation please refer the page \ref XDK_VIRTUAL_XDK_APP_USER_GUIDE
 *
 * @file
 **/

/* module includes ********************************************************** */

/* own header files */
#include "XdkAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

/* own header files */
#include "AppController.h"

/* system header files */
#include <stdio.h>

/* additional interface header files */
#include <math.h>
#include "XdkVersion.h"
#include "XDK_Button.h"
#include "XDK_BLE.h"
#include "XDK_ExternalSensor.h"
#include "XDK_LED.h"
#include "XDK_Sensor.h"
#include "XDK_VirtualSensor.h"
#include "XDK_Storage.h"
#include "XDK_Utils.h"
#include "BCDS_BSP_Board.h"
#include "BCDS_CmdProcessor.h"
#include "BCDS_SDCard_Driver.h"
#include "XDK_NoiseSensor.h"
#include "BCDS_BSP_Max31865.h"
#include "BCDS_Max31865.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "AdcCentralConfig.h"
#include "XDK_Lem.h"
/* constant definitions ***************************************************** */

#define LEM_SENSOR_DATA_VOLTS_TO_MICROVOLTS                 UINT32_C(1000000)
#define NOISE_SENSOR_DATA_VOLTS_TO_MILLIVOLTS               1000.0f

#define BLE_SEND_TIMEOUT_IN_MS                              UINT32_C(1000)
#define NOISE_RMS_ADC_SAMPLES                               UINT32_C(30)
#define NOISE_RMS_AVG_RANGE_IN_MV                           0.005f
#define NOISE_SAMPLING_FREQUENCY                            (20000UL)            /**< ADC sampling frequency in hertz (Hz)*/

/* Step1 LEDs are initialized and step2 other modules are initialized */
#define APP_CONTROLLER_STEP1                            UINT32_C(0)
#define APP_CONTROLLER_STEP2                            UINT32_C(1)

ExternalSensor_Target_T ExtHwConnected = XDK_EXTERNAL_INVALID;

/* local variables ********************************************************** */
static void AppControllerSetup(void * param1, uint32_t param2);
static void AppControllerBleDataRxCB(uint8_t *rxBuffer, uint8_t rxDataLength, void * param);

static bool lemHwConnectStatus = false;
static bool extTempHwConnectStatus = false;
static uint8_t ModelNumberCharacteristicValue[] = "XDK110";
static uint8_t ManufacturerCharacteristicValue[] = "Robert Bosch GmbH";
static uint8_t SoftwareRevisionCharacteristicValue[10U] = { 0 }; /* 10U is a random number. @todo - Provide a precise max length. */
static BLE_Setup_T BLESetupInfo =
        {
                .DeviceName = APP_CONTROLLER_BLE_DEVICE_NAME,
                .IsMacAddrConfigured = false,
                .MacAddr = 0UL,
                .Service = BLE_XDK_SENSOR_SERVICES,
                .IsDeviceCharacteristicEnabled = true,
                .CharacteristicValue =
                        {
                                .ModelNumber = &ModelNumberCharacteristicValue[0],
                                .Manufacturer = &ManufacturerCharacteristicValue[0],
                                .SoftwareRevision = &SoftwareRevisionCharacteristicValue[0]
                        },
                .DataRxCB = AppControllerBleDataRxCB,
                .CustomServiceRegistryCB = NULL,
        };/**< BLE setup parameters */

#if ENABLE_HIGH_PRIO_DATA_SERVICE

static Sensor_Setup_T SensorSetup =
        {
                .CmdProcessorHandle = NULL,
                .Enable =
                        {
                                .Accel = true,
                                .Mag = true,
                                .Gyro = true,
                                .Humidity = true,
                                .Temp = true,
                                .Pressure = true,
                                .Light = true,
                                .Noise = false, /* Noise sensor is read directly from Noise Sensor APIs. Hence it must be disabled in SensorSetup */
                        },
                .Config =
                        {
                                .Accel =
                                        {
                                                .Type = SENSOR_ACCEL_BMA280,
                                                .IsRawData = false,
                                                .IsInteruptEnabled = false,
                                                .Callback = NULL,
                                        },
                                .Gyro =
                                        {
                                                .Type = SENSOR_GYRO_BMG160,
                                                .IsRawData = false,
                                        },
                                .Mag =
                                        {
                                                .IsRawData = false
                                        },
                                .Light =
                                        {
                                                .IsInteruptEnabled = false,
                                                .Callback = NULL,
                                        },
                                .Temp =
                                        {
                                                .OffsetCorrection = APP_TEMPERATURE_OFFSET_CORRECTION,
                                        },
                        },
        };/**< Sensor setup parameters */

#else /* ENABLE_HIGH_PRIO_DATA_SERVICE */

static Sensor_Setup_T SensorSetup =
{
    .CmdProcessorHandle = NULL,
    .Enable =
    {
        .Accel = ENABLE_ACCELEROMETER_NOTIFICATIONS,
        .Mag = ENABLE_MAGNETOMETER_NOTIFICATIONS,
        .Gyro = ENABLE_GYRO_NOTIFICATIONS,
        .Humidity = ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS,
        .Temp = ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS,
        .Pressure = ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS,
        .Light = ENABLE_LIGHT_SENSOR_NOTIFICATIONS,
        .Noise = false,
    },
    .Config =
    {
        .Accel =
        {
            .Type = SENSOR_ACCEL_BMA280,
            .IsRawData = false,
            .IsInteruptEnabled = false,
            .Callback = NULL,
        },
        .Gyro =
        {
            .Type = SENSOR_GYRO_BMG160,
            .IsRawData = false,
        },
        .Mag =
        {
            .IsRawData = false
        },
        .Light =
        {
            .IsInteruptEnabled = false,
            .Callback = NULL,
        },
        .Temp =
        {
            .OffsetCorrection = APP_TEMPERATURE_OFFSET_CORRECTION,
        },
    },
};/**< Sensor setup parameters */

#define ENABLE_EXTERNAL_LEM_SENSOR  false/**< Disabled LEM since it is not used during sensor data notification */

#define ENABLE_EXTERNAL_MAX_SENSOR  true/**< External Temperature Sensor is enabled as a hot-plug to super seed internal temperature sensor when connected */

#endif /* ENABLE_HIGH_PRIO_DATA_SERVICE */

static ExternalSensor_Setup_T ExternalSensorSetup =
        {
                .CmdProcessorHandle = NULL,
                .Enable =
                        {
                                .LemCurrent = false,
                                .LemVoltage = ENABLE_EXTERNAL_LEM_SENSOR, /* Is false when ENABLE_HIGH_PRIO_DATA_SERVICE is disabled */
                                .MaxTemp = ENABLE_EXTERNAL_MAX_SENSOR,
                                .MaxResistance = false,
                        },
                .LemConfig =
                        {
                                .CurrentRatedTransformationRatio = APP_CURRENT_RATED_TRANSFORMATION_RATIO,
                        },
                .Max31865Config =
                        {
                                .TempType = SENSOR_TEMPERATURE_PT1000,
                        },
        };/**< External Sensor setup parameters */

const VirtualSensor_Enable_T VirtualSensors =
        {
                .Rotation = true,
                .Compass = false,
                .AbsoluteHumidity = false,
                .CalibratedAccel = false,
                .CalibratedGyro = false,
                .CalibratedMag = false,
                .Gravity = false,
                .StepCounter = false,
                .FingerPrint = false,
                .LinearAccel = false,
                .Gesture = false
        };

static void Button1EventCallback(ButtonEvent_T buttonEvent);
static void Button2EventCallback(ButtonEvent_T buttonEvent);

static Button_Setup_T ButtonSetup =
        {
                .CmdProcessorHandle = NULL,
                .InternalButton1isEnabled = true,
                .InternalButton2isEnabled = true,
                .InternalButton1Callback = Button1EventCallback,
                .InternalButton2Callback = Button2EventCallback,
        };/**< Button setup parameters */

static Storage_Setup_T StorageSetup =
        {
                .SDCard = true,
                .WiFiFileSystem = false,
        };/**< Storage setup parameters */

static uint16_t SampleCounter = 0; /**< counter used for toggling low priority data messages. @todo - use a bool */
static uint8_t IsUseBuiltInSensorFusion = 1;
static uint8_t Button1Status = 0;
static uint8_t Button2Status = 0;
static bool SensorDataStartSampling = false;

#if ENABLE_HIGH_PRIO_DATA_SERVICE
static uint32_t DataSampleRateInMs = 200; /**< variable to store the sensor sample time */
#else /* ENABLE_HIGH_PRIO_DATA_SERVICE */
static uint32_t DataSampleRateInMs = 2000; /**< variable to store the sensor sample time */
#endif /* ENABLE_HIGH_PRIO_DATA_SERVICE */

static CmdProcessor_T * AppCmdProcessor;/**< Handle to store the main Command processor handle to be used by run-time event driven threads */

static xTaskHandle AppControllerHandle = NULL;/**< OS thread handle for Application controller to be used by run-time blocking threads */
static xTaskHandle NoiseSensorHandle = NULL;/**< OS thread handle for noise sensor to be used by run-time blocking threads */
static volatile float NoiseRmsVoltage = 0.0f;

/* global variables ********************************************************* */

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

static void Button1EventCallback(ButtonEvent_T buttonEvent)
{
    if (BUTTON_EVENT_PRESSED == buttonEvent)
    {
        Button1Status = true;
    }
    else
    {
        Button1Status = false;
    }
}

static void Button2EventCallback(ButtonEvent_T buttonEvent)
{
    if (BUTTON_EVENT_PRESSED == buttonEvent)
    {
        Button2Status = true;
    }
    else
    {
        Button2Status = false;
    }
}

static void AppControllerBleDataRxCB(uint8_t *rxBuffer, uint8_t rxDataLength, void * param)
{
    BCDS_UNUSED(rxDataLength);

    SensorServices_Info_T sensorServiceInfo = *(SensorServices_Info_T*) param;
    if ((int) BLE_CONTROL_NODE == sensorServiceInfo.sensorServicesType)
    {
        if ((int) CONTROL_NODE_START_SAMPLING == sensorServiceInfo.sensorServicesContent)
        {
            SensorDataStartSampling = true;
        }
        else if ((int) CONTROL_NODE_CHANGE_SAMPLING_RATE == sensorServiceInfo.sensorServicesContent)
        {
            uint32_t receivedSamplingRate = 0;

            uint8_t receiveBuffer[4];

            memset(receiveBuffer, 0, (sizeof(receiveBuffer) / sizeof(uint8_t)));

            receiveBuffer[0] = *(rxBuffer);
            receiveBuffer[1] = *(rxBuffer + 1);
            receiveBuffer[2] = *(rxBuffer + 2);
            receiveBuffer[3] = *(rxBuffer + 3);

            receivedSamplingRate = receiveBuffer[3];
            receivedSamplingRate = (receivedSamplingRate << 8) + receiveBuffer[2];
            receivedSamplingRate = (receivedSamplingRate << 8) + receiveBuffer[1];
            receivedSamplingRate = (receivedSamplingRate << 8) + receiveBuffer[0];

            printf("AppControllerBleDataRxCB : New sampling rate: %d \r\n", (int) receivedSamplingRate);

            if ((receivedSamplingRate < 200) || (receivedSamplingRate > 5000))
            {
                receivedSamplingRate = 200;
            }

            DataSampleRateInMs = receivedSamplingRate;
        }
        else if ((int) CONTROL_NODE_REBOOT == sensorServiceInfo.sensorServicesContent)
        {
            printf("AppControllerBleDataRxCB : Reset request received from App! \r\n");
            BSP_Board_SoftReset();
        }
        else if ((int) CONTROL_NODE_USE_SENSOR_FUSION == sensorServiceInfo.sensorServicesContent)
        {
            IsUseBuiltInSensorFusion = *rxBuffer;
        }
    }
}

/**
 * @brief Convert Decimal/Hex value to String Format to display Version Information.
 *
 * @param[in] data - Number which needs to be convert into string to display in Decimal instead of HEX values.
 *
 * @param[in/out] *str - pointer to Store/write the Version number in String Format.
 *
 */
static uint8_t * ConvertDecimalToSTring(uint8_t data, uint8_t * str)
{
    uint8_t arr[3] = { 0, 0, 0 };
    uint8_t noOfDigits = 0x00;
    uint8_t index = 0x00;
    do
    {
        arr[index] = data % 10;
        data = data / 10;
        index++;
        noOfDigits++;
    } while (data != (0x00));

    for (int8_t i = (noOfDigits - 1); (i >= 0); i--)
    {
        *str = arr[i] + 0x30;
        str++;
    }
    return str;
}

/**
 * @brief This function is to convert the integer data into a string data
 *
 * @param[in] xdkVersion - 32 bit value containing the XDK SW version information
 *
 * @param[in/out] str - pointer to the parsed version string
 *
 */
static void ConvertInteger32ToVersionString(uint32_t xdkVersion, uint8_t* str)
{
    uint8_t shiftValue = UINT8_C(16);
    uint8_t byteVar = 0x00;
    *str = 'v';
    for (uint8_t i = 0; i <= 2; i++)
    {
        str++;
        byteVar = ((xdkVersion >> shiftValue) & 0xFF);
        str = ConvertDecimalToSTring(byteVar, str);
        *str = '.';
        shiftValue = shiftValue - 8;
    }
    *str = '\0';
}

/**
 * @brief This function to Get the Firmware Version by combining the XDK SW Release (i.e., Workbench Release) version (i.e., 3.2.0)
 */
static void GetFwVersionInfo(void)
{
    uint32_t xdkVersion;
    xdkVersion = XdkVersion_GetVersion();
    ConvertInteger32ToVersionString(xdkVersion, SoftwareRevisionCharacteristicValue);
}

static Retcode_T CurrentExternalHWConnected(ExternalSensor_Target_T * currentHw)
{
    Retcode_T retcode = RETCODE_OK;
    bool lemHwStatus = false;
    bool externalTempHwStatus = false;

    if (NULL == currentHw)
    {
        return (RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_NULL_POINTER));
    }
    *currentHw = XDK_EXTERNAL_INVALID;
    retcode = ExternalSensor_IsAvailable(XDK_EXTERNAL_LEM, &lemHwStatus);
    if ((RETCODE_OK == retcode) && (true == lemHwStatus))
    {
        *currentHw = XDK_EXTERNAL_LEM;
    }
    else
    {
        retcode = ExternalSensor_IsAvailable(XDK_EXTERNAL_MAX31865, &externalTempHwStatus);
        if ((RETCODE_OK == retcode) && (true == externalTempHwStatus))
        {
            *currentHw = XDK_EXTERNAL_MAX31865;
        }
    }
    return retcode;
}

static void HandleExternalHwHotPlug(ExternalSensor_Target_T * connectedHw)
{
    ExternalSensor_Target_T newHwState = XDK_EXTERNAL_INVALID;
    Retcode_T retcode = RETCODE_OK;

    if (NULL == connectedHw)
    {
        printf("NULL pointer given \n\r");
        return;
    }
    retcode = CurrentExternalHWConnected(&newHwState);
    if (RETCODE_OK != retcode)
    {
        printf("Error in Getting the external hw status \n\r");
    }
    *connectedHw = newHwState;
    switch (ExtHwConnected)
    {
    case XDK_EXTERNAL_LEM:
        if (XDK_EXTERNAL_INVALID == newHwState)
        {
            retcode = Lem_Disable();
            if (RETCODE_OK == retcode)
            {
                retcode = ExternalSensor_HwStatusPinInit();
            }
            if (RETCODE_OK == retcode)
            {
                ExtHwConnected = XDK_EXTERNAL_INVALID;
            }
            else
            {
                printf("Error in configuring ExtPin init \n\r");
            }
        }
        break;
    case XDK_EXTERNAL_MAX31865:
        if (XDK_EXTERNAL_INVALID == newHwState)
        {
            retcode = MAX31865_Disconnect();
            if (RETCODE_OK == retcode)
            {
                retcode = ExternalSensor_HwStatusPinInit();
            }
            if (RETCODE_OK == retcode)
            {
                ExtHwConnected = XDK_EXTERNAL_INVALID;
            }
            else
            {
                printf("Error in configuring ExtPin init \n\r");
            }
        }
        break;
    case XDK_EXTERNAL_INVALID:
        if (XDK_EXTERNAL_MAX31865 == newHwState)
        {
            retcode = ExternalSensor_HwStatusPinDeInit();
            if (RETCODE_OK == retcode)
            {
                retcode = MAX31865_Connect();
            }
            if (RETCODE_OK == retcode)
            {
                ExtHwConnected = XDK_EXTERNAL_MAX31865;
            }
            else
            {
                printf("Error in configuring Max external Hw \n\r");
            }
        }
        else if (XDK_EXTERNAL_LEM == newHwState)
        {
            retcode = ExternalSensor_HwStatusPinDeInit();
            if (RETCODE_OK == retcode)
            {
                retcode = Lem_Enable();
            }
            if (RETCODE_OK == retcode)
            {
                ExtHwConnected = XDK_EXTERNAL_LEM;
            }
            else
            {
                printf("Error in configuring LEM external Hw \n\r");
            }
        }
        break;
    }
}

#if ENABLE_HIGH_PRIO_DATA_SERVICE

static Retcode_T SampleAndNotifyHighPriorityData(void)
{
    Retcode_T retcode = RETCODE_OK;
    SensorServices_Info_T ServiceInfo;
    Sensor_Value_T sensorValue;
    float maxTemp = 0.0f;
    float lemVoltage = 0.0f;
    VirtualSensor_DataType_T rotationData;
    uint32_t RmsuVolt = 0;
    uint32_t noiseMVolt = 0;
    uint8_t sendBuffer[20];
    uint8_t ledYellowStatus = 0;
    uint8_t ledOrangeStatus = 0;
    uint8_t ledRedStatus = 0;
    bool lemSensorStatus = false;
    ExternalSensor_Target_T newHwType;

    memset(&sensorValue, 0x00, sizeof(sensorValue));

    HandleExternalHwHotPlug(&newHwType);

    retcode = Sensor_GetData(&sensorValue);

    if ((RETCODE_OK == retcode) && ((ExternalSensorSetup.Enable.LemVoltage) && (XDK_EXTERNAL_LEM == newHwType)))
    {
        lemSensorStatus = true;
        retcode = ExternalSensor_GetLemData(NULL, &lemVoltage);
    }
    if ((RETCODE_OK == retcode) && (ExternalSensorSetup.Enable.MaxTemp && (XDK_EXTERNAL_MAX31865 == newHwType)))
    {
        retcode = ExternalSensor_GetMax31865Data(&maxTemp, NULL);
        sensorValue.Temp = (uint32_t) (maxTemp * 1000); /* Over write the internal temperature sensor reading */
    }
    if (RETCODE_OK == retcode)
    {
        retcode = LED_Status(LED_INBUILT_YELLOW, &ledYellowStatus);
    }
    if (RETCODE_OK == retcode)
    {
        retcode = LED_Status(LED_INBUILT_RED, &ledRedStatus);
    }
    if (RETCODE_OK == retcode)
    {
        retcode = LED_Status(LED_INBUILT_ORANGE, &ledOrangeStatus);
    }
    if (RETCODE_OK == retcode)
    {
        memset(sendBuffer, 0, (sizeof(sendBuffer) / sizeof(uint8_t)));

        if (!IsUseBuiltInSensorFusion)
        {
            sendBuffer[0] = (uint8_t) sensorValue.Accel.X;
            sendBuffer[1] = (uint8_t) (sensorValue.Accel.X >> 8);
            sendBuffer[2] = (uint8_t) sensorValue.Accel.Y;
            sendBuffer[3] = (uint8_t) (sensorValue.Accel.Y >> 8);
            sendBuffer[4] = (uint8_t) sensorValue.Accel.Z;
            sendBuffer[5] = (uint8_t) (sensorValue.Accel.Z >> 8);

            sendBuffer[6] = (uint8_t) sensorValue.Gyro.X;
            sendBuffer[7] = (uint8_t) (sensorValue.Gyro.X >> 8);
            sendBuffer[8] = (uint8_t) sensorValue.Gyro.Y;
            sendBuffer[9] = (uint8_t) (sensorValue.Gyro.Y >> 8);
            sendBuffer[10] = (uint8_t) sensorValue.Gyro.Z;
            sendBuffer[11] = (uint8_t) (sensorValue.Gyro.Z >> 8);
#if DEBUG_LOGGING
            printf("SampleAndNotifyHighPriorityData : accelData x: %d \r\n", (uint) sensorValue.Accel.X);
            printf("SampleAndNotifyHighPriorityData : accelData y: %d \r\n", (uint) sensorValue.Accel.Y);
            printf("SampleAndNotifyHighPriorityData : accelData z: %d \r\n", (uint) sensorValue.Accel.Z);

            printf("SampleAndNotifyHighPriorityData : gyroData x: %d \r\n", (uint) sensorValue.Gyro.X);
            printf("SampleAndNotifyHighPriorityData : gyroData y: %d \r\n", (uint) sensorValue.Gyro.Y);
            printf("SampleAndNotifyHighPriorityData : gyroData z: %d \r\n", (uint) sensorValue.Gyro.Z);
#endif
        }
        else
        {
            retcode = VirtualSensor_GetRotationData(&rotationData, ROTATION_QUATERNION);
            if (RETCODE_OK == retcode)
            {
#if DEBUG_LOGGING
                printf("Rotation - Quaternion : %3.2f %3.2f %3.2f %3.2f\n\r",
                        rotationData.RotationQuaternion.W, rotationData.RotationQuaternion.X, rotationData.RotationQuaternion.Y, rotationData.RotationQuaternion.Z);
#endif
            }
            else
            {
                printf("Rotation Data Value Read failed \n\r");
            }
            memcpy(sendBuffer, (&rotationData.RotationQuaternion.W), 4);
            memcpy(sendBuffer + 4, (&rotationData.RotationQuaternion.X), 4);
            memcpy(sendBuffer + 8, (&rotationData.RotationQuaternion.Y), 4);
            memcpy(sendBuffer + 12, (&rotationData.RotationQuaternion.Z), 4);
        }

        ServiceInfo.sensorServicesContent = (uint8_t) HIGH_DATA_RATE_HIGH_PRIO;
        ServiceInfo.sensorServicesType = (uint8_t) BLE_HIGH_DATA_RATE;
        retcode = BLE_SendData((uint8_t*) sendBuffer, 20, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }

    if (RETCODE_OK == retcode)
    {
        memset(sendBuffer, 0, (sizeof(sendBuffer) / sizeof(uint8_t)));
        if (SampleCounter % 2)
        {
#if DEBUG_LOGGING
            printf("SampleAndNotifyHighPriorityData : Message 1 \r\n");
#endif
            sendBuffer[0] = (uint8_t) 0x01; //message id 1

            sendBuffer[1] = (uint8_t) (sensorValue.Light);
            sendBuffer[2] = (uint8_t) (sensorValue.Light >> 8);
            sendBuffer[3] = (uint8_t) (sensorValue.Light >> 16);
            sendBuffer[4] = (uint8_t) (sensorValue.Light >> 24);

            /* This byte is reserved and unused */
            sendBuffer[5] = 0x00;

            sendBuffer[6] = (uint8_t) sensorValue.Pressure;
            sendBuffer[7] = (uint8_t) (sensorValue.Pressure >> 8);
            sendBuffer[8] = (uint8_t) (sensorValue.Pressure >> 16);
            sendBuffer[9] = (uint8_t) (sensorValue.Pressure >> 24);

            sendBuffer[10] = (uint8_t) sensorValue.Temp;
            sendBuffer[11] = (uint8_t) ((uint32_t) sensorValue.Temp >> 8);
            sendBuffer[12] = (uint8_t) ((uint32_t) sensorValue.Temp >> 16);
            sendBuffer[13] = (uint8_t) ((uint32_t) sensorValue.Temp >> 24);

            sendBuffer[14] = (uint8_t) sensorValue.RH;
            sendBuffer[15] = (uint8_t) (sensorValue.RH >> 8);
            sendBuffer[16] = (uint8_t) (sensorValue.RH >> 16);
            sendBuffer[17] = (uint8_t) (sensorValue.RH >> 24);

            if (SDCARD_INSERTED == SDCardDriver_GetDetectStatus())
            {
                sendBuffer[18] = 0x01;
            }
            else
            {
                sendBuffer[18] = 0x00;
            }

            sendBuffer[19] |= ((Button1Status) << 0);
            sendBuffer[19] |= ((Button2Status) << 1);
        }
        else
        {
            sendBuffer[0] = (uint8_t) 0x02; //message id 2

            sendBuffer[1] = (uint8_t) sensorValue.Mag.X;
            sendBuffer[2] = (uint8_t) (sensorValue.Mag.X >> 8);
            sendBuffer[3] = (uint8_t) sensorValue.Mag.Y;
            sendBuffer[4] = (uint8_t) (sensorValue.Mag.Y >> 8);
            sendBuffer[5] = (uint8_t) sensorValue.Mag.Z;
            sendBuffer[6] = (uint8_t) (sensorValue.Mag.Z >> 8);
            sendBuffer[7] = (uint8_t) (sensorValue.Mag.R);
            sendBuffer[8] = (uint8_t) (sensorValue.Mag.R >> 8);

            sendBuffer[9] |= ((ledYellowStatus) << 0);
            sendBuffer[9] |= ((ledOrangeStatus) << 1);
            sendBuffer[9] |= ((ledRedStatus) << 2);

            if (ExternalSensorSetup.Enable.LemVoltage)
            {
                sendBuffer[14] = lemSensorStatus;
                /* If LEM sensor Connected then only populating the sensor values */
                if (lemSensorStatus)
                {
                    /* converting Volts to Micro Volts */
                    RmsuVolt = (uint32_t) (lemVoltage * LEM_SENSOR_DATA_VOLTS_TO_MICROVOLTS);
                    sendBuffer[10] = RmsuVolt & 0xFF;
                    sendBuffer[11] = (RmsuVolt >> 8) & 0xFF;
                    sendBuffer[12] = (RmsuVolt >> 16) & 0xFF;
                    sendBuffer[13] = (RmsuVolt >> 24) & 0xFF;
                }
            }

            noiseMVolt = (uint32_t) (NoiseRmsVoltage * NOISE_SENSOR_DATA_VOLTS_TO_MILLIVOLTS);
#if DEBUG_LOGGING
            printf("Noise sensor Avg RMS : %d \n\r", (int) noiseMVolt);
#endif /* DEBUG_LOGGING */
            sendBuffer[15] = noiseMVolt & 0xFF;
            sendBuffer[16] = (noiseMVolt >> 8) & 0xFF;
            sendBuffer[17] = (noiseMVolt >> 16) & 0xFF;
            sendBuffer[18] = (noiseMVolt >> 24) & 0xFF;
        }
        ServiceInfo.sensorServicesContent = (uint8_t) HIGH_DATA_RATE_LOW_PRIO;
        ServiceInfo.sensorServicesType = (uint8_t) BLE_HIGH_DATA_RATE;
        retcode = BLE_SendData((uint8_t*) sendBuffer, 20, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    return retcode;
}

#else /* ENABLE_HIGH_PRIO_DATA_SERVICE */

static Retcode_T SampleAndNotifySensorData(void)
{
    Retcode_T retcode = RETCODE_OK;
    SensorServices_Info_T ServiceInfo;
    Sensor_Value_T sensorValue;
    uint32_t noiseMVolt = 0;

    retcode = Sensor_GetData(&sensorValue);

#if ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS
    float maxTemp = 0.0f;
    bool max31865SensorStatus = false;

    if ((RETCODE_OK == retcode) && (ExternalSensorSetup.Enable.MaxTemp))
    {
        retcode = ExternalSensor_IsAvailable(XDK_EXTERNAL_MAX31865, &max31865SensorStatus);
        if ((RETCODE_OK == retcode) && (max31865SensorStatus))
        {
            retcode = ExternalSensor_GetMax31865Data(&maxTemp, NULL);
            sensorValue.Temp = (uint32_t) maxTemp; /* Over write the internal temperature sensor reading */
        }
    }
#endif /* ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS */

#if ENABLE_ACCELEROMETER_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_X;
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_ACCELEROMETER;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Accel.X, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }

    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_Y;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Accel.Y, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }

    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_Z;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Accel.Z, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_ACCELEROMETER_NOTIFICATIONS */

#if ENABLE_GYRO_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_GYRO;
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_X;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Gyro.X, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_Y;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Gyro.Y, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_AXIS_Z;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Gyro.Z, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_GYRO_NOTIFICATIONS */

#if ENABLE_LIGHT_SENSOR_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_LIGHT;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Light, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_LIGHT_SENSOR_NOTIFICATIONS */

#if ENABLE_NOISE_SENSOR_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_NOISE;
        noiseMVolt = (uint32_t) (NoiseRmsVoltage * NOISE_SENSOR_DATA_VOLTS_TO_MILLIVOLTS);
        retcode = BLE_SendData((uint8_t *) &noiseMVolt, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_NOISE_SENSOR_NOTIFICATIONS */

#if ENABLE_MAGNETOMETER_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_MAGNETOMETER_AXIS_X;
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_MAGNETOMETER;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Mag.X, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_MAGNETOMETER_AXIS_Y;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Mag.Y, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_MAGNETOMETER_AXIS_Z;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Mag.Z, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_MAGNETOMETER_RESISTANCE;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Mag.R, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_MAGNETOMETER_NOTIFICATIONS */

#if ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_ENVIRONMENTAL_PRESSURE;
        ServiceInfo.sensorServicesType = (uint8_t) BLE_SENSOR_ENVIRONMENTAL;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Pressure, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_ENVIRONMENTAL_TEMPERATURE;
        retcode = BLE_SendData((uint8_t *) &sensorValue.Temp, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
    if (RETCODE_OK == retcode)
    {
        ServiceInfo.sensorServicesContent = (uint8_t) SENSOR_ENVIRONMENTAL_HUMIDITY;
        retcode = BLE_SendData((uint8_t *) &sensorValue.RH, 4, &ServiceInfo, BLE_SEND_TIMEOUT_IN_MS);
    }
#endif /* ENABLE_ENVIRONMENT_SENSOR_NOTIFICATIONS */

    return (retcode);
}

#endif

/**
 * @brief This function is to control the Application specific LED indication for BLE connection status
 * - If connected then Orange LED is turned ON
 * - If disconnected then Rolling LED pattern is provided
 *
 * @param[in]   bleConnectionStatus
 * Boolean representing the control BLE connection status
 *
 */
static void AppSpecificLedIndication(bool bleConnectionStatus)
{
    Retcode_T retcode = RETCODE_OK;
    if (bleConnectionStatus)
    {

        retcode = LED_Pattern(false, LED_PATTERN_ROLLING, 0UL);
        if (RETCODE_OK == retcode)
        {
            retcode = LED_On(LED_INBUILT_ORANGE);
        }
    }
    else
    {
        retcode = LED_Off(LED_INBUILT_ORANGE);
        if (RETCODE_OK == retcode)
        {
            retcode = LED_Pattern(true, LED_PATTERN_ROLLING, 1000UL);
        }
    }
    if (RETCODE_OK != retcode)
    {
        Retcode_RaiseError(retcode);
    }
}

/**
 * @brief Responsible for controlling the Virtual XDK demo application control flow.
 *
 * - Wait for BLE to be connected
 * - Based on user need, sample the sensor and notify all the necessary sensor / high priority data
 *
 * @param[in] pvParameters
 * Unused
 */
static void AppControllerFire(void* pvParameters)
{
    BCDS_UNUSED(pvParameters);

    Retcode_T retcode = RETCODE_OK;
    bool bleIsConnected = false;

    while (1)
    {
        /* Resetting / clearing the necessary buffers / variables for re-use */
        retcode = RETCODE_OK;

        if (bleIsConnected != BLE_IsConnected())
        {
            bleIsConnected = BLE_IsConnected();
            AppSpecificLedIndication(bleIsConnected);
        }

        if (SensorDataStartSampling && bleIsConnected)
        {
#if ENABLE_HIGH_PRIO_DATA_SERVICE
            retcode = SampleAndNotifyHighPriorityData();
#else /* ENABLE_HIGH_PRIO_DATA_SERVICE */
            retcode = SampleAndNotifySensorData();
#endif /* ENABLE_HIGH_PRIO_DATA_SERVICE */
        }
        else
        {
            SensorDataStartSampling = false;
        }
        if (RETCODE_OK != retcode)
        {
            Retcode_RaiseError(retcode);
        }
        SampleCounter++;

        vTaskDelay(pdMS_TO_TICKS(DataSampleRateInMs));
    }
}

/**
 * @brief Responsible for reading the RMS voltage of noise sensor.
 *
 * @param[in] pvParameters
 * Unused
 */
static void NoiseSensorTask(void* pvParameters)
{
    BCDS_UNUSED(pvParameters);

    Retcode_T retcode = RETCODE_OK;
    float rmsVoltage = 0.0f;
    static uint32_t sampleCounter = 0UL;
    static float startVal = 0.0f;
    static uint32_t avgCntr = 0UL;
    static float smoothVal = 0.0f;
    static float smoothAvgVal = 0.0f;

    while (1)
    {
        retcode = NoiseSensor_ReadRmsValue(&rmsVoltage, 20U);
        if (RETCODE_OK != retcode)
        {
            Retcode_RaiseError(retcode);
            continue;
        }

        if (sampleCounter == 0)
        {
            startVal = rmsVoltage;
            sampleCounter = 1;
        }
        else
        {
            if (NOISE_RMS_ADC_SAMPLES >= sampleCounter)
            {
                if (rmsVoltage > (startVal + NOISE_RMS_AVG_RANGE_IN_MV))
                {
                    /* @todo - Handle in common code noise sensor */
                    smoothVal += (rmsVoltage * rmsVoltage) * ADC_CENTRAL_NO_OF_SAMPLES;
                    avgCntr++;
                }
                else if (rmsVoltage <= startVal)
                {
                    startVal = rmsVoltage;
                }
                sampleCounter++;
            }
            else
            {
                if (avgCntr != 0)
                {
                    smoothAvgVal = sqrt(smoothVal / (ADC_CENTRAL_NO_OF_SAMPLES * avgCntr));
                }
                else
                {
                    smoothAvgVal = rmsVoltage;
                }
                NoiseRmsVoltage = smoothAvgVal;
                avgCntr = 0;
                sampleCounter = 1;
                startVal = 1;
                smoothVal = 0;
            }
        }
    }
}

/**
 * @brief To enable the necessary modules for the application
 * - BLE
 * - Sensor
 *  - Internal
 *  - External
 *  - Virtual
 * - Storage
 * - LED
 * - Button
 */
static void AppControllerEnable(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    Retcode_T retcode = RETCODE_OK;

    if (APP_CONTROLLER_STEP1 == param2)
    {
        retcode = Sensor_Enable();
        if (RETCODE_OK == retcode)
        {
            retcode = LED_Enable();
            if (RETCODE_OK == retcode)
            {
                AppSpecificLedIndication(false); /* BLE Connection Status */
                retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, APP_CONTROLLER_STEP2);
            }
        }

        if (RETCODE_OK != retcode)
        {
            printf("AppControllerEnable : Failed \r\n");
            Retcode_RaiseError(retcode);
            assert(0); /* To provide LED indication for the user */
        }
    }
    else
    {
        retcode = BLE_Enable();

        if (RETCODE_OK == retcode)
        {
            retcode = NoiseSensor_Enable();
        }

        if (RETCODE_OK == retcode)
        {
            retcode = Storage_Enable();
        }
        if ((RETCODE_OK == retcode) || ((uint32_t) RETCODE_STORAGE_SDCARD_NOT_AVAILABLE == Retcode_GetCode(retcode)))
        {
            retcode = Button_Enable();
        }
        if (((ExternalSensorSetup.Enable.LemVoltage) || (ExternalSensorSetup.Enable.LemCurrent)) && (true == lemHwConnectStatus))
        {
            if (RETCODE_OK == retcode)
            {
                retcode = Lem_Enable();
            }
        }
        if (((ExternalSensorSetup.Enable.MaxTemp) || (ExternalSensorSetup.Enable.MaxResistance)) && (true == extTempHwConnectStatus))
        {
            if (RETCODE_OK == retcode)
            {
                retcode = MAX31865_Connect();
            }
        }
        if (RETCODE_OK == retcode)
        {
            retcode = VirtualSensor_Enable();
        }
        if (RETCODE_OK == retcode)
        {
            if (pdPASS != xTaskCreate(AppControllerFire, (const char * const ) "AppController", TASK_STACK_SIZE_APP_CONTROLLER, NULL, TASK_PRIO_APP_CONTROLLER, &AppControllerHandle))
            {
                retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_OUT_OF_RESOURCES);
            }
        }
        if (RETCODE_OK == retcode)
        {
            if (pdPASS != xTaskCreate(NoiseSensorTask, (const char * const ) "NoiseSensor", TASK_STACK_SIZE_NOISE_SENSOR, NULL, TASK_PRIO_NOISE_SENSOR, &NoiseSensorHandle))
            {
                retcode = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_OUT_OF_RESOURCES);
            }
        }
        if (RETCODE_OK != retcode)
        {
            printf("AppControllerEnable : Failed \r\n");
            Retcode_RaiseError(retcode);
            assert(0); /* To provide LED indication for the user */
        }
        Utils_PrintResetCause();
    }
}

/**
 * @brief To setup the necessary modules for the application
 * - BLE
 * - Sensor
 *  - Internal
 *  - External
 *  - Virtual
 * - Storage
 * - LED
 * - Button
 *
 * @param[in] param1
 * Unused
 *
 * @param [in] param2
 * Step to be fired
 */
static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    Retcode_T retcode = RETCODE_OK;

    if (APP_CONTROLLER_STEP1 == param2)
    {
        SensorSetup.CmdProcessorHandle = AppCmdProcessor;
        retcode = Sensor_Setup(&SensorSetup);

        if (RETCODE_OK == retcode)
        {
            retcode = LED_Setup();
            if (RETCODE_OK == retcode)
            {
                retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, APP_CONTROLLER_STEP1);
            }
        }
        if (RETCODE_OK != retcode)
        {
            printf("AppControllerSetup : Failed \r\n");
            Retcode_RaiseError(retcode);
            assert(0); /* To provide LED indication for the user */
        }
    }
    else
    {

        VirtualSensor_Setup_T setup;
        setup.Enable = VirtualSensors;

        /* XDK Software Release Version is updated by this API */
        GetFwVersionInfo();

        retcode = BLE_Setup(&BLESetupInfo);
        if (RETCODE_OK == retcode)
        {
            retcode = NoiseSensor_Setup(NOISE_SAMPLING_FREQUENCY);
        }
        if (RETCODE_OK == retcode)
        {
            retcode = ExternalSensor_HwStatusPinInit();
        }
        if ((RETCODE_OK == retcode) && ((ExternalSensorSetup.Enable.LemVoltage) || (ExternalSensorSetup.Enable.LemCurrent)))
        {
            retcode = ExternalSensor_IsAvailable(XDK_EXTERNAL_LEM, &lemHwConnectStatus);
            if (true == lemHwConnectStatus)
            {
                ExtHwConnected = XDK_EXTERNAL_LEM;
            }
            else
            {
                retcode = ExternalSensor_IsAvailable(XDK_EXTERNAL_MAX31865, &extTempHwConnectStatus);
            }
            if (RETCODE_OK == retcode)
            {
                if ( false == lemHwConnectStatus && false == extTempHwConnectStatus)
                {
                    ExtHwConnected = XDK_EXTERNAL_INVALID;
                    //No sensor connected on power up
                }
                else if (true == lemHwConnectStatus && true == extTempHwConnectStatus)
                {
                    // there is borad problem not possible to have both at same time
                }
                else if (true == extTempHwConnectStatus)
                {
                    ExtHwConnected = XDK_EXTERNAL_MAX31865;
                }
            }
        }
        if (RETCODE_OK == retcode)
        {
            if (ExtHwConnected != XDK_EXTERNAL_INVALID)
            {
                retcode = ExternalSensor_HwStatusPinDeInit();
            }
        }
        if (RETCODE_OK == retcode)
        {
            if ((ExternalSensorSetup.Enable.LemVoltage) || (ExternalSensorSetup.Enable.MaxTemp))
            {
                if (RETCODE_OK == retcode)
                {
                    ExternalSensorSetup.CmdProcessorHandle = AppCmdProcessor;
                    retcode = ExternalSensor_Setup(&ExternalSensorSetup);
                }
            }
        }
        if (RETCODE_OK == retcode)
        {
            retcode = VirtualSensor_Setup(&setup);
        }
        if (RETCODE_OK == retcode)
        {
            retcode = Storage_Setup(&StorageSetup);
        }
        if (RETCODE_OK == retcode)
        {
            ButtonSetup.CmdProcessorHandle = AppCmdProcessor;
            retcode = Button_Setup(&ButtonSetup);
        }
        if (RETCODE_OK == retcode)
        {
            retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, APP_CONTROLLER_STEP2);
        }
        if (RETCODE_OK != retcode)
        {
            printf("AppControllerSetup : Failed \r\n");
            Retcode_RaiseError(retcode);
            assert(0); /* To provide LED indication for the user */
        }
    }
}

/* global functions ********************************************************* */

/** Refer interface header for description */
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
        retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, APP_CONTROLLER_STEP1);
    }

    if (RETCODE_OK != retcode)
    {
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
    }
}

/**@} */
/** ************************************************************************* */
