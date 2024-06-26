/*
* Vzorovy kod od laskakit.cz pro LaskaKit ESP-VINDRIKTNING a cidlem CO2, teploty a vlhkosti SCD41
* Kod posle pres seriovy port (UART) a zaroven na server TMEP.cz
*
* TMEP.cz momentalne neumi zobrazovat ctyri veliciny najednou - prasnost, CO2, teplota, vlhkost. 
* Proto jsou zaregistrovana dve cidla zvlast - prasnost (cidlo PM1006) a CO2/teplota/vlhkost (cidlo SCD41)
*
* LaskaKit ESP-VINDRIKTNING (https://www.laskakit.cz/laskakit-esp-vindriktning-esp-32-i2c/)
* LaskaKit SCD41 Senzor CO2, teploty a vlhkosti vzduchu (https://www.laskakit.cz/laskakit-scd41-senzor-co2--teploty-a-vlhkosti-vzduchu/)
*
* Vytvoreno (c) laskakit.cz 2022
*
* Potrebne knihovny:
* https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library //SCD41
* https://github.com/bertrik/pm1006 //PM1006
*/

#include <WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

/* LaskaKit ESP-VINDRIKTNING - cidlo prasnosti PM1006 */
#include "pm1006.h"
/* LaskaKit ESP-VINDRIKTNING s čidlem CO2/teploty/vlhkosti SCD41 */
#include "SparkFun_SCD4x_Arduino_Library.h"

/* RGB adresovatelne LED */
#include <Adafruit_NeoPixel.h>

/* LaskaKit ESP-VINDRIKTNING - cidlo prasnosti PM1006 */
#define PIN_FAN 12 // spinani ventilatoru
#define RXD2 16 // UART - RX
#define TXD2 17 // UART - TX

/* Nastaveni RGB LED */
#define BRIGHTNESS 10
#define PIN_LED 25
#define PM_LED 0
#define TEMP_LED 1
#define CO2_LED 2

/*--------------------- UPRAV NASTAVENI ---------------------*/
const char* ssid = "SSID";
const char* password = "PASSWORD";

// vypln tvou domenu cidla, kterou sis zaregistroval na tmep.cz PRO PRASNOST (cidlo PM1006)
String serverNamePM = "http://TVOJE_DOMENA_pro_cidlo_PRASNOSTI.tmep.cz/index.php?";
// vypln tve GUID cidla pro prasnost
String GUID_PM = "TEBOU_ZVOLENE_GUID_PRO_PM1006";

// vypln tvou domenu cidla, kterou sis zaregistroval na tmep.cz PRO CO2 (pokud mas, cidlo SCD41)
String serverNameCO2 = "http://TVOJE_DOMENA_pro_cidlo_CO2_teplota_vlhkost.tmep.cz/index.php?";
// vypln tve GUID cidla pro SCD41 - CO2, teplota, tlak
String GUID_CO2 = "TEBOU_ZVOLENE_GUID_PRO_SCD41"; 
/*------------------------ KONEC UPRAV -----------------------*/

/* LaskaKit ESP-VINDRIKTNING - cidlo prasnosti PM1006, nastaveni UART2 */
static PM1006 pm1006(&Serial2);

/* LaskaKit ESP-VINDRIKTNING s čidlem CO2/teploty/vlhkosti SCD41 */
SCD4x SCD41;

/* RGB adresovatelne LED */
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(3, PIN_LED, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(PIN_FAN, OUTPUT); // Ventilator pro cidlo prasnosti PM1006
  Serial.begin(115200);
  Wire.begin();
  
  pixels.begin(); // WS2718
  pixels.setBrightness(BRIGHTNESS);

  delay(10);
  
  /*-------------- RGB adresovatelne LED - zhasni --------------*/
  pixels.setPixelColor(PM_LED, pixels.Color(0, 0, 0)); // R, G, B
  pixels.setPixelColor(TEMP_LED, pixels.Color(0, 0, 0)); // R, G, B
  pixels.setPixelColor(CO2_LED, pixels.Color(0, 0, 0)); // R, G, B
  pixels.show();  // Zaktualizuje barvu

  /*-------------- PM1006 - cidlo prasnosti ---------------*/
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // cidlo prasnosti PM1006

  /*------------- SCD41 - CO2, teplota, vlhkost -----------*/
  // inicializace
  //             begin, autokalibrace
  //               |      |
  if (SCD41.begin(false, true) == false)
  {
    Serial.println("SCD41 nenalezen.");
    Serial.println("Zkontroluj propojeni.");
    while(1)
      ;
  }
 
  // prepnuti do low power modu
  if (SCD41.startLowPowerPeriodicMeasurement() == true)  
  {
    Serial.println("Low power mod povolen.");
  }
  
  /*------------- Wi-Fi -----------*/
  WiFi.begin(ssid, password);
  Serial.println("Pripojovani");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Pripojeno do site, IP adresa zarizeni: ");
  Serial.println(WiFi.localIP());
}
 
void loop() {
  /*------------- SCD41 - CO2, teplota, vlhkost -----------*/
  int co2 = 0;
  float teplota = 0.0;
  int vlhkost = 0;
 
  while (!SCD41.readMeasurement()) // cekani na nova data (zhruba 30s)
  {
    delay(1);
  } 

  co2 = SCD41.getCO2();
  teplota = SCD41.getTemperature();
  vlhkost = SCD41.getHumidity();

  // odeslani hodnot pres UART
  Serial.print("Teplota: "); Serial.print(teplota); Serial.println(" degC");
  Serial.print("Vlhkost: "); Serial.print(vlhkost); Serial.println("% rH");
  Serial.print("CO2: "); Serial.print(co2); Serial.println(" ppm");

  /*-------------- PM1006 - cidlo prasnosti ---------------*/
  uint16_t pm2_5;
  digitalWrite(PIN_FAN, HIGH);
  Serial.println("Fan ON");
  delay(30000);

  while (!pm1006.read_pm25(&pm2_5)) 
  {
    delay(1);
  } 

  delay(100);
  digitalWrite(PIN_FAN, LOW);
  Serial.println("Fan OFF");

  // odeslani hodnot pres UART
  Serial.print("PM2.5: "); Serial.print(pm2_5); Serial.println(" ppm");
  
  
  /*-------------- RGB adresovatelne LED ---------------*/
  // CO2 LED
  if(co2 < 1000){
    pixels.setPixelColor(CO2_LED, pixels.Color(0, 255, 0)); // R, G, B
  }
  
  if((co2 >= 1000) && (co2 < 1200)){
    pixels.setPixelColor(CO2_LED, pixels.Color(128, 255, 0)); // R, G, B
  }
  
  if((co2 >= 1200) && (co2 < 1500)){
  pixels.setPixelColor(CO2_LED, pixels.Color(255, 255, 0)); // R, G, B
  }
  
  if((co2 >= 1500) && (co2 < 2000)){
    pixels.setPixelColor(CO2_LED, pixels.Color(255, 128, 0)); // R, G, B
  }
  
  if(co2 >= 2000){
    pixels.setPixelColor(CO2_LED, pixels.Color(255, 0, 0)); // R, G, B
  }

  // teplota LED
  if(teplota < 20.0){
    pixels.setPixelColor(TEMP_LED, pixels.Color(0, 0, 255)); // R, G, B
  }

  if((teplota >= 20.0) && (teplota < 23.0)){
    pixels.setPixelColor(TEMP_LED, pixels.Color(0, 255, 0)); // R, G, B
  }

  if(teplota >= 23.0){
    pixels.setPixelColor(TEMP_LED, pixels.Color(255, 0, 0)); // R, G, B
  }

  // PM LED
  if(pm2_5 < 30){
    pixels.setPixelColor(PM_LED, pixels.Color(0, 255, 0)); // R, G, B
  }
  
  if((pm2_5 >= 30) && (pm2_5 < 40)){
    pixels.setPixelColor(PM_LED, pixels.Color(128, 255, 0)); // R, G, B
  }
  
  if((pm2_5 >= 40) && (pm2_5 < 80)){
  pixels.setPixelColor(PM_LED, pixels.Color(255, 255, 0)); // R, G, B
  }
  
  if((pm2_5 >= 80) && (pm2_5 < 90)){
    pixels.setPixelColor(PM_LED, pixels.Color(255, 128, 0)); // R, G, B
  }
  
  if(pm2_5 >= 90){
    pixels.setPixelColor(PM_LED, pixels.Color(255, 0, 0)); // R, G, B
  }
  pixels.show();  // Zaktualizuje barvu

  /*------------ Odeslani hodnot na TMEP.cz ------------------*/
  if(WiFi.status()== WL_CONNECTED)
  {
    //GUID, nasleduje hodnota teploty, pro vlhkost "humV", pro CO2 "CO2" cidla SCD41
    String serverPathCO2 = serverNameCO2 + "" + GUID_CO2 + "=" + teplota + "&humV=" + vlhkost + "&CO2=" + co2; 
    sendhttpGet(serverPathCO2);

    delay(100);
    //GUID, nasleduje hodnota cidla prasnosti PM1006 a odeslani na druhou domenu
    String serverPathPM = serverNamePM + "" + GUID_PM + "=" + pm2_5; 
    sendhttpGet(serverPathPM);

  }
  else 
  {
    Serial.println("Wi-Fi odpojeno");
  }

  esp_sleep_enable_timer_wakeup(900 * 1000000); // uspani na 15 minut
  Serial2.flush();
  Serial.flush(); 
  delay(100);
  esp_deep_sleep_start();
}

// funcke pro odeslani dat na TMEP.cz
void sendhttpGet(String httpGet)
{
  HTTPClient http;
      
  // odeslani dat 
  String serverPath = httpGet; 
  
  // zacatek http spojeni
  http.begin(serverPath.c_str());
  
  // http get request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode>0) 
  {
    Serial.print("HTTP odpoved: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else 
  {
    Serial.print("Error kod: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}
