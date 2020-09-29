/*
Proof of concept  for connection to Azure IoT hub using the Azure Embedded C SDK 
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <paho-mqtt/MQTTClient.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

// This is where AZIoT sends C2D messages
#define AZ_IOT_HUB_SUBSCRIBE_TOPIC "devices/+/messages/devicebound/#"

#define MQTT_TIMEOUT_RECEIVE_MS (60 * 1000)

// Hardcoded in for now, can be optained through environment variables if necessary
// char iot_hub_device_id[] = "patrick-test-pi-2";
// char iot_hub_hostname[] = "michaels-test-hub.azure-devices.net";

static az_iot_hub_client hub_client;
MQTTClient mqtt_client;

// Could not get the username fucntion to generate this string properly, so it is hardcoded
char mqtt_client_username[128] = "michaels-test-hub.azure-devices.net/patrick-test-pi-2/?api-version=2018-06-30&DeviceClientType=c%2F1.0.0-preview.6";

// These functions are basically intended to mimic those in the other iot_hub samples
void create_mqtt_client(void);
void connect_client_to_hub(void);
void subscribe_to_iot_hub_topic(void);
void send_message_to_iot_hub(void);
void receive_c2d_message(void);

int main(void) 
{
    create_mqtt_client();

    connect_client_to_hub();

    subscribe_to_iot_hub_topic();

    printf("\n**********\nWelcome to Patrick's Azure IoT Hub POC!\n**********\n");

    char resp[10];
    while(1)
    {
	printf("\nWould you like to send message, receive message, or exit (s/r/e)?\n");
	scanf("%s", resp);

	if(strcmp(resp, "s") == 0)
	{
	    send_message_to_iot_hub();
	}
	
	else if(strcmp(resp, "r") == 0)
	{
	    receive_c2d_message();
	}

	else if(strcmp(resp, "e") == 0)
	{
	    break;
	}

	else
	{
	    printf("Invalid response.\n\n");
	}
    }

    printf("Thanks for playing!\n");
}

void create_mqtt_client()
{
    int rc;

    char* iot_device_id_buffer = getenv("AZ_IOT_HUB_DEVICE_ID");
    char* iot_hostname_buffer = getenv("AZ_IOT_HUB_HOSTNAME");
    
    // hub_client_init expects 'az_span's
    az_span id_span = AZ_SPAN_FROM_BUFFER(iot_device_id_buffer);
    az_span hostname_span = AZ_SPAN_FROM_BUFFER(iot_hostname_buffer);

    rc = az_iot_hub_client_init(&hub_client, hostname_span, id_span, NULL);

    if (az_result_failed(rc))
    {
        printf("Failed to initialize hub client: az_result return code 0x%08x.", rc);
        exit(rc);
    }
    else
    {
        printf("IoT Hub client created successfully\n");
    }
    
    // Could not get commented function below to return the proper id, so it is hardcoded
    char mqtt_client_id[128] = "patrick-test-pi-2";

    /*rc = az_iot_hub_client_get_client_id(
	&hub_client, mqtt_client_id,  sizeof(mqtt_client_id), NULL);

    if(az_result_failed(rc))
    {
	printf("Failed to get MQTT client id 0x%08x.", rc);
	exit(rc);
    }*/

    // Hardcoded, but could be easily constructed from a user input if necessary
    char mqtt_endpoint[128] = "ssl://michaels-test-hub.azure-devices.net:8883";

    rc = MQTTClient_create(&mqtt_client, mqtt_endpoint, mqtt_client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    if(rc != MQTTCLIENT_SUCCESS)
    {
	printf("Failed  to create MQTT client: MQTTClient return code %d\n", rc);
    }

    else
    {
	printf("MQTT Client created successfully!\n");
    }
}

void connect_client_to_hub()
{
    int rc;

    // Get the key store filepath from environment variable
    // Hard coding in a string for this filepath causes MQTTClient_connect to fail
    char* key_store_filepath = getenv("AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH");
    if(key_store_filepath == NULL)
    {
	printf("Please set AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH env variable");
	return;
    }

    // This function will not work properly and just sets username to hostname (?)
    // Username is hard coded in
    /*rc = az_iot_hub_client_get_user_name(
	&hub_client, mqtt_client_username, sizeof(mqtt_client_username), NULL); 
    if(az_result_failed(rc))
    {
	printf("Failed to get hub client username: error code 0x%08x\n", rc);
	exit(rc);
    }

    else
    {
	printf("Get IoT Hub client username successful\n");
    }*/

    MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
    mqtt_connect_options.username = mqtt_client_username;
    printf("AZ IoT Hub username: %s\n", mqtt_connect_options.username);
    mqtt_connect_options.password = NULL;
    mqtt_connect_options.cleansession = false;
    mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

    MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
    mqtt_ssl_options.keyStore = key_store_filepath;

    mqtt_connect_options.ssl = &mqtt_ssl_options;

    rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options);
    
    if(rc != MQTTCLIENT_SUCCESS)
    {
	printf("Failed to connect MQTT Client: error code %d\n", rc);
	exit(rc);
    }
    else
    {
	printf("MQTT Client connected successfully\n");
    }
}

void subscribe_to_iot_hub_topic()
{
    int rc = MQTTClient_subscribe(mqtt_client, AZ_IOT_HUB_SUBSCRIBE_TOPIC, 1);
    if(rc != MQTTCLIENT_SUCCESS)
    {
	printf("Failed to subscribe to MQTT topic. Error code: %d\n", rc);
	exit(rc);
    }
    else
    {
	printf("MQTT Topic subscribe successful\n");
    }
}

void send_message_to_iot_hub()
{
    int rc;

    char telemetry_topic_buffer[128] = "devices/patrick-test-pi-2/messages/events/";
    char user_msg[25];
    char message_payload[50] = "{\"Message\":\"";

    printf("Enter a brief test message:\n");
    scanf("%s", user_msg);

    strcat(message_payload, user_msg);
    strcat(message_payload, "\"}");

    /*
    rc = az_iot_hub_client_telemetry_get_publish_topic(
	&hub_client, NULL, telemetry_topic_buffer, sizeof(telemetry_topic_buffer), NULL);

    if(az_result_failed(rc))
    {
	printf("Failed to get telemtry publish topic. Error code: %d\n", rc);
	exit(rc);
    }
    */

    rc = MQTTClient_publish(
	mqtt_client, telemetry_topic_buffer, (int)strlen(message_payload), message_payload, 1, 0, NULL);

    if(rc != MQTTCLIENT_SUCCESS)
    {
	printf("Failed to send MQTT message. Error code: %d\n", rc);
	exit(rc);
    }

    else
    {
	printf("MQTT Message published to topic: %s.\n", telemetry_topic_buffer);
    }
}

void receive_c2d_message()
{
    int rc;
    char* topic = NULL;
    int topic_len = 0;
    MQTTClient_message *message = NULL;

    printf("Waiting for message from IoT Hub...\n");

    rc = MQTTClient_receive(mqtt_client, &topic, &topic_len, &message, MQTT_TIMEOUT_RECEIVE_MS);
    if((rc != MQTTCLIENT_SUCCESS) && (rc != MQTTCLIENT_TOPICNAME_TRUNCATED))
    {
	printf("Failed to receive MQTT Message. Error code: %d\n", rc);
	exit(rc);
    }

    else if(message == NULL)
    {
	printf("Message timeout expired\n");
    }

    else if(rc == MQTTCLIENT_TOPICNAME_TRUNCATED)
    {
	topic_len = (int)strlen(topic);
    }

    printf("Message Received from IoT Hub\n");

    az_iot_hub_client_c2d_request c2d_request;
    az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);

    rc = az_iot_hub_client_c2d_parse_received_topic(
		    &hub_client, topic_span, &c2d_request);

    if(az_result_failed(rc))
    {
	printf("Message from unknown topic: %s. Error code: %d\n", topic, rc);
	exit(rc);
    }

    printf("Topic: %s\n", topic);
    printf("Payload: %s\n", (char*)message->payload);
}
