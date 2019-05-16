/* --------------------------------------------------------------------------- |
 * INCLUDES & DEFINES ******************************************************** |
 * -------------------------------------------------------------------------- */

/* own header files */
#include "XdkAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_APP_CONTROLLER

/* own header files */
#include "AppController.h"

/* system header files */
#include <stdio.h>

/* additional interface header files */
#include "BCDS_BSP_Board.h"
#include "BCDS_CmdProcessor.h"
#include "BCDS_NetworkConfig.h"
#include "BCDS_Assert.h"
#include "XDK_WLAN.h"
#include "XDK_MQTT.h"
#include "XDK_Sensor.h"
#include "XDK_SNTP.h"
#include "XDK_ServalPAL.h"
#include "FreeRTOS.h"
#include "task.h"

/* local variables ********************************************************** */

static WLAN_Setup_T WLANSetupInfo =
        {
                .IsEnterprise = false,
                .IsHostPgmEnabled = false,
                /*.SSID = "eduroam",
				.Username = "eboury@u-bordeaux.fr",
                .Password = "N@ntes0504",*/
				.SSID = "XDK_Hotspot",
				.Password = "FreeWifi",
                .IsStatic = false,
        };/**< WLAN setup parameters */


static MQTT_Setup_T MqttSetupInfo =
        {
                .MqttType = MQTT_TYPE_SERVALSTACK,
                .IsSecure = false,
        };/**< MQTT setup parameters */

static MQTT_Connect_T MqttConnectInfo =
        {
                .ClientId = "XDK110_Sunderland",
                .BrokerURL = "broker.hivemq.com",
                .BrokerPort = 1883,
                .CleanSession = true,
                .KeepAliveInterval = 100,
        };/**< MQTT connect parameters */

static void AppMQTTSubscribeCB(MQTT_SubscribeCBParam_T param);

static MQTT_Subscribe_T MqttSubscribeInfo =
        {
                .Topic = "mqtt/xdk/#",
                .QoS = 1UL,
                .IncomingPublishNotificationCB = AppMQTTSubscribeCB,
        };/**< MQTT subscribe parameters */

static MQTT_Publish_T MqttPublishInfo =
        {
                .Topic = "mqtt/xdk/test",
                .QoS = 1UL,
                .Payload = NULL,
                .PayloadLength = 0UL,
        };/**< MQTT publish parameters */

static uint32_t AppIncomingMsgCount = 0UL;/**< Incoming message count */

static char AppIncomingMsgTopicBuffer[256];/**< Incoming message topic buffer */

static char AppIncomingMsgPayloadBuffer[256];/**< Incoming message payload buffer */

static CmdProcessor_T * AppCmdProcessor;/**< Handle to store the main Command processor handle to be used by run-time event driven threads */

static xTaskHandle AppControllerHandle = NULL;/**< OS thread handle for Application controller to be used by run-time blocking threads */

/* global variables ********************************************************* */

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

static void AppMQTTSubscribeCB(MQTT_SubscribeCBParam_T param)
{
    AppIncomingMsgCount++;
    memset(AppIncomingMsgTopicBuffer, 0, sizeof(AppIncomingMsgTopicBuffer));
    memset(AppIncomingMsgPayloadBuffer, 0, sizeof(AppIncomingMsgPayloadBuffer));

    if (param.PayloadLength < (int) sizeof(AppIncomingMsgPayloadBuffer))
    {
        strncpy(AppIncomingMsgPayloadBuffer, (const char *) param.Payload, param.PayloadLength);
    }

    if (param.TopicLength < (int) sizeof(AppIncomingMsgTopicBuffer))
    {
        strncpy(AppIncomingMsgTopicBuffer, param.Topic, param.TopicLength);
    }


    printf("AppMQTTSubscribeCB : #%d, Incoming Message:\r\n"
            "\tTopic: %s\r\n"
            "\tPayload: \r\n\"\"\"\r\n%s\r\n\"\"\"\r\n", (int) AppIncomingMsgCount,
            AppIncomingMsgTopicBuffer, AppIncomingMsgPayloadBuffer);
}


/**
 * @brief Responsible for controlling the send data over MQTT application control flow.
 *
 * - Synchronize SNTP time stamp for the system if MQTT communication is secure
 * - Connect to MQTT broker
 * - Subscribe to MQTT topic
 * - Read environmental sensor data
 * - Publish data periodically for a MQTT topic
 *
 * @param[in] pvParameters
 * Unused
 */
static void AppControllerFire(void* pvParameters)
{
    BCDS_UNUSED(pvParameters);

    Retcode_T retcode = RETCODE_OK;

    const char * publishBuffer = "Hello World";
    MqttPublishInfo.Payload = publishBuffer;
    MqttPublishInfo.PayloadLength = strlen(publishBuffer);

    if (RETCODE_OK == retcode)
    {
        printf("MQTT Connection : Failed \r\n");
        retcode = MQTT_ConnectToBroker(&MqttConnectInfo, UINT32_C(60000));
    }

    if (RETCODE_OK == retcode)
    {
        printf("MQTT Subscription : Failed \r\n");
        retcode = MQTT_SubsribeToTopic(&MqttSubscribeInfo, UINT32_C(5000));
    }

    if (RETCODE_OK != retcode)
    {
        /* We raise error and still proceed to publish data periodically */
        Retcode_RaiseError(retcode);
    }

    /* A function that implements a task must not exit or attempt to return to
     its caller function as there is nothing to return to. */
    while (1)
    {
        MQTT_PublishToTopic(&MqttPublishInfo, UINT32_C(5000));
        vTaskDelay(UINT32_C(5000));
    }
}

/**
 * @brief To enable the necessary modules for the application
 * - WLAN
 * - ServalPAL
 * - SNTP (if secure communication)
 * - MQTT
 * - Sensor
 *
 * @param[in] param1
 * Unused
 *
 * @param[in] param2
 * Unused
 */
static void AppControllerEnable(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = WLAN_Enable();
    if (RETCODE_OK == retcode)
    {
        retcode = ServalPAL_Enable();
    }

    if (RETCODE_OK == retcode)
    {
        if (pdPASS != xTaskCreate(AppControllerFire, (const char * const ) "AppController", TASK_STACK_SIZE_APP_CONTROLLER, NULL, TASK_PRIO_APP_CONTROLLER, &AppControllerHandle))
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
}

/**
 * @brief To setup the necessary modules for the application
 * - WLAN
 * - ServalPAL
 * - SNTP (if secure communication)
 * - MQTT
 * - Sensor
 *
 * @param[in] param1
 * Unused
 *
 * @param[in] param2
 * Unused
 */
static void AppControllerSetup(void * param1, uint32_t param2)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(param2);

    Retcode_T retcode = WLAN_Setup(&WLANSetupInfo);
    if (RETCODE_OK == retcode)
    {
        retcode = ServalPAL_Setup(AppCmdProcessor);
    }
    if (RETCODE_OK == retcode)
    {
        retcode = MQTT_Setup(&MqttSetupInfo);
    }
    if (RETCODE_OK == retcode)
    {
        retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerEnable, NULL, UINT32_C(0));
    }
    if (RETCODE_OK != retcode)
    {
        printf("AppControllerSetup : Failed \r\n");
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
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
        retcode = CmdProcessor_Enqueue(AppCmdProcessor, AppControllerSetup, NULL, UINT32_C(0));
    }

    if (RETCODE_OK != retcode)
    {
        Retcode_RaiseError(retcode);
        assert(0); /* To provide LED indication for the user */
    }
}
