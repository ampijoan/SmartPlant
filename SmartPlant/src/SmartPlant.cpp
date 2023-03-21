/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/adrianpijoan/Documents/IoT/SmartPlant/SmartPlant/src/SmartPlant.ino"
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
#include <IoTTimer.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT/Adafruit_MQTT_SPARK.h>
#include <Adafruit_MQTT/Adafruit_MQTT.h>
#include <bitmaps.h>
#include <credentials.h>

void setup();
void loop();
void plantMood(float _airQuality, float _concentration, float _soilMoisture);
#line 21 "/Users/adrianpijoan/Documents/IoT/SmartPlant/SmartPlant/src/SmartPlant.ino"
const int BMEADDRESS = 0x76;
const int OLEDADDRESS = 0x3C;
const int OLED_RESET = D2;
const int LEDPIN = D3;
const int PUMPPIN = D5;
const int DUSTPIN = D6;
const int TONEPIN = A3;
const int PLANTPIN = A1;
const int SOILMOISTPIN = A2;
const int AQPIN = A4;
const int sampleTime = 30000;

static unsigned int duration, lowPulseOccupancy;

int startTime, soilMoisture;
bool waterButtonState;
bool waterToggle;
float airQuality, concentration, ratio, subValue;

TCPClient TheClient;

DFRobotDFPlayerMini plantMp3Player;
AirQualitySensor plantAirQuality(AQPIN);
Adafruit_BME280 plantBme;
Adafruit_SSD1306 plantDisplay(OLED_RESET);
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY);
IoTTimer waterTimer;

Adafruit_MQTT_Subscribe waterButtonFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/waterButton"); 
Adafruit_MQTT_Publish airQualityFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/airQuality");
Adafruit_MQTT_Publish dustFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/dustLevel");

void plantMood(float _airQuality, float _concentration, float _plantReading);
void MQTT_connect();
bool MQTT_ping();

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

    Wire.begin();
    Serial1.begin(9600);
    Serial.begin(115200);
    
    waitFor(Serial.isConnected, 10000);

    plantMp3Player.begin(Serial1);
    plantMp3Player.outputDevice(DFPLAYER_DEVICE_SD);

    // if (!plantMp3Player.begin(Serial1)) {  //Use softwareSerial to communicate with mp3.
    //   Serial.println(F("Unable to begin:"));
    //   Serial.println(F("1.Please recheck the connection!"));
    //   Serial.println(F("2.Please insert the SD card!"));
    //   while(true);
    //   }

    WiFi.on();
    WiFi.connect();

    mqtt.subscribe(&waterButtonFeed);

    plantAirQuality.init();
    plantBme.begin(BMEADDRESS);
  
    plantDisplay.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);
    plantDisplay.setRotation(0);
    plantDisplay.clearDisplay();
    plantDisplay.display();

    pinMode(TONEPIN,OUTPUT);
    pinMode(PUMPPIN,OUTPUT);
    pinMode(LEDPIN,OUTPUT);
    pinMode(PLANTPIN,INPUT);
    pinMode(DUSTPIN,INPUT);
    pinMode(AQPIN,INPUT);
    pinMode(SOILMOISTPIN,INPUT);

    startTime = millis();

}

void loop() {

  MQTT_connect();
  MQTT_ping();

  //pulled this code from Subscribe Publish, not totally sure on it??
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &waterButtonFeed) {
      subValue = atoi((char *)waterButtonFeed.lastread);
    }
  }

  waterButtonState = subValue;

  duration = pulseIn(DUSTPIN, LOW);
  lowPulseOccupancy = lowPulseOccupancy + duration;

  //Read air quality and dust sensors and print to serial monitor / send to adafruit desktop every 30 s
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

    //check soil moisture
    soilMoisture = analogRead(SOILMOISTPIN);
    Serial.printf("Soil moisture: %i\n", soilMoisture);

    if(soilMoisture > 3000 ){ 
      digitalWrite(PUMPPIN,HIGH);
      Serial.printf("pumping!!\n");
      waterTimer.startTimer(500);

  }

    //display env data to OLED
  }
    
  plantMood(airQuality, concentration, soilMoisture);



  if(waterButtonState){
    digitalWrite(PUMPPIN,HIGH);
    Serial.printf("button received\n");
    waterTimer.startTimer(500);
  }

  if(waterTimer.isTimerReady()){
    digitalWrite(PUMPPIN,LOW);
  }

}

//check the plant's mood using impedence sensors and environmental data
void plantMood(float _airQuality, float _concentration, float _soilMoisture){

  int plantReading;
  int ledBrightness;
  int toneVal;
  int trackNum;
  int volume;

//play tones no matter what then:

plantMp3Player.volume(30);
plantMp3Player.play(1);
// Serial.printf("playing MP3!");
plantReading = analogRead(PLANTPIN);
Serial.printf("Plant value: %i", plantReading);
tone(TONEPIN,plantReading,1000);

//create several switch cases based on environment && plant mood ? (I think I did it with ONE if/else...but we'll see)
//choose MP3 set "happy" "bad" or "neutral"
//print to OLED
//create a timer that waits a random amount of time before speaking

  //toneVal = read the tonepin
  //tone(toneVal,1000);

  // if(_airQuality > 100|| _concentration > 10000 || _soilMoisture < 30000){            ||room is too cold 
  //  plantMp3Player.setVolume(30);
  //   plantMp3Player.playFolderTrack(1,random(1,random(8,25))); //Play mp3 from NEUTRAL and BAD states
  //   digitalWrite(LEDPIN,ledBrightness); //turn on LED when plant is speaking
  // }

  // //plant's default state is happy, unless something about its environment is bothering it
  // else{
  //   plantMp3Player.setVolume(30);
  //   plantMp3Player.playFolderTrack(1,random(1,20));  //Play mp3 from GOOD and NEUTRAL states
  // }

}


//Brian's MQTT functions
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