#include "taskInitThingsBorad.h"

TaskHandle_t TaskThingsBoard = NULL;

constexpr char TOKEN[] = "aQDbrlpvJIVtdGIWMZLc";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr char DEVICE_PROFILE[] = "Test Gateway";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

uint32_t previousStateChange;

constexpr int16_t telemetrySendInterval = 10000U;
uint32_t previousDataSend;

// Connection retry parameters
constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 3;
constexpr uint32_t RECONNECT_DELAY_MS = 5000;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR,
    BLINKING_INTERVAL_ATTR};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

RPC_Response setLedSwitchState(const RPC_Data &data)
{
    Serial.println("Received Switch state");
    bool newState = data;
    Serial.print("Switch state change: ");
    Serial.println(newState);
    attributesChanged = true;
    return RPC_Response("setLedSwitchValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
    RPC_Callback{"setLedSwitchValue", setLedSwitchState}};

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
        {
            const uint16_t new_interval = it->value().as<uint16_t>();
            if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
            {
                blinkingInterval = new_interval;
                Serial.print("Blinking interval is set to: ");
                Serial.println(new_interval);
            }
        }
        else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0)
        {
            ledState = it->value().as<bool>();
            Serial.print("LED state is set to: ");
            Serial.println(ledState);
        }
    }
    attributesChanged = true;
}

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

bool connectToThingsBoard()
{
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);

    if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT, DEVICE_PROFILE))
    {
        Serial.println("Failed to connect to ThingsBoard");
        return false;
    }

    // Send device info
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    tb.sendAttributeData("deviceType", "M5Atom");
    tb.sendAttributeData("firmwareVersion", "1.0.0");

    Serial.println("Subscribing for RPC...");
    if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
    {
        Serial.println("Failed to subscribe for RPC");
        return false;
    }

    if (!tb.Shared_Attributes_Subscribe(attributes_callback))
    {
        Serial.println("Failed to subscribe for shared attribute updates");
        return false;
    }

    Serial.println("Subscribe done");

    if (!tb.Shared_Attributes_Request(attribute_shared_request_callback))
    {
        Serial.println("Failed to request for shared attributes");
        return false;
    }

    Serial.println("ThingsBoard connected successfully");
    return true;
}

void taskThingsBoard(void *pvParameters)
{
    uint8_t reconnectAttempts = 0;
    uint32_t lastReconnectAttempt = 0;

    while (true)
    {
        // Check WiFi connection first
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi not connected, waiting...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Try to connect to ThingsBoard if not connected
        if (!tb.connected())
        {
            if (millis() - lastReconnectAttempt > RECONNECT_DELAY_MS)
            {
                if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS)
                {
                    Serial.printf("ThingsBoard connection attempt %d/%d\n", reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);

                    if (connectToThingsBoard())
                    {
                        reconnectAttempts = 0; // Reset counter on successful connection
                    }
                    else
                    {
                        reconnectAttempts++;
                    }
                    lastReconnectAttempt = millis();
                }
                else
                {
                    Serial.println("Max reconnection attempts reached, waiting longer...");
                    vTaskDelay(pdMS_TO_TICKS(30000)); // Wait 30 seconds before trying again
                    reconnectAttempts = 0;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Send telemetry data periodically
        if (millis() - previousDataSend > telemetrySendInterval)
        {
            previousDataSend = millis();

            // Send WiFi information
            tb.sendAttributeData("rssi", WiFi.RSSI());
            tb.sendAttributeData("channel", WiFi.channel());
            tb.sendAttributeData("bssid", WiFi.BSSIDstr().c_str());
            tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
            tb.sendAttributeData("ssid", WiFi.SSID().c_str());

            // Send system information
            tb.sendTelemetryData("freeHeap", ESP.getFreeHeap());
            tb.sendTelemetryData("uptime", millis() / 1000);

            Serial.println("Telemetry data sent");
        }

        // Process ThingsBoard messages
        tb.loop();

        // Task delay
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void initSetUpThingBoard()
{
    BaseType_t result = xTaskCreate(
        taskThingsBoard,
        "ThingsBoardTask",
        4096,
        NULL,
        1,
        &TaskThingsBoard);

    if (result == pdPASS)
    {
        Serial.println("ThingsBoard task created successfully");
    }
    else
    {
        Serial.println("Failed to create ThingsBoard task");
    }
}