/*
 * hymTR 2020
 * Baris DINC - TA7W
 * 10 Ocak 2020
 * APRS Tracker
 */
//Pin tanimlamalari kutuphane include larindan once yapilmalidir.
#define RX_PIN 11  //GPS TX pin 
#define TX_PIN 12  //GPS RX pin

#include <LibAPRS_Tracker.h>
#include <NMEAGPS.h>
#include <GPSport.h>
#include <EEPROM.h>

/*
 * Notlar:
 * NeoGPS kutuphanesinde su duzenlemeleri yapiniz;
 *  GPSport.h icinde #include <NeoSWSerial.h> satirini acip #include <AltSoftSerial.h> satirini kapatiniz
 *  NMEAGPS_cfg.h icinde NMEAGPS_INTERRUPT_PROCESSING define satirini acmaniz ya da burda tanimlamaniz gerekir
 */

//Genel Tanimlamalar
#define VERSIYON "01012020a"

//Kutuphane kontrolleri
#if !defined(GPS_FIX_TIME) | !defined(GPS_FIX_DATE)
  #error NeoGPS kutuphanesinde GPSfix_cfg.h icinde  GPS_FIX_TIME ve DATE define edilmelidir
#endif
#if !defined(NMEAGPS_PARSE_RMC) & !defined(NMEAGPS_PARSE_ZDA)
  #error You must define NMEAGPS_PARSE_RMC or ZDA in NMEAGPS_cfg.h!
#endif

NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values
char comment []= "TAMSAT hymTR APRS Tracker Test";
#define CONV_BUF_SIZE 16
static char conv_buf[CONV_BUF_SIZE];

struct APRS_Ayarlari {
  char APRS_CagriIsareti[7];
  char APRS_CagriIsaretiSSID;
  char APRS_Destination[7];
  char APRS_DestinationSSID;  
  char APRS_Path1[7];
  char APRS_Path1SSID;
  char APRS_Path2[7];
  char APRS_Path2SSID;
  char APRS_Sembolu;
  char APRS_SembolTabi;
  byte APRS_BeaconTipi;    //0=sure beklemeli, 1=Smart Beacon
  unsigned long APRS_BeaconSuresi;
  unsigned int  APRS_GPSSeriHizi;    
  char APRS_Mesaj[41];
  unsigned int CheckSum;    //Cagri isaretinin byte toplamini kullaniyoruz
};
APRS_Ayarlari Ayarlar;

//forward declarations - fonksiyon tanimlarimiz
void KomutSatiriCalistir();
char* deg_to_nmea(long deg, boolean is_lat);
void eepromOku();
void eepromaYaz();
void VarsayilanAyarlar();
void konfigurasyonYazdir();
void parametreOku(char *szParam, int iMaxLen);
bool seridenAl();
void seriyeGonder();

static void GPSisr( uint8_t c )
{
  gps.handle( c );
} // GPSisr

void setup()
{
  eepromOku();
  APRS_init();
  char myCALL[] = "TA7W";
  char Lat[] = "2134.00N";
  char Lon[] = "01234.00E";
  APRS_setCallsign(myCALL, 9);
  APRS_setLat(Lat);
  APRS_setLon(Lon);
  //APRS_setPreamble(1000);
  //APRS_setTail(50);

  DEBUG_PORT.begin(115200);
  while (!DEBUG_PORT);
  DEBUG_PORT.print( F("hymTR APRS Tracker\n") );
  gpsPort.attachInterrupt( GPSisr );
  gpsPort.begin(9600);
}

int getGpsData(boolean *newDataLoc)
{
 while (gps.available( gpsPort )) {
    fix = gps.read();
    if (fix.valid.location && fix.valid.time && fix.valid.altitude) {
      *newDataLoc = true;
      return fix.dateTime.minutes*60+fix.dateTime.seconds;
    }
  }
return -1; 
}



void loop()
{
  boolean newData = false;
  int gpsMinSec = getGpsData(&newData);
  //if (gps.available()) {
    // Print all the things!
    //trace_all( DEBUG_PORT, gps, gps.read() );
  //}

    if (gps.overrun()) {
    gps.overrun( false );
    DEBUG_PORT.println( F("DATA OVERRUN: took too long to print GPS data!") );
  }
  
  if (newData && ((gpsMinSec % 20) == 0))
  {
    newData = false;
    DEBUG_PORT.println("sending....");
    APRS_setLat((char*)deg_to_nmea(fix.latitude(), true));
    APRS_setLon((char*)deg_to_nmea(fix.longitude(), false));
    char commentS[40]="                                       ";
//    snprintf(commentS,"/A=000000 %s",comment);
    snprintf(commentS,sizeof(commentS),"/A=000000 %s",comment);
    DEBUG_PORT.println(commentS);
    APRS_sendLoc(commentS, strlen(commentS));
    while(bitRead(PORTB,5)); //Wait for transmission to be completed
    DEBUG_PORT.println("sent....");
    //delay(25000);
  }
  //DEBUG_PORT.print(".");

//TODO: bunu Interrupt a tasiyacagim
  /*
   * Burada seri porttan komutlari okuyoruz
   * TODO: bunu interrupt a tasimaliyiz
   */
  while (DEBUG_PORT.available())
  {
    char komut = DEBUG_PORT.read();
     if (komut == '!') {
      KomutSatiriCalistir();
      }
  }


}

char* deg_to_nmea(long deg, boolean is_lat) {
  bool is_negative=0;
  if (deg < 0) is_negative=1;

  // Use the absolute number for calculation and update the buffer at the end
  deg = labs(deg);

  unsigned long b = (deg % 1000000UL) * 60UL;
  unsigned long a = (deg / 1000000UL) * 100UL + b / 1000000UL;
  b = (b % 1000000UL) / 10000UL;

  conv_buf[0] = '0';
  // in case latitude is a 3 digit number (degrees in long format)
  if( a > 9999) {
    snprintf(conv_buf , 6, "%04lu", a);
  } else {
    snprintf(conv_buf + 1, 5, "%04lu", a);
  }

  conv_buf[5] = '.';
  snprintf(conv_buf + 6, 3, "%02lu", b);
  conv_buf[9] = '\0';
  if (is_lat) {
    if (is_negative) { conv_buf[8]='S';}
    else conv_buf[8]='N';
    return conv_buf+1;
    // conv_buf +1 because we want to omit the leading zero
    }
  else {
    if (is_negative) {conv_buf[8]='W';}
    else conv_buf[8]='E';
    return conv_buf;
    }
}

/*
 * Config Struct ina uygun sayida veriyi sirayla EEPROM'dan okuyup 
 * bu struct a uygun degiskenin icine atiyoruz
 */
void eepromOku() {
  for (unsigned int i=0; i<sizeof(Ayarlar); i++) {
    *((char*)&Ayarlar + i) = EEPROM.read(i);
  }
  /*Cagri isareti dogru ise hepsinin dogru oldugunu kabul ediyoruz*/
  unsigned int iCheckSum = 0;
  for (int i=0; i<7; i++) {
    iCheckSum += Ayarlar.APRS_CagriIsareti[i];
  }
  if (iCheckSum != Ayarlar.CheckSum)  VarsayilanAyarlar();
  DEBUG_PORT.println(F("EEPROM'dan okuma tamamlandi"));
}

/*
 * CONFIG tipinde bir Struct i EEPROM a yaziyoruz
 */
void eepromaYaz() {
  for (unsigned int i=0; i<sizeof(Ayarlar); i++) {
    EEPROM.write(i, *((char*)&Ayarlar + i));
  }
  DEBUG_PORT.println("Eeproma yazildi");
}

/*
 * Print configuration details to serial port
 */
void konfigurasyonYazdir()
{
  /*
  DEBUG_PORT.println("Mevcut Konfigurasyon Bilgileri");
  DEBUG_PORT.println("------------------------------");
  */
  DEBUG_PORT.print(F("Cagri Isareti : "));  
  DEBUG_PORT.println(Ayarlar.APRS_CagriIsareti);
  DEBUG_PORT.print(F("         SSID : -")); 
  DEBUG_PORT.println(Ayarlar.APRS_CagriIsaretiSSID);
  DEBUG_PORT.print(F("        Hedef : "));  
  DEBUG_PORT.println(Ayarlar.APRS_Destination);
  DEBUG_PORT.print(F("   Hedef SSID : -")); 
  DEBUG_PORT.println(Ayarlar.APRS_DestinationSSID);  
  DEBUG_PORT.print(F("        Path1 : "));  
  DEBUG_PORT.println(Ayarlar.APRS_Path1);
  DEBUG_PORT.print(F("   Path1 SSID : -")); 
  DEBUG_PORT.println(Ayarlar.APRS_Path1SSID);
  DEBUG_PORT.print(F("        Path2 : "));  
  DEBUG_PORT.println(Ayarlar.APRS_Path2);
  DEBUG_PORT.print(F("   Path2 SSID : -")); 
  DEBUG_PORT.println(Ayarlar.APRS_Path2SSID);
  DEBUG_PORT.print(F("       Sembol : "));  
  DEBUG_PORT.println(Ayarlar.APRS_Sembolu);
  DEBUG_PORT.print(F("  Sembol Tipi : "));  
  DEBUG_PORT.println(Ayarlar.APRS_SembolTabi);
  DEBUG_PORT.print(F("  Beacon Tipi : "));  
  DEBUG_PORT.println(Ayarlar.APRS_BeaconTipi); //0=sure beklemeli, 1=Smart Beacon
  DEBUG_PORT.print(F("Beacon Suresi : "));  
  DEBUG_PORT.println(Ayarlar.APRS_BeaconSuresi);
  DEBUG_PORT.print(F("     GPS Hizi : "));  
  DEBUG_PORT.println(Ayarlar.APRS_GPSSeriHizi);    
  DEBUG_PORT.print(F("        Mesaj : "));  
  DEBUG_PORT.println(Ayarlar.APRS_Mesaj);
  DEBUG_PORT.print(F("Kontrol (CRC) : "));  
  DEBUG_PORT.println(Ayarlar.CheckSum);   
}

void VarsayilanAyarlar() {
  strcpy(Ayarlar.APRS_CagriIsareti, "TA7W"); 
  Ayarlar.APRS_CagriIsaretiSSID = '9';
  strcpy(Ayarlar.APRS_Destination, "APRS  ");
  Ayarlar.APRS_DestinationSSID = '0';
  strcpy(Ayarlar.APRS_Path1, "WIDE1 ");
  Ayarlar.APRS_Path1SSID = '1';
  strcpy(Ayarlar.APRS_Path2, "WIDE2 ");
  Ayarlar.APRS_Path2SSID = '1';
  Ayarlar.APRS_Sembolu = '>';
  Ayarlar.APRS_SembolTabi = '/';
  Ayarlar.APRS_BeaconTipi = 3;
  Ayarlar.APRS_BeaconSuresi = 255;
  strcpy(Ayarlar.APRS_Mesaj, "TAMSAT hymTR APRS Tracker");
  Ayarlar.APRS_GPSSeriHizi = 9600;
  Ayarlar.CheckSum = 291;  //TAMSAT icin Checksum degeri
  eepromaYaz();
}

void KomutSatiriCalistir() {
  byte komut;

  DEBUG_PORT.println(F("hymTR Konfigurasyon Arayuzu"));
  DEBUG_PORT.print('#');
  delay(50);

  while (komut != 'Q') {
    //digitalWrite(13, !digitalRead(13)); //Gonderme yaptigimizda tracker i konsol modundan cikariyoruz
    delay(50);
    if (DEBUG_PORT.available()) {
      komut = DEBUG_PORT.read();

      if (komut == 'R') {
        eepromOku();    
        seriyeGonder();
      }

      if (komut == 'P') {
        konfigurasyonYazdir();    
      }

      if (komut == 'W') {
        DEBUG_PORT.println(F("Konfigurasyon kaydediliyor..."));
        delay(500);
        DEBUG_PORT.println(VERSIYON);
       konfigurasyonYazdir();
        if (seridenAl()) {
              eepromaYaz();
            } else {
          //DEBUG_PORT.println(F("hata  olustu..."));
        }
      }
      
      if (komut == 'D') {
        DEBUG_PORT.println(F("Varsayilan konfigurasyona donuluyor"));
        VarsayilanAyarlar();        
      }
      DEBUG_PORT.println('#');
    } //DEBUG_PORT.available
 } //komut != Q
 DEBUG_PORT.println(F("Konfigurasyon midundan cikiliyor"));
} //komutSatiriCalistir


/*
 * seri olarak okunan byte lari 0x09 veya 0x04 e kadar olanini geri dondurmek icin
 */
void parametreOku(char *szParam, int iMaxLen) {
  byte c;
  int iSize;
  unsigned long iMilliTimeout = millis() + 2000; 

  for (iSize=0; iSize<iMaxLen; iSize++) szParam[iSize] = 0x00; 
  iSize = 0;   

  while (millis() < iMilliTimeout) {

    if (DEBUG_PORT.available()) {
      c = DEBUG_PORT.read();

      if (c == 0x09 || c == 0x04) {
        DEBUG_PORT.println();
        return;
      }
      if (iSize < iMaxLen) {
        szParam[iSize] = c;
        iSize++;
      }
    }
  }

}

bool seridenAl() {
  char szParam[45]; //en uzun mesaajdan daha uzun 45>41
  unsigned long iMilliTimeout = millis() + 10000;    
  while (millis() < iMilliTimeout) {
  while (!DEBUG_PORT.available()) { } //veri gelmesini bekle
    if (DEBUG_PORT.read() == 0x01) {
      parametreOku(szParam, sizeof(szParam));
      if (strcmp(szParam, VERSIYON) != 0) {
        DEBUG_PORT.println(szParam);
          DEBUG_PORT.println(F("E99 Versiyonlar uyumsuz..."));
        return false;
      }
    
      parametreOku(szParam, sizeof(Ayarlar.APRS_CagriIsareti));    //CagriIsareti
      strcpy(Ayarlar.APRS_CagriIsareti, szParam);
      parametreOku(szParam, 1);    //CagriIsareti SSID
      Ayarlar.APRS_CagriIsaretiSSID = szParam[0];

/*
      parametreOku(szParam, sizeof(Ayarlar.APRS_Destination));    //Destination
      strcpy(Ayarlar.APRS_Destination, szParam);
      parametreOku(szParam, 1);    //SSID
      Ayarlar.APRS_DestinationSSID = szParam[0];

      parametreOku(szParam, sizeof(Ayarlar.APRS_Path1));    //Path1
      strcpy(Ayarlar.APRS_Path1, szParam);
      parametreOku(szParam, 1);    //SSID
      Ayarlar.APRS_Path1SSID = szParam[0];

      parametreOku(szParam, sizeof(Ayarlar.APRS_Path2));    //Path2
      strcpy(Ayarlar.APRS_Path2, szParam);
      parametreOku(szParam, 1);    //SSID
      Ayarlar.APRS_Path2SSID = szParam[0];
*/
      //Symbol/Tab
      parametreOku(szParam, 1);
      Ayarlar.APRS_Sembolu = szParam[0];
      parametreOku(szParam, 1);
      Ayarlar.APRS_SembolTabi = szParam[0];

      //BeaconTipi
      parametreOku(szParam, sizeof(szParam));
      Ayarlar.APRS_BeaconTipi = atoi(szParam);

      //Beacon Suresi
      parametreOku(szParam, sizeof(szParam));
      Ayarlar.APRS_BeaconSuresi = atoi(szParam);

      //Status Message
      parametreOku(szParam, sizeof(szParam));
      strcpy(Ayarlar.APRS_Mesaj, szParam);


      parametreOku(szParam, sizeof(szParam));
      Ayarlar.APRS_GPSSeriHizi = atoi(szParam);    


      unsigned int iCheckSum = 0;
      for (int i=0; i<7; i++) {
        iCheckSum += Ayarlar.APRS_CagriIsareti[i];
      }
      Ayarlar.CheckSum = iCheckSum;
      return true;    //OKuma tamamlandi
    } // read 0x01
  } //millis
  return false;

}

/*
 * Elimizdeki CONFIG degiskenini byte byte PC'ye gonderiyoruz
 */
void seriyeGonder() {
        DEBUG_PORT.print(F("{"));    //JSON Baslangici
        DEBUG_PORT.print(F("'V':'"));
        DEBUG_PORT.print(VERSIYON);
        DEBUG_PORT.print(F("','CagriIsareti':'"));
        DEBUG_PORT.print(Ayarlar.APRS_CagriIsareti);
        DEBUG_PORT.print(F("','CagriIsaretiSSID':"));
        DEBUG_PORT.print(Ayarlar.APRS_CagriIsaretiSSID);
        DEBUG_PORT.print(F(",'Destination':'"));
        DEBUG_PORT.print(Ayarlar.APRS_Destination);
        DEBUG_PORT.print(F("','DestinationSSID':"));
        DEBUG_PORT.print(Ayarlar.APRS_DestinationSSID);
        DEBUG_PORT.print(F(",'Path1':'"));
        DEBUG_PORT.print(Ayarlar.APRS_Path1);
        DEBUG_PORT.print(F("','Path1SSID':"));
        DEBUG_PORT.print(Ayarlar.APRS_Path1SSID);
        DEBUG_PORT.print(F(",'Path2':'"));
        DEBUG_PORT.print(Ayarlar.APRS_Path2);
        DEBUG_PORT.print(F("','Path2SSID':"));
        DEBUG_PORT.print(Ayarlar.APRS_Path2SSID);
        //Symbol
        DEBUG_PORT.print(F(",'Sembol':'"));
        DEBUG_PORT.print(Ayarlar.APRS_Sembolu);
        DEBUG_PORT.print(F("','SembolTabi':'"));
        DEBUG_PORT.print(Ayarlar.APRS_SembolTabi);
        //Beacon Type
        DEBUG_PORT.print(F("','BeaconTipi':"));
        DEBUG_PORT.print(Ayarlar.APRS_BeaconTipi, DEC);
        DEBUG_PORT.print(F(",'BeaconSuresi':"));
        //Beacon - Simple Delay
        DEBUG_PORT.print(Ayarlar.APRS_BeaconSuresi, DEC);
        //Status Message
        DEBUG_PORT.print(F(",'Mesaj':'"));
        DEBUG_PORT.print(Ayarlar.APRS_Mesaj);
        //GPS DEBUG_PORT Data
        DEBUG_PORT.print(F("','GPSHizi':"));
        DEBUG_PORT.print(Ayarlar.APRS_GPSSeriHizi, DEC);      
        DEBUG_PORT.print(F("}")); //JSON Sonu 
}

