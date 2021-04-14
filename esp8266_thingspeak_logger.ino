/*******************************************************************
 *  An example of how to use a custom reply keyboard markup.       *
 *                                                                 *
 *                                                                 *
 *  written by Vadim Sinitski                                      *
 *******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>

//------------------------------------------

//DS18B20
#define ONE_WIRE_BUS D1 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 8 //The maximum number of devices

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int numberOfDevices; //Number of temperature devices found
DeviceAddress devAddr[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
float tempDev[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevLast[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
char temperatureString[6];

//------------------------------------------
//WIFI
const char* ssid = "ssid";
const char* password = "password";

//Init thingspeak service
unsigned long myChannelNumber = 123456; //thingspeak channel number
const char * myWriteAPIKey = "KGKHJGMKJHGM"; //thingspeak write api key
unsigned long ts_mtbs = 300000; //mean time between scan messages (seconds)
long ts_lasttime;   //last time messages' scan has been done

float getTemperature(int sensId) {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(sensId);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

//Convert device id to String
String GetAddressToString(DeviceAddress deviceAddress){
  String str = "";
  for (uint8_t i = 0; i < 8; i++){
    if( deviceAddress[i] < 16 ) str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

//Setting the temperature sensor
void SetupDS18B20(){
  DS18B20.begin();

  Serial.print("Parasite power is: "); 
  if( DS18B20.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }
  
  numberOfDevices = DS18B20.getDeviceCount();
  Serial.print( "Device count: " );
  Serial.println( numberOfDevices );

  DS18B20.requestTemperatures();

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if( DS18B20.getAddress(devAddr[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20.getResolution( devAddr[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20.getTempC( devAddr[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }
}

void setup() {
 Serial.begin(115200);
 delay(100);
 Serial.println("You are in setup...");
 pinMode(LED_BUILTIN, OUTPUT);
  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  SetupDS18B20();
}

void updateThingspeak() {
  String reqString = "http://api.thingspeak.com/update?api_key="+String(myWriteAPIKey);
      HTTPClient http;  //Declare an object of class HTTPClient
      for (int i=0; i<=numberOfDevices-1; i++) {
        int fieldNum = i+1;
        Serial.print("fieldNum=");
        Serial.println(fieldNum);
        float temperature = getTemperature(i);
        dtostrf(temperature, 2, 1, temperatureString);        
        reqString += "&field"+String(fieldNum)+"="+temperatureString;
        Serial.print("temperature = ");
        Serial.println(temperatureString);
      }
      Serial.println(reqString);
      http.begin(reqString);  //Specify request destination
      int httpCode = http.GET();  //Send the request
      if (httpCode > 0) {  //Check the returning code
        String payload = http.getString();  //Get the request response payload
        Serial.print("response: ");
        Serial.println(payload);  //Print the response payload
      }
      http.end();  //Close connection
}

void loop() {
  //Если таймаут прошел, то измерим температуру и отправим данные на thingspeak.com
  if (millis() > ts_lasttime + ts_mtbs)  {
      ts_lasttime = millis();
      updateThingspeak();
  }
}
