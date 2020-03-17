# hymTR
APRS Tracker
Arduino Uno ile calisan bir APRS tracker projesidir. 
Temel oalrak asagidaki ozelliklere sahiptir :
- Herhangibir arduino ile calisir
- Telsize hoparlor arayuzunden baglanir
- 4 bitlik bir Direnc iskelesi (resistor ladder) DAC ile calisir (16 seviye)
- Asagidaki verileri GPS den okur
  - Zaman
  - Pozisyon
  - Hiz/yonlenme vb
  - Uydu sayisi
  - Fix durumu
- Smart beacon ozelligi ile hiz ile orantili olarak gonderme sikligini degistirir
- Verileri EEPROM'da saklar
- PC Yazilimi ile tum ayarlari degistirilebilir

Eklenmesi planlanan ozellikler :
- APRS dinleme ozelligi
- Dorji radyolari yonetebilme ozelligi
- Display ekelenebilir


PC Yonetim arayuzu programi "hymTR Ayar Programi" ayri bir repository icindedir.

Donanim detaylari icin <A HREF="https://github.com/barisdinc/hymTR/blob/master/Hardware/README.md">Donanim Sayfasi'na bakabilirsiniz</A>


Ocak 2020
