/*
 * BrightGarden
 *
 * Authors: Sebastian Garcia
 *          Sergio Oliveros
 *          Leandro Gualdrón
 *
 * Connections:
 *              - LED1: GPIO 15
 *              - LED2: GPIO 4
 */

#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <Preferences.h>
#include <WiFi.h>

#include "AES128.h"
#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
#include "credentials.h"

// Pin definitions
#define LED0_PIN 2  // Built in LED for debugging and notifications
#define LED1_PIN 15
#define LED2_PIN 4

// Firebase definitions
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Connection status
bool wifiOk = false;
bool signupOk = false;
bool gotSettings = false;

// Save settings and JSON definitions
Preferences prefs;

String json_data;
StaticJsonDocument<200> json_doc;
String default_settings = "IWRe3TPX8H+sUhJhFyVRxmgjlthtLQM7lkeqPeaRUuHL4DK+275IPQAbB6og4e3RjOWvWB4l7FhCSWNcDD3UQcy8eteEM/f85bMmR5TewqThJ7p2+GpKKgaQ7txUgmypuCJZ9YrxSc6WnOd0sluJaA==";

// PWM definitions
const int freq = 5000;
const int led1_channel = 0;
const int led2_channel = 1;
const int resolution = 12;

// AES128 buffer definitions
char cleartext[256];
char ciphertext[512];

String convert_json() {
    // Convert JSON to string
    json_data = "";
    serializeJson(json_doc, json_data);
    return json_data;
}

void save_settings() {
    // Save settings to flash memory
    sprintf(cleartext, "%s", convert_json().c_str());
    prefs.putString("settings", aes_encrypt(cleartext));
}

void load_settings() {
    // Load settings from flash memory
    String encrypted = prefs.getString("settings", default_settings);

    // Decrypt settings
    sprintf(ciphertext, "%s", encrypted.c_str());
    json_data = aes_decrypt(ciphertext);

    DeserializationError error = deserializeJson(json_doc, json_data);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    Serial.print("Loaded settings: ");
    Serial.println(convert_json());
}

void apply_settings() {
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

void update_settings(String messageStr) {
    Serial.print("Received settings: ");
    Serial.println(messageStr);
    // Decrypt message
    sprintf(ciphertext, "%s", messageStr.c_str());
    messageStr = aes_decrypt(ciphertext);
    Serial.print("Decrypted settings: ");
    Serial.println(messageStr);

    // Process JSON and update settings
    DeserializationError error = deserializeJson(json_doc, messageStr);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    save_settings();
    apply_settings();
}

void setup() {
    // Setup LEDs
    pinMode(LED0_PIN, OUTPUT);
    ledcSetup(led1_channel, freq, resolution);
    ledcAttachPin(LED1_PIN, led1_channel);
    ledcSetup(led2_channel, freq, resolution);
    ledcAttachPin(LED2_PIN, led2_channel);

    Serial.begin(115200);
    delay(10);

    // Initialize AES128
    aes_init();

    // Initialize preferences and load settings from flash memory
    prefs.begin("BrightGarden", false);
    load_settings();

    // Connect to WiFi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    for (uint8_t i = 0; i < 10; i++) {  // Try to connect 10 times
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi connection succeeded");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            wifiOk = true;
            break;
        }
        Serial.print(".");
        delay(500);
    }

    if (!wifiOk) {
        // If WiFi connection failed, use last saved settings
        Serial.println("WiFi connection failed");
        apply_settings();
    } else {
        // If WiFi connection succeeded, try to sign up
        // Connect to Firebase
        Serial.println();
        config.api_key = api_key;
        config.database_url = database_url;

        for (uint8_t i = 0; i < 3; i++) {  // Try to sign up 3 times
            if (Firebase.signUp(&config, &auth, "", "")) {
                Serial.println("Sign up succeeded");
                signupOk = true;
                break;
            } else {
                Serial.println("Sign up failed");
                Serial.printf("REASON: %s\n", config.signer.signupError.message.c_str());
            }
            delay(500);
        }

        config.token_status_callback = tokenStatusCallback;

        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);

        // Get settings from Firebase
        if (Firebase.ready() && signupOk) {
            for (uint8_t i = 0; i < 3; i++) {  // Try to get settings 3 times
                if (Firebase.getString(fbdo, "BrightGarden_FireBase/settings")) {
                    Serial.println("Get settings succeeded");
                    update_settings(fbdo.stringData());
                    gotSettings = true;
                    break;
                } else {
                    Serial.println("Get settings failed");
                    Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
                }
                delay(500);
            }
        }

        // If settings were not retrieved, use last saved settings
        if (!gotSettings) {
            Serial.println("Using last saved settings");
            apply_settings();
        }

    }  // End of WiFi connection
}

void loop() {
}