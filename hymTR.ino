/*
 * hymTR 2020
 * Baris DINC - TA7W
 * APRS Tracker
 */

#include <LibAPRS.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

#define DEBUG 1

//MAKRO fonksiyonlar
void print_debug(char* mesaj) {
#ifdef DEBUG
  Serial.print(mesaj);
#endif
}

//GPS
#define GPS_RX 11 // GPS'in TX portuna bagli olan Pin
#define GPS_TX 12 // GPS'in RX portuna bagli olan Pin 

//APRS Tracker
#define OPEN_SQUELCH  false
#define ADC_REFERENCE REF_5V


//Donanim Ayarlari
#define DAC_PIN_1 4 // 8K8 Direnc 
#define DAC_PIN_2 5 // 3K9 Direnc
#define DAC_PIN_3 6 // 2K1 Direnc
#define DAC_PIN_4 7 // 1K  Direnc
#define PTT_PIN   3 // N Kanal FET Gate'ine bagli

//Ayarlanabilir Parametreler
char   Mesaj[]        = "hymTR APRS Tracker /A=001234";
char   CagriIsareti[] = "TA7W";
int    CagriSSID    = 9;
char   Sembol       = '>';
int    GPS_PortHiz  = 9600;
bool   LokasyonGonder = true;
bool   YukseklikGonder= false;
bool   BataryaGonder  = false;
int    akilliBeacon   = 1;
int    preambleSure   = 350;
int    txDelay        = 50;


// GPS ve Baglanti Noktasi 
SoftwareSerial gpsSerial(GPS_RX, GPS_TX); 
Adafruit_GPS GPS(&gpsSerial);


//Ayarlarin EEPROM'da saklanmasi icin  

void EepromYaz() {
//TODO: doldurulacak
}

//Ayarlarin EEPROM'dan okunmasi icin
void EepromOku() {
//TODO: doldurulacak  
}


void setup() {
  //EepromOku(); //ilk baslangicta hafizadaki degerleri okuyoruz
  Serial.begin(115200); //PC haberlesme port hizi 
  
  pinMode( 13, OUTPUT ); //bunu ne icin kullandigimiza bakilacak

  //APRS kutuphanesini ayarla
  APRS_init(ADC_REFERENCE, OPEN_SQUELCH);
  APRS_setPreamble(preambleSure);
  APRS_setTail(txDelay);
  APRS_setSymbol(Sembol);
  APRS_setCallsign(CagriIsareti, CagriSSID);

    
  Serial.println("hymTR APRS Tracker");
  
  //GPS portunu ve GPS in gonderecegi cumleleri ayarla
  GPS.begin(GPS_PortHiz);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);

  //GPS den okunabilecek mesajlar
  /*
  *
  * hour
  * minute
  * seconds
  * day
  * month
  * year
  * fix
  * fixquality
  * latitude
  * lat
  * longitude
  * lon
  * speed
  * angle
  * altitude
  * satellites
  */
 
  // Timer0 interrupt kuruluyor : 0x7F
  OCR0A = 0x7F;
  TIMSK0 |= _BV(OCIE0A);
  
  delay(1000);
  gpsSerial.println(PMTK_Q_RELEASE);
}
 
// Timer0 ISR icinde GPS verileri okunacak
//TODO: Bu mantik dogru degil, degistirilecek
SIGNAL(TIMER0_COMPA_vect) {
  //char c = GPS.read();
  GPS.read();
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))
      return;
  }
}


void lokasyonCevirGuncelle() {
  char msgLat[10];
  char msgLon[10];
  char strLat[10];
  char strLon[10];
  //Lokasyon cevrimleri
  dtostrf(GPS.latitude,  6, 2, strLat);
  dtostrf(GPS.longitude, 6, 2, strLon);
  
  sprintf(msgLat, "%s%c", strLat, GPS.lat);
  if( GPS.longitude >= 10000 ) 
   sprintf(msgLon,"%s%c", strLon, GPS.lon);
  else
   sprintf(msgLon,"0%s%c",strLon, GPS.lon);
  //APRS icine mesajlari yaz
  APRS_setLat(msgLat);
  APRS_setLon(msgLon);
  APRS_sendLoc(Mesaj, strlen(Mesaj));
  //Serial.println((String)"Pozisyon Lat: " + msgLat + " Lon: " + msgLon + " Mesaj: " + Mesaj);
}
 
void lokasyonGuncelle() {
    
  gpsSerial.end();
 
  // TX
  //APRS_sendLoc(comment, strlen(comment));
 
  // read blue TX LED pin and wait till TX has finished (PB1, Pin 9)
  //while(bitRead(PORTB,1));
 
  //start soft serial again
  GPS.begin(9600);
}

void loop() {

  while (!GPS.fix) {
    Serial.println("GPS fix almadi.. bekliyoruz...");
    delay(1000);
  }
  
  //TODO: Buraya SmartBeacon ozelligi detaylandirilacak
  int gpsHizimizKMH = GPS.speed * 1.852; //TODO: bu degeri GPS mesajindan alacagiz GPS.speed
  int beaconSikligi = 300; //Varsayilan deger 5 dakiakda 1 beacon
  if (gpsHizimizKMH >= 10) beaconSikligi = 240; //10 kilometre/saat hizi uzerinde 4 dakikada 1 beacon 
  if (gpsHizimizKMH >= 30) beaconSikligi = 180; //30 kilometre/saat hizi uzerinde 3 dakikada 1 beacon
  if (gpsHizimizKMH >= 50) beaconSikligi = 120; //50 kilometre/saat hizi uzerinde 2 dakikada 1 beacon
  if (gpsHizimizKMH >=100) beaconSikligi = 60;  //100 kilometre/saat hizi uzerinde 1 dakikada 1 beacon
  Serial.println(GPS.seconds);
//BURAYI test icin 1 dakika yapiyoruz
  beaconSikligi = 60;
  
  int dakika = GPS.minute;
  int saniye = GPS.seconds;
  int sure = dakika * 60 + saniye;
  if ( (sure % beaconSikligi) <= 2 ) lokasyonCevirGuncelle(); //TODO: fix ilk 2 saniye

}

// libAPRS icin callback fonksiyonu
void aprs_msg_callback(struct AX25Msg *msg) {
}
