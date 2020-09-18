/*
Proof of concept  for connection to Azure IoT hub using the Azure Embedded C SDK 
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <paho-mqtt/MQTTClient.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

char iot_hub_device_id = "patrick-test-pi";
char iot_hub_hostname = "michaels-test-hub.azure-devices.net";
static az_iot_hub_client hub_client;


int main(void) 
{
    MQTTClient client;
    create_mqtt_client(&client);

    connect_client_to_hub();
}

az_result create_mqtt_client()
{
    int rc;

    char mqtt_endpoint[128] = "ssl://michaels-test-hub.azure-devices.net:8883";

    rc = az_iot_hub_client_init(&hub_client, iot_hub_hostname, iot_hub_device_id, NULL);)

    if (az_result_failed(rc))
    {
        printf("Failed to initialize hub client: az_result return code 0x%08x.", rc);
        exit(rc);
    }
    else
    {
        printf("IoT Hub cleint created successfully");
    }
    
    
}