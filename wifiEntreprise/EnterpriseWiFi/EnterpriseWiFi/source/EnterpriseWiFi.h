/**
 * This software is copyrighted by Bosch Connected Devices and Solutions GmbH, 2016.
 * The use of this software is subject to the XDK SDK EULA
 */

/* header definition ******************************************************** */
#ifndef XDK110_ENTERPRISEWIFI_H_
#define XDK110_ENTERPRISEWIFI_H_

#include "BCDS_NetworkConfig.h"
#include "wlan.h"
/* local interface declaration ********************************************** */

/* local type and macro definitions */

//#warning Please enter WLAN configurations with valid SSID & WPA username and key in below Macros and remove this line to avoid warnings.

#define WLAN_ENT_SSID           "NETWORK_SSID"
#define WLAN_USERNAME           "NETWORK_USER_NAME"
#define WLAN_ENT_PWD            "NETWORK_PASS_PHRASE"

#define ENABLE_DHCP             true
/* Update static IP fields if DHCP is disabled, replace zero values */
#define IPV4                    NetworkConfig_Ipv4Value(0, 0, 0, 0)
#define IPV4_DNS_SERVER         NetworkConfig_Ipv4Value(0, 0, 0, 0)
#define IPV4_GATEWAY            NetworkConfig_Ipv4Value(0, 0, 0, 0)
#define IPV4_MASK               NetworkConfig_Ipv4Value(0, 0, 0, 0)

#define SEC_TYPE                0x05 //WLI_SECURITY_TYPE_WPA_ENT

#define WIFI_FAILED             INT32_C(-1)                 /**< Macro used to return failure message*/
#define WIFI_MAC_ADDR_LEN       UINT8_C(6)                  /**< Macro used to specify MAC address length*/

#define FILE_NAME               "/cert/ca.pem"

static Retcode_T WlanConnect(WlanConnect_SSID_T connectSSID,
		WlanConnect_PassPhrase_T connectPass,
		WlanConnect_Callback_T connectCallback);

#endif /* XDK110_ENTERPRISEWIFI_H_ */

/** ************************************************************************* */
