/*
 * Project SmartPlant
 * Description: A smart plant watering system
 * Author: Adrian Pijoan
 * Date: 16-MAR-2023
 */

#include <math.h>
#include <wire.h>
#include <Grove_Air_quality_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <DFRobotDFPlayerMini.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT/Adafruit_MQTT_SPARK.h>
#include <Adafruit_MQTT/Adafruit_MQTT.h>
#include <bitmaps.h>
#include <credentials.h>

const int BMEADDRESS = 0x76;
const int OLEDADDRESS = 0x3C;
const int OLED_RESET = D2;
const int LEDPIN = D3;
const int PUMPPIN = D5;
const int DUSTPIN = D6;
const int MOISTPIN = A2;
const int AQPIN = A4;

TCPClient TheClient;

SoftwareSerial mySoftwareSerial(10, 9); // RX, TX
DFRobotDFPlayerMini plantMp3Player;
AirQualitySensor plantAirQuality(AQPIN);
Adafruit_BME280 plantBme;
Adafruit_SSD1306 plantDisplay(OLED_RESET);
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY);

Adafruit_MQTT_Publish airQualityFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/airQuality");
Adafruit_MQTT_Publish dustFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/dustLevel");

void checkEnvironment();
void waterPlant();
void plantMood(float _airQuality, float _concentration, float _plantReading);
void MQTT_connect();
bool MQTT_ping();

SYSTEM_MODE(SEMI_AUTOMATIC);


void setup() {

    Wire.begin();
    Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);

    plantAirQuality.init();
    plantBme.begin(BMEADDRESS);
  
    plantDisplay.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);
    plantDisplay.setRotation(0);
    plantDisplay.clearDisplay();
    plantDisplay.display();

    //PINMODES HERE:

}

void loop() {

    MQTT_connect();
    checkEnvironment();
    MQTT_ping();

}

//check ambient temperature, air quality, and dust level
void checkEnvironment() {

  static int startTime;
  const int sampleTime = 30000;

  float ratio, concentration, airQuality;

  //Read air quality and dust sensors and print to serial monitor every 30 seconds
  if(millis()-startTime > sampleTime){

    airQuality = plantAirQuality.getValue();
    Serial.printf("Air Quality: %f\n", airQuality);
    airQualityFeed.publish(airQuality);

    ratio = lowPulseOccupancy/(sampleTime*10.0); //int percent from 0-100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62;

    // Serial.printf("%i, %f, %f\n\n", lowPulseOccupancy, ratio, concentration);
    Serial.printf("Dust level: %f\n", concentration);
    dustFeed.publish(concentration);
    lowPulseOccupancy = 0;
    startTime = millis();

    plantMood(airQuality, concentration);

  }

  waterPlant();

}

//water the plant either through soil moisture readings or with button on Adafruit desktop
void waterPlant();

//check the plant's mood using impedence sensors and environmental data
void plantMood(float _airQuality, float _concentration){

  int plantReading;
  int ledBrightness;
  int trackNum;
  int volume;

//create several switch cases based on environment

//create several switch cases based on plant "mood" from plant reading

  plantMp3Player.volume(volume);  //Set volume value. From 0 to 30
  plantMp3Player.play(trackNum);  //Play the mp3 num
  digitalWrite(LEDPIN, ledBrightness);

}

// void MQTT_publish() {

// }

void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}