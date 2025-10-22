#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include "time.h"

#define LED_PIN 2   // Most ESP32 DevKitC boards have the onboard LED on GPIO 2

// WIFI setups
const char* WIFI_SSID = "Uniovi-i40";
const char* WIFI_PASSWORD = "1000000001";

// MQTT setups
const char* MQTT_HOST = "0.tcp.eu.ngrok.io";
const int MQTT_PORT = 17068;
const char* MQTT_USERNAME = "admin";
const char* MQTT_PASSWORD = "password";
const char* MQTT_CLIENT_ID = "ARD01";
const char* MQTT_TOPIC_SENSOR = "sensors/ACC01/data";
const char* MQTT_TOPIC_DEVICE = "devices/ARD01/data";
const int MQTT_FREQUENCY = 5000;

// Time setups
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;      // Spain = UTC+1
const int   daylightOffset_sec = 7200; // adjust for DST winter timezone

// Arduino sensor and clients
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_MPU6050 imu;
sensors_event_t a, g, temp;

// Define a struct to hold sensor readings
struct IMUData {
  float ax, ay, az;   // acceleration (m/s^2)
  float gx, gy, gz;   // gyro (rad/s)
  float temp;         // temperature (°C)
};

void setup_wifi() {
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED_PIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(LED_PIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void setup_mqtt_client() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
}

void setup_imu_sensor() {
  if (!imu.begin()) {
    Serial.println("Failed to find MPU6050 IMU");
    while (1) { delay(10); }
  }

  Serial.println("MPU6050 Found!");

  imu.setAccelerometerRange(MPU6050_RANGE_8_G);
  imu.setGyroRange(MPU6050_RANGE_500_DEG);
  imu.setFilterBandwidth(MPU6050_BAND_21_HZ);  
}

void setup_time() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void setup() {
  // Initialize serial speed comunication
  Serial.begin(115200);

  while (!Serial)
    delay(10);

  // Initialize the LED_PIN pin as an output
  pinMode(LED_PIN, OUTPUT);

  // setup wifi  
  setup_wifi();
  
  // setup mqtt client
  setup_mqtt_client();

  // setup imu sensor
  setup_imu_sensor();

  // setup real time
  setup_time();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
            
      // ... and resubscribe
      client.subscribe(MQTT_TOPIC_DEVICE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      String mess = "try again in ";
      mess.concat(MQTT_FREQUENCY);
      mess.concat(" seconds"); 
      Serial.println(mess);
      
      // Wait MQTT_FRECUENCY seconds before retrying
      delay(MQTT_FREQUENCY);
    }
  }
}

IMUData getSensorData() {  
  // get imu sensor data
  imu.getEvent(&a, &g, &temp);
  
  Serial.print("Accel X: "); Serial.print(a.acceleration.x);
  Serial.print(", Y: "); Serial.print(a.acceleration.y);
  Serial.print(", Z: "); Serial.print(a.acceleration.z); Serial.println(" m/s^2");

  Serial.print("Gyro X: "); Serial.print(g.gyro.x);
  Serial.print(", Y: "); Serial.print(g.gyro.y);
  Serial.print(", Z: "); Serial.print(g.gyro.z); Serial.println(" rad/s");

  Serial.print("Temp: "); Serial.print(temp.temperature); Serial.println(" °C");

  delay(500);

  // create data from sensor
  IMUData data;
  data.ax = a.acceleration.x;
  data.ay = a.acceleration.y;
  data.az = a.acceleration.z;

  data.gx = g.gyro.x;
  data.gy = g.gyro.y;
  data.gz = g.gyro.z;

  data.temp = temp.temperature;

  return data;
}

String getTimeNow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "N/A";
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buffer);
}

void loop() {
  // check connection to reconnect
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();

  // get sensor data
  IMUData imuData = getSensorData();

  // Build JSON from sensor data
  StaticJsonDocument<200> doc;
  doc["accX"] = imuData.ax;
  doc["accY"] = imuData.ay;
  doc["accZ"] = imuData.az;
  doc["gycX"] = imuData.gx;
  doc["gycY"] = imuData.gy;
  doc["gycZ"] = imuData.gz;
  doc["temp"] = imuData.temp;
  //doc["timestamp"] = millis();
  doc["timestamp"] = getTimeNow();

  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  // Publish JSON to MQTT topic
  client.publish(MQTT_TOPIC_SENSOR, buffer, n);  

  Serial.println(buffer); // Debug
  delay(1000);   
}