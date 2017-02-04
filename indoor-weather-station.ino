/******************************************************************************
* Copyright (c) 2017 Ashwin V Kumar
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************/
/**
 * @file        indoor-weather-station.ino
 *
 * @brief       An ESP8266 based indoor weather station 
 *
 * @date        Jan 16, 2017, 01:06:04 AM
 *
 * @details     The ESP8266 acts as an MQTT sensor node which publishes 
 *              temperature & humidity topics. 
 */

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

/* Pin assignments */
#define LED_PIN   2
#define DHT_PIN   4

/* WiFi constants */
#define WLAN_SSID         "Jataayu"
#define WLAN_PASS         "2698730108"

/* MQTT constants */
#define MQTT_BROKER       "ashi-server"   // host name or IP address
#define MY_MQTT_CLIENT_ID "ESP8266 Client"
#define TOPIC_HUMIDITY    "humidity"
#define TOPIC_TEMP_C      "tempC"
#define TOPIC_TEMP_F      "tempF"
#define MSG_LENGTH        50

/* Sensor variables */
float humidity, tempC, tempF, hif, hic;
char msg[MSG_LENGTH] = {};

/* Instances */
DHT dht( DHT_PIN, DHT11 );
WiFiClient espWifiClient;
PubSubClient espPubSubClient( espWifiClient );


/**
 * @brief   Initialize all components in the project
 */
void setup()
{
  /* Wait till the sensor completely powers up */
  delay( 2000 );
  
  Serial.begin( 115200 );
  Serial.println( "Initializing indoor weather station..." );

  pinMode( LED_PIN, OUTPUT );
  
  EEPROM.begin( 512 );

  /* Start the DHT sensor */
  dht.begin();

  /* Initialize Wifi and connect to WLAN */
  setup_wifi();

  /* Configure MQTT client */
  espPubSubClient.setServer( MQTT_BROKER, 1883 );
  espPubSubClient.setCallback( sub_callback );
}


/**
 * @brief   Infinite loop
 */
void loop() 
{
  /* Read relative humidity */
  humidity = dht.readHumidity();
  /* Read temperature as Celsius (the default) */
  tempC = dht.readTemperature();
  /* Read temperature as Fahrenheit (isFahrenheit = true) */
  tempF = dht.readTemperature(true);

  /* Check if any reads failed and exit early (to try again). */
  if( isnan(humidity) || isnan(tempC) || isnan(tempF) ) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  /* Compute heat index in Fahrenheit (the default) */
  hif = dht.computeHeatIndex( tempF, humidity );
  /* Compute heat index in Celsius (isFahreheit = false) */
  hic = dht.computeHeatIndex( tempC, humidity, false );

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.print(" *C ");
  Serial.print(tempF);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");
 
  if( !espPubSubClient.connected() ) 
  {
    connect_mqtt();
  }
  espPubSubClient.loop();

  snprintf( msg, MSG_LENGTH, "%2f", humidity );
  espPubSubClient.publish( TOPIC_HUMIDITY, msg );
}


/**
 * @brief   
 */
void sub_callback( char* topic, byte* payload, unsigned int length )
{
  Serial.print( "Message arrived [" );
  Serial.print( topic );
  Serial.print( "] " );
  for ( int i = 0; i < length; i++ ) 
  {
    Serial.print( (char)payload[i] );
  }
  Serial.println();

  /* Switch on the LED if a 1 was received as first character */
  if ((char)payload[0] == '1')
  {
    digitalWrite( LED_PIN, LOW );     // Active-low LED
  } 
  else 
  {
    digitalWrite( LED_PIN, HIGH);     // Active-low LED
  }
}


/**
 * @brief   Connect to MQTT broker
 */
void connect_mqtt( void )
{
  /* Loop until we're connected */
  while( !espPubSubClient.connected() )
  {
    Serial.print( "Attempting MQTT connection..." );
    if( espPubSubClient.connect( MY_MQTT_CLIENT_ID ) )
    {
      Serial.println( "Connected" );
    }
    else
    {
      Serial.print( "Failed to connect, rc=" );
      Serial.print( espPubSubClient.state() );
      Serial.println( " try again in 5 seconds" );
      /* Wait 5 seconds before retrying */
      delay( 5000 );
    }
  }
}

/**
 * @brief   Initialize WiFi and connect to WLAN
 */
void setup_wifi( void )
{
  delay( 10 );
  
  /* Connect to the WLAN */
  Serial.println();
  Serial.print( "Connecting to " );
  Serial.println( WLAN_SSID );

  WiFi.begin( WLAN_SSID, WLAN_PASS );

  while( WiFi.status() != WL_CONNECTED ) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println( "" );
  Serial.println( "WiFi connected" );
  Serial.println( "IP address: " );
  Serial.println( WiFi.localIP() );
}
