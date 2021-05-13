#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Scheduler.h>                  // https://github.com/nrwiersma/ESP8266Scheduler
#include <WiFiManager.h>                // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h> 
#include "html.h"

const char *NODE_NAME = "StepperControl";

const int STEPPER_PIN_ENABLE = D1;
const int STEPPER_PIN_DIRECTION = D2;
const int STEPPER_PIN_STEP = D3;
const int STEPPER_MIN = 10;             // minimum delay between pulses: fastest speed
const int STEPPER_MAX = 1000;           // maximum delay between pulses: slowest speed
const int STEPPER_PULSE = 10;           // pulse width

const int COMMAND_MIN = -511;
const int COMMAND_MAX = 512;
const int COMMAND_RATE = 100;           // minimum number of milliseconds between commands sent by the client

bool stepper_enable = false;
int stepper_direction = 0;
int stepper_speed = 0;

WiFiManager wifiManager;
ESP8266WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", html);
    Serial.println("root");
}

void handleCommand() {
    int vector = server.arg("vector").toInt();
    if (vector < COMMAND_MIN) {
        vector = COMMAND_MIN;
    }
    else if (vector > COMMAND_MAX) {
        vector = COMMAND_MAX;
    }
    stepper_direction = vector < 0 ? -1 : 1;
    stepper_speed = abs(vector);
    stepper_enable = stepper_speed > 0;
    char message[100];
    sprintf(message, "command enable: %d  direction: %d  speed: %d", 
        stepper_enable, stepper_direction, stepper_speed);
    server.send(200, "text/plain", message);
    Serial.println(message);
}

void handleConfig() {
    char json[100];
    snprintf(json, 100, "{\"name\": \"%s\", \"min\": %i, \"max\": %i, \"rate\": %i}", 
        NODE_NAME, COMMAND_MIN, COMMAND_MAX, COMMAND_RATE);
    server.send(200, "application/json", json);
    Serial.println("config");
}

class ServerTask : public Task {
    protected:
    void setup() {        
        server.on("/", handleRoot);
        server.on("/command", handleCommand);
        server.on("/config", handleConfig);
        server.begin();
        //if (MDNS.begin(NODE_NAME)) {
        if (MDNS.begin(NODE_NAME, WiFi.localIP(), 1)) { // TTL is ignored
            Serial.println("mDNS responder started");
        } else {
            Serial.println("Error setting up MDNS responder");
        }
        MDNS.addService("http", "tcp", 80);
    }
    void loop()  {
        server.handleClient();
        MDNS.update();
    }
} server_task;


class ControlTask : public Task {
    protected:
    void setup() {
        pinMode(STEPPER_PIN_ENABLE, OUTPUT);
        pinMode(STEPPER_PIN_DIRECTION, OUTPUT);
        pinMode(STEPPER_PIN_STEP, OUTPUT);
        digitalWrite(STEPPER_PIN_ENABLE, LOW);
        digitalWrite(STEPPER_PIN_DIRECTION, LOW);
        digitalWrite(STEPPER_PIN_STEP, LOW);
    }
    void loop() {
        while (stepper_enable && (0 < stepper_speed)) {
            digitalWrite(STEPPER_PIN_DIRECTION, (0 < stepper_direction) ? HIGH : LOW);
            digitalWrite(STEPPER_PIN_ENABLE, HIGH);
            digitalWrite(STEPPER_PIN_STEP, HIGH);
            delay(STEPPER_PULSE);
            digitalWrite(STEPPER_PIN_STEP, LOW);
            int pause = map(stepper_speed, COMMAND_MIN, COMMAND_MAX, STEPPER_MAX*2, STEPPER_MIN);
            delay(pause);
        }
        digitalWrite(STEPPER_PIN_ENABLE, LOW);
    }
} control_task;


class MonitorTask : public Task {
    protected:
    void loop() {
        Serial.printf("IP: %s  enable: %d  direction: %d  speed: %d\n", 
            WiFi.localIP().toString().c_str(), stepper_enable, stepper_direction, stepper_speed);
        delay(5000);
    }
} monitor_task;


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(115200);
    Serial.println("-------------------------------------------------");
    delay(1000);
    wifiManager.autoConnect(NODE_NAME);
    Scheduler.start(&server_task);
    Scheduler.start(&control_task);
    Scheduler.start(&monitor_task);
    Scheduler.begin();
}

void loop() {}
