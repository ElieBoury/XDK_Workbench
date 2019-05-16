/**
 * This software is copyrighted by Bosch Connected Devices and Solutions GmbH, 2016.
 * The use of this software is subject to the XDK SDK EULA
 */

/* module includes ********************************************************** */

/* system header files */
#include <stdint.h>

/* interface header files */
#include "PIp.h"
#include "BCDS_WlanConnect.h"
#include "netcfg.h"
#include "Serval_Ip.h"
#include "wlan.h"
#include "EnterpriseWiFi.h"
/* system header files */
#include <stdint.h>
#include <stdio.h>
/* constant definitions ***************************************************** */

/* local variables ********************************************************** */
static char wifiMacAddress[]              = "00:00:00:00:00:00";    /**< To keep the MAC adress returned by the wifi network device */

/* local variables ********************************************************** */

/* global variables ********************************************************* */
void *g_mac_addr = (void*)wifiMacAddress;

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

/* global functions ********************************************************* */


/**
 * @brief conenct to WPA2-Enterprise Wifi
 *
 * @return NONE
 */
void EnterpriseWiFi(void)
{
    /* Initialize Variables */
    uint8_t _macVal[WIFI_MAC_ADDR_LEN];
    uint8_t _macAddressLen = WIFI_MAC_ADDR_LEN;
    NetworkConfig_IpSettings_T myIpSettings;
    Retcode_T retStatusSetIp;
    memset(&myIpSettings, (uint32_t) 0, sizeof(myIpSettings));
    int8_t ipAddress[PAL_IP_ADDRESS_SIZE] = {0};
    Ip_Address_T* IpaddressHex = Ip_getMyIpAddr();
    WlanConnect_SSID_T connectSSID = (WlanConnect_SSID_T) WLAN_ENT_SSID;
    WlanConnect_PassPhrase_T connectPassPhrase = (WlanConnect_PassPhrase_T) WLAN_ENT_PWD;

    /* Initialize the Wireless Network Driver */
    if(WLANDRIVER_SUCCESS != WlanConnect_Init())
    {
        printf("Error occurred initializing WLAN \r\n");
        return;
    }

    else
    {
    	printf("WLAN Module initialization succeeded \r\n");
    }

    if(ENABLE_DHCP)
    {
        /* Configure settings to DHCP */
        retStatusSetIp = NetworkConfig_SetIpDhcp(NULL);
        if (retStatusSetIp != RETCODE_SUCCESS)
        {
            printf("Error in setting IP to DHCP\n\r");
            return;
        }

        else
        {
        	printf("Set DHCP settings succeeded\r\n");
        }
    }
    else 
    {
        /* Configure settings to static IP */
    	NetworkConfig_IpSettings_T myIpSet;
        myIpSet.isDHCP =        (uint8_t) NETWORKCONFIG_DHCP_DISABLED;
        myIpSet.ipV4 =          IPV4;
        myIpSet.ipV4DnsServer = IPV4_DNS_SERVER;
        myIpSet.ipV4Gateway =   IPV4_GATEWAY;
        myIpSet.ipV4Mask =      IPV4_MASK;

        retStatusSetIp = NetworkConfig_SetIpStatic(myIpSet);
        if (retStatusSetIp == RETCODE_SUCCESS)
        {
            printf("[NCA] : Static IP is set to : %u.%u.%u.%u\n\r",
                    (unsigned int) (NetworkConfig_Ipv4Byte(myIpSet.ipV4, 3)),
                    (unsigned int) (NetworkConfig_Ipv4Byte(myIpSet.ipV4, 2)),
                    (unsigned int) (NetworkConfig_Ipv4Byte(myIpSet.ipV4, 1)),
                    (unsigned int) (NetworkConfig_Ipv4Byte(myIpSet.ipV4, 0)));
        }
        else 
        {
            printf("Static IP configuration failed\n");
        }
    }

    /* disable server authentication */
    unsigned char pValues;
    pValues = 0;  //0 - Disable the server authentication | 1 - Enable (this is the default)
    sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,19,1,&pValues);

    /* Connect to the Wireless WPA2-Enterprise Network */
    printf("Connecting to %s...\r\n", connectSSID);

    if (RETCODE_SUCCESS == WlanConnect(connectSSID, connectPassPhrase, NULL)) {

    	NetworkConfig_GetIpSettings(&myIpSettings);
        *IpaddressHex = Basics_htonl(myIpSettings.ipV4);
        (void)Ip_convertAddrToString(IpaddressHex,(char *)&ipAddress);
        printf("Connected to WPA network successfully. \r\n");
        printf("Ip address of the device: %s \r\n",ipAddress);
    }
    else  {
        printf("Error occurred connecting %s \r\n ", connectSSID);
        return;
    }

    /* Get the MAC Address */
    memset(_macVal, UINT8_C(0), WIFI_MAC_ADDR_LEN);
    int8_t _status = sl_NetCfgGet(SL_MAC_ADDRESS_GET,
                                    NULL,
                                    &_macAddressLen,
                                    (uint8_t *) _macVal);

    if (WIFI_FAILED == _status) {
        printf("sl_NetCfgGet Failed\r\n");
    }
    else {
        /* Store the MAC Address into the Global Variable */
        memset(wifiMacAddress, UINT8_C(0), strlen(wifiMacAddress));
        sprintf(wifiMacAddress,"%02X:%02X:%02X:%02X:%02X:%02X",_macVal[0],
                                                             _macVal[1],
                                                             _macVal[2],
                                                             _macVal[3],
                                                             _macVal[4],
                                                             _macVal[5]);
        printf("MAC address of the device: %s \r\n",wifiMacAddress);
    }

}

Retcode_T WlanConnect(WlanConnect_SSID_T connectSSID,
		WlanConnect_PassPhrase_T connectPass,
		WlanConnect_Callback_T connectCallback)
{
    /* Local variables */
	WlanConnect_Status_T retVal;
    int8_t slStatus;
    SlSecParams_t secParams;
    SlSecParamsExt_t eapParams;

    /* Set network security parameters */
    secParams.Key = connectPass;
    secParams.KeyLen = strlen((const char*) connectPass);
    secParams.Type = SEC_TYPE;

    /* Set network EAP parameters */
    eapParams.User = (signed char*)WLAN_USERNAME;
    eapParams.UserLen = strlen(WLAN_USERNAME);
    eapParams.AnonUser = (signed char*)WLAN_USERNAME;
    eapParams.AnonUserLen = strlen(WLAN_USERNAME);
    eapParams.CertIndex = UINT8_C(0);
    eapParams.EapMethod = SL_ENT_EAP_METHOD_PEAP0_MSCHAPv2;

    /* Call the connect function */
    slStatus = sl_WlanConnect((signed char*) connectSSID,
            strlen(((char*) connectSSID)), (unsigned char *) WLANCONNECT_NOT_INITIALZED,
            &secParams, &eapParams);

    /* Determine return value*/
    if (WLANCONNECT_NOT_INITIALZED == slStatus)
    {
        if (WLANCONNECT_NOT_INITIALZED == connectCallback)
        {
            while ((WLAN_DISCONNECTED == WlanConnect_GetStatus())
                    || (NETWORKCONFIG_IP_NOT_ACQUIRED == NetworkConfig_GetIpStatus()))
            {
                /* Stay here until connected. */
                /* Timeout logic can be added here */
            	printf("Hello I am going to connect to manus\n\r");
            }
            /* In blocking mode a successfully return of the API also means
             * that the connection is ok*/
            retVal = WLAN_CONNECTED;
        }
        else
        {
            /* In callback mode a successfully return of the API does not mean
             * that the WLAN Connection has succeeded. The SL event handler will
             * notify the user with WLI_CONNECTED when connection is done*/
            retVal = WLAN_CONNECTED;
            vTaskDelay(5000);
            printf("It' working ! \n\r");
        }
    }
    else
    {
    	vTaskDelay(5000);
    	printf("Failed at simplelink \n\r");
        /* Simple Link function encountered an error.*/
        retVal = WLAN_CONNECTION_ERROR;
    }

    return (retVal);
}


/* @brief This is a template function where the user can write his custom application.
*
*/
void appInitSystem(xTimerHandle xTimer)
{
	(void) xTimer;
    EnterpriseWiFi();
}

/** ************************************************************************* */
