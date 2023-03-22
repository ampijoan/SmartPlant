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
float airQuality, temp, concentration, ratio, subValue;

TCPClient TheClient;

DFRobotDFPlayerMini plantMp3Player;
AirQualitySensor plantAirQuality(AQPIN);
Adafruit_BME280 plantBme;
Adafruit_SSD1306 plantDisplay(OLED_RESET);
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY);
IoTTimer waterTimer;
IoTTimer plantSpeakTimer;

Adafruit_MQTT_Subscribe waterButtonFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/waterButton"); 
Adafruit_MQTT_Publish airQualityFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/airQuality");
Adafruit_MQTT_Publish dustFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/dustLevel");
Adafruit_MQTT_Publish tempFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/roomTemp");
Adafruit_MQTT_Publish needsWaterFeed = Adafruit_MQTT_Publish(&mqtt,AIO_USERNAME "/feeds/needsWater");

void plantMood(float _airQuality, float _concentration, float _plantReading);
void MQTT_connect();
bool MQTT_ping();

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

    Wire.begin();
    Serial1.begin(9600);
    Serial.begin(9600);
    
    waitFor(Serial.isConnected, 10000);

    WiFi.on();
    WiFi.connect();

    mqtt.subscribe(&waterButtonFeed);

    plantMp3Player.begin(Serial1);
    plantMp3Player.outputDevice(DFPLAYER_DEVICE_SD);

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

  //Using button on Adafruit dashboard to water plant
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &waterButtonFeed) {
      subValue = atoi((char *)waterButtonFeed.lastread);
    }
  }

  waterButtonState = subValue;

  if(waterButtonState){
    digitalWrite(PUMPPIN,HIGH);
    Serial.printf("button received\n");
    waterTimer.startTimer(500);
  }

  if(waterTimer.isTimerReady()){
    digitalWrite(PUMPPIN,LOW);
  }

  duration = pulseIn(DUSTPIN, LOW);
  lowPulseOccupancy = lowPulseOccupancy + duration;

  //Read air quality and dust sensors and print to serial monitor / send to adafruit desktop every 30 s
  if(millis()-startTime > sampleTime){

    temp = (plantBme.readTemperature() * 1.8) + 32;
    tempFeed.publish(temp);
    // Serial.printf("Temperature: %f\n",temp);

    airQuality = plantAirQuality.getValue();
    // Serial.printf("Air Quality: %f\n", airQuality);
    airQualityFeed.publish(airQuality);

    ratio = lowPulseOccupancy/(sampleTime*10.0); //int percent from 0-100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62;
    // Serial.printf("Dust level: %f\n", concentration);
    dustFeed.publish(concentration);
    lowPulseOccupancy = 0;
    startTime = millis();

    //check soil moisture
    soilMoisture = analogRead(SOILMOISTPIN);
    needsWaterFeed.publish(soilMoisture);
    // Serial.printf("Soil moisture: %i\n", soilMoisture);

    if(soilMoisture > 3000 ){ 
      digitalWrite(PUMPPIN,HIGH);
      // Serial.printf("pumping!!\n");
      waterTimer.startTimer(500);
    }

    //print environmental data to the OLED
    plantDisplay.clearDisplay();
    plantDisplay.display();
    plantDisplay.setTextColor(WHITE);
    plantDisplay.setTextSize(1);
    plantDisplay.setCursor(0,0);
    plantDisplay.printf("\nT: %.2f\nDust: %.2f\nAQ: %.2f\nSoil Moisture: %i",temp,concentration,airQuality,soilMoisture);
    plantDisplay.display();

  }

  plantMood(airQuality, concentration, soilMoisture);

}

//check the plant's mood using impedence sensors and environmental data and play sounds depending on plant's "mood"
void plantMood(float _airQuality, float _concentration, float _soilMoisture){

  int plantReading, trackId;
  static bool speechToggle = true;
  static int plantSpeakWait;

  //play tones based on impedence reading no matter what
  plantReading = analogRead(PLANTPIN);
  tone(TONEPIN,plantReading,10000);

  if(speechToggle){
    plantSpeakWait = random(500,3000);
    plantSpeakTimer.startTimer(plantSpeakWait);
    speechToggle = false;
    digitalWrite(LEDPIN,LOW);
  }

  //if one of these values is "bad", the plant is unhappy
  if (plantSpeakTimer.isTimerReady()){
    if(_airQuality > 100|| _soilMoisture > 3000 || temp < 32){    //_concentration > 10000 |
      plantMp3Player.volume(20);
      trackId = random(4,10);
      plantMp3Player.play(trackId); //Pick an mp3 from NEUTRAL and BAD states
      digitalWrite(LEDPIN,HIGH); //turn on LED when plant is speaking
      speechToggle = true;
      Serial.printf("%i\n",trackId);
  }

    // //plant's default state is happy, unless something about its environment is bothering it
    else{
      plantMp3Player.volume(20);
      trackId = random(1,7); //Pick mp3 an from GOOD and NEUTRAL states
      plantMp3Player.play(trackId); 
      digitalWrite(LEDPIN,HIGH); //turn on LED when plant is speaking 
      speechToggle = true;
      Serial.printf("%i\n",trackId);

    }

  }

    //print something to OLED, probably bitmaps

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