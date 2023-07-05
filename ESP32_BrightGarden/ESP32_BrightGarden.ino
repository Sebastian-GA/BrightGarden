/*
 * BrightGarden
 *
 * Authors: Sebastian Garcia Angarita
 *          Sergio Sebastian Oliveros Sepulveda
 *
 * Connections:
 *              - LED1: GPIO 15
 *              - LED2: GPIO 4
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#include "credentials.h"

// Pin definitions
#define LED0_PIN 2 // Built in LED for debugging and notifications
#define LED1_PIN 15
#define LED2_PIN 4

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure espClient;
PubSubClient client(espClient);
char msg[50];

// Save settings and JSON definitions
Preferences prefs;

String json_data;
StaticJsonDocument<200> json_doc;
String default_settings = "{\"lamp1\":{\"enable\":true,\"brightness\":100,\"time\":2},\"lamp2\":{\"enable\":true,\"brightness\":100,\"time\":2}}";

// PWM definitions
const int freq = 5000;
const int led1_channel = 0;
const int led2_channel = 1;
const int resolution = 12;

String convert_json()
{
    // Convert JSON to string
    json_data = "";
    serializeJson(json_doc, json_data);
    return json_data;
}

void save_settings()
{
    // Save settings to flash memory
    prefs.putString("settings", convert_json());
}

void load_settings()
{
    json_data = prefs.getString("settings", default_settings);
    DeserializationError error = deserializeJson(json_doc, json_data);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    Serial.print("Loaded settings: ");
    Serial.println(convert_json());
}

void apply_settings()
{
    // Apply settings to LEDs
    if (json_doc["lamp1"]["enable"])
        ledcWrite(led1_channel, json_doc["lamp1"]["brightness"].as<int>() * 40.95);
    else
        ledcWrite(led1_channel, 0);

    if (json_doc["lamp2"]["enable"])
        ledcWrite(led2_channel, json_doc["lamp2"]["brightness"].as<int>() * 40.95);
    else
        ledcWrite(led2_channel, 0);
}

void setup_wifi()
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    // ----------------------- Process topic and message ----------------------- //
    if (String(topic) == mqtt_topic_request)
    {
        if (messageTemp == "settings")
        {
            // Send JSON with settings
            Serial.println("Sending settings");
            load_settings(); // Get save settings in flash memory
            client.publish(mqtt_topic_status, convert_json().c_str());
        }
        else if (messageTemp == "status")
        {
            // Send JSON with status
            Serial.println("Sending status");
            client.publish(mqtt_topic_status, "{\"status\":\"OK\"}");
        }
    }
    else if (String(topic) == mqtt_topic_setup)
    {
        // Process JSON and update settings
        DeserializationError error = deserializeJson(json_doc, messageTemp);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        else
        {
            save_settings();
            apply_settings();
            client.publish(mqtt_topic_request, "status"); // Send status to confirm
        }
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", mqtt_username, mqtt_password))
        {
            Serial.println("connected");
            client.subscribe(mqtt_topic_setup);   // Subscribe to receive setup messages
            client.subscribe(mqtt_topic_request); // Subscribe to receive request messages
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup()
{
    // Setup LEDs
    pinMode(LED0_PIN, OUTPUT);
    ledcSetup(led1_channel, freq, resolution);
    ledcSetup(led2_channel, freq, resolution);
    ledcAttachPin(LED1_PIN, led1_channel);
    ledcAttachPin(LED2_PIN, led2_channel);

    Serial.begin(115200);
    delay(10);

    // Load settings
    prefs.begin("BrightGarden", false);
    load_settings();
    apply_settings();

    // Connect to WiFi and MQTT Broker
    setup_wifi();
    espClient.setCACert(root_ca);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
}