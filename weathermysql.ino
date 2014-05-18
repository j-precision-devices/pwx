//Included Libraries
#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"
#include "sha1.h"
#include "mysql.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
        return 1.8 * celsius + 32;
}

// fast integer version with rounding
//int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}

//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
        return celsius + 273.15;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
        // (1) Saturation Vapor Pressure = ESGG(T)
        double RATIO = 373.15 / (273.15 + celsius);
        double RHS = -7.90298 * (RATIO - 1);
        RHS += 5.02808 * log10(RATIO);
        RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
        RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
        RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
        double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558 - T);
}

// DHT Sensor Setup
#define DHTPIN 3 // DHT Sensor attached to digital pin 7
#define DHTTYPE DHT22 // This is the type of DHT Sensor (Change it to DHT11 if you're using that model)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT object

// BMP085 Sensor Setup
Adafruit_BMP085 bmp;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Ethernet & Server Setup
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 1, 175 }; // google will tell you: "public ip address"
char serverName[] = "192.168.1.100"; 

void startEthernet(){
    Serial.println("... Initializing ethernet");
    if(Ethernet.begin(mac) == 0)
      {
        Serial.println("... Failed to configure Ethernet using DHCP");
        // no point in carrying on, so do nothing forevermore:
        // try to congifure using IP address instead of DHCP:
        Ethernet.begin(mac, ip);
      }
    Serial.println("... Done initializing ethernet");
}

int photocellPin0 = 0;     // the cell and 10K pulldown are connected to a0
int photocellReading0;     // the analog reading from the analog resistor divider
float Res0=9.83;              // Resistance in the circuit of sensor 0 (KOhms)
// depending of the Resistance used, you could measure better at dark or at bright conditions.
// you could use a double circuit (using other LDR connected to analog pin 1) to have fun testing the sensors.
// Change the value of Res0 depending of what you use in the circuit

void setup() {
  Serial.begin( 9600 );

  startEthernet();
  

  dht.begin();
  if (!bmp.begin()) {
//	Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	while (1) {}

// Start up the library
  sensors.begin();
  
  }  

  
  delay(1000);
}

void loop() {
  EthernetClient client;
  float  temp0, temp1, humidity, temp2, pressure, temp_avg, tdp;
  
  if( client.connect(serverName, 80) ) 
    {    

  photocellReading0 = analogRead(photocellPin0);   // Read the analogue pin
  float Vout0=photocellReading0*0.0048828125;      // calculate the voltage
  int lux=1000/(Res0*((5-Vout0)/Vout0));           // calculate the Lux CHNG JN
        
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  sensors.requestTemperatures(); // Send the command to get temperatures
  
    
      temp0 = (sensors.getTempCByIndex(0));
      temp1 = dht.readTemperature();
      humidity = dht.readHumidity();
      temp2 = bmp.readTemperature();
      pressure = bmp.readPressure();
      temp_avg = (temp0 + temp1 + temp2) / 3;  // Average 
      tdp      = (dewPoint(temp_avg, humidity));
      
      
      Serial.print(temp0);
      Serial.println(" C");
      Serial.print(temp1);
      Serial.println(" C");
      Serial.print(humidity);
      Serial.println(" %RH");
    
    Serial.print(temp2);
    Serial.println(" C");    
    Serial.print(pressure);  // Pa to "Hg conversion /(3386.389)
    Serial.println(" mb");    
    Serial.print(temp_avg);
    Serial.println(" C");    
    Serial.print(tdp);
    Serial.println(" C");  
    Serial.print("Luminosidad 0: ");  // Print the measurement (in Lux units) in the screen
    Serial.print(lux); //Write the value of the photoresistor to the serial monitor.
    Serial.println(" lux");  

      client.print( "GET /weather_data/add.php?");
      client.print("temp0=");
      client.print(temp0);
      client.print("&");
      client.print("temp1=");
      client.print(temp1);
      client.print("&");
      client.print("humidity=");
      client.print(humidity);
  
    client.print("&");
    client.print("temp2=");
    client.print(temp2);
    client.print("&");
    client.print("pressure=");
    client.print(pressure);  // Pa to "Hg conversion    /(3386.389)
    client.print("&");
    client.print("temp_avg=");
    client.print(temp_avg);
    client.print("&");
    client.print("tdp=");
    client.print(tdp);
    client.print("&");
    client.print("lux=");
    client.print(lux);
    
      client.println( " HTTP/1.1");
      client.println( "Host: 192.168.1.100" );
      client.println( "Content-Type: application/x-www-form-urlencoded" );
      client.println( "Connection: close" );
      client.println();
      client.println();
      client.stop();
    }
    

    
  delay(60000); //delay updates for 1 minute
}
