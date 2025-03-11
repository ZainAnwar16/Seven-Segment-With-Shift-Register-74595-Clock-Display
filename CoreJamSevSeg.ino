#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "RTClib.h"
#include <PrayerTimes.h>
#include <LittleFSRW.h>
#include <AlarmBuzzer.h>


RTC_DS3231 rtc;
LittleFSRW AccesFile;
AlarmBuzzer BuzzerJWS(2); //fill GPIO pin Buzzer 

IPAddress local_IP(192, 168, 1, 97); //coba
IPAddress gateway(192, 168, 1, 97); //coba
IPAddress subnet(255, 255, 255, 0);

byte segment[] = { 0x80, 0xF2, 0x48, 0x60, 0x32, 0x24, 0x04, 0xF0, 0x00, 0x20, 0 };  //CA
byte pinSR[] = { 14, 13, 12, 2 };                                                    //C,D,L,15,2,0
byte forMinute, nowHourAlrm, nowMinuteAlrm, chckConection;
byte tAlarms[2][2];//array -1
bool flagbuz, statenow;
int PWM, lastPWM, lastvalPWM, valLastSlider, intButtonTurnAll, repeatBuz, hijriYear, hijriMonth, hijriDay, pasarDay;
String button, PrayTimetoWeb[7];
unsigned long forMillis[] = { 0, 0, 0, 0, 0, 0 }; //4,5 empty
double jamSholat[sizeof(TimeName)];
char* namaBulanHijriah;
char* namaHariJawa;
const char* nameDandM[19]={
    //nama Hari from 0
		"Minggu", "Senin", "Selasa", "Rabu", 
    "Kamis", "Jumat", "Sabtu",
    //namaBulan from 7
    "Desember", "Januari",    "Februari", "Maret", 
    "April",    "Mei",        "Juni",     "Juli",
    "Agustus",  "September",  "Oktober",  "November"
	};

ESP8266WebServer server(80);





//=============================================================================================================================================================
//Function for Handle Web
void handleRoot() {
  server.send(200,"text/html", AccesFile.ReadFiletoString("/WebServer2025.html"));
}

void handleForm() {
  String hasilJam = server.arg("Jam");  //name pada inputan
  String hasilMenit = server.arg("Menit");
  String hasilDetik = server.arg("Detik");  //name pada inputan
  String hasilTanggal = server.arg("Tanggal");
  String hasilBulan = server.arg("Bulan");  //name pada inputan
  String hasilTahun = server.arg("Tahun");

  // Serial.println("HH:MM:SS");
  // Serial.print(hasilJam);
  // Serial.print(":");
  // Serial.print(hasilMenit);
  // Serial.print(":");
  // Serial.println(hasilDetik);
  
  // Serial.println("DD:MM:YYYY");
  // Serial.print(hasilTanggal);
  // Serial.print("/");
  // Serial.print(hasilBulan);
  // Serial.print("/");
  // Serial.println(hasilTahun);

   if( hasilJam == "" && hasilMenit == "" && hasilDetik == "" && hasilTanggal == "" && hasilBulan == "" && hasilTahun == ""){
    DateTime now = rtc.now();
    now.hour();
    now.minute();
    now.second();
    now.dayOfTheWeek();
    now.day();
    now.month();
    now.year();
   }

  if (hasilJam.length() > 0 && hasilMenit.length() > 0 && hasilDetik.length() > 0 && hasilTanggal.length() > 0 && hasilBulan.length() > 0 && hasilTahun.length() > 0) {
    rtc.adjust(DateTime(hasilTahun.toInt(), hasilBulan.toInt(), hasilTanggal.toInt(), hasilJam.toInt(), hasilMenit.toInt(), (hasilDetik.toInt() + 3)));
    //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //server.send(200, "text/html", "<a href='/'> BackForSettingAgain</a>"); //Send web page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void infoVoltage() {
  String voltage = String(valueVoltage(valuePWM()));
  // Serial.println(voltage);
  server.send(200, "text/plane", voltage);
}

void infoPWM() {
  String PWMnow = String(valuePWM());
  // Serial.println(PWMnow);
  server.send(200, "text/plane", PWMnow);
}

void infoLastSlider() {
  String lastSlider = String(valueSlider());
  // Serial.println(lastSlider);
  server.send(200, "text/plane", lastSlider);
}

void infoJWSWEB() {
  DateTime dateJWS = rtc.now();
  String JWSWEB = "► SURABAYA, " + String(nameDandM[dateJWS.dayOfTheWeek()]) + " " + String(namaHariJawa) + " " + String(dateJWS.day()/10) + String(dateJWS.day()%10) +
                  " " + String(nameDandM[dateJWS.month()+7]) + " " + String(dateJWS.year()) +" / " +
                  String(hijriDay) + " " + String(namaBulanHijriah) + " " + String(hijriYear) + " H - " +
                  PrayTimetoWeb[0] + " - " + PrayTimetoWeb[1] + " - " + PrayTimetoWeb[2] + " - " + PrayTimetoWeb[3] + " - " +
                  PrayTimetoWeb[4] + " - " + PrayTimetoWeb[5] + " - " + PrayTimetoWeb[6] + " - " + PrayTimetoWeb[7] + " ◄ " + "     Additional " + String(dateJWS.hour()) + " : " +String(dateJWS.minute());
  server.send(200, "text/plane", JWSWEB); //◄►
}

void infoIDChip() {
  String ChipID = String(ESP.getChipId(),HEX);
  ChipID.toUpperCase();
  // Serial.println(ESP.getChipId(),HEX);
  server.send(200, "text/plane", "0x"+ChipID);
}

void TurnOnAll() {
  button = server.arg("valstate7Seg");
  // Serial.println(button);
  server.send(200, "text/plane", "0");
}

void RestartESP(){
  String Trg  = server.arg("valRestart");
  // Serial.println(Trg);
  server.send(200, "text/plane", "0");
  if(Trg=="1"){
    ESP.restart();
  }
}

void infoSliderNow() {
  String sliderNow = server.arg("valueSlider");
  // Serial.println("test " + sliderNow);
  server.send(200, "text/plane", "0");
  //=============================================================================
  //Handle For Save Value from web to file /InfoPWMandSlider.txt
  lastPWM = map(sliderNow.toInt(), 100, 0, 0, 255); //nilai dibalik karena menggunakan display CA aktif LOW/0
  if (lastPWM != PWM || valLastSlider != sliderNow.toInt()) {
    PWM = lastPWM;
    valLastSlider = sliderNow.toInt();
    int dataInt[] = {PWM, valLastSlider};
    AccesFile.FileWriteInt("/InfoPWMandSlider.txt", dataInt, 2, ";");
  }
}





//=============================================================================================================================================================
//Handle For Load Value from file /InfoPWMandSlider.txt

int valuePWM() {
  int valPWM;
  AccesFile.FileReadInt("/InfoPWMandSlider.txt", 1, ";", valPWM);
  return valPWM;
}
double valueVoltage(int input) {
  return (input * 3.3) / 255;
}

int valueSlider() {
  int valSlider;  
  AccesFile.FileReadInt("/InfoPWMandSlider.txt", 2, ";", valSlider);
  return valSlider;
}




//=============================================================================================================================================================
//Function for Handle 74595 ShiftRegister 7Seg
void refreshTime(int x, DateTime now = rtc.now()) {

  if ((millis() - forMillis[0]) >= 1000) {
    int calculateDot = (((now.second() % 10) % 2) == 0) ? 0 : 1;
    forMinute = segment[now.minute() % 10] | (calculateDot << 0);
    //Serial.println(calculateDot);
    forMillis[0] = millis();
  }

  if (now.hour() == 0 && now.minute() == 0 && now.second() == 1) {
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), (now.second() + 1)));
  }

  switch (x) {
    case 0:
      digitalWrite(pinSR[2], LOW);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.second() % 10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.second() / 10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, forMinute);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.minute() / 10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.hour() % 10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.hour() / 10]);
      digitalWrite(pinSR[2], HIGH);
      break;
    case 1:
      digitalWrite(pinSR[2], LOW);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[10]);
      digitalWrite(pinSR[2], HIGH);
      break;
    default:
      break;
  }
}




//=============================================================================================================================================================
//=============================================================================================================================================================
//=============================================================================================================================================================
//Process
void setup() {
  //=============================================================================================================================================================
  //Setup For All
  Serial.begin(115200);
  for (int p = 0; p < 4; p++) {
    pinMode(pinSR[p], 1);
  }
  rtc.begin();

  //=============================================================================================================================================================
  //Begin For WiFi
  WiFi.mode(WIFI_AP);
  //WiFi.softAP(ssid, password, channel(1), hidden(T(1)/F(0)), max_connection)
  WiFi.softAP("ESP826612FCoba", "12345678");  //Connect to your WiFi router coba
  WiFi.softAPConfig(local_IP, gateway, subnet);
  /*Serial.println("");
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println("WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());  //IP address assigned to your ESP*/

  //=============================================================================================================================================================
  //Server on (EndPoint)
  server.on("/", handleRoot);                     //Which routine to handle at root location
  server.on("/zainanwar.webserver", handleForm);  //form action is handled here
  server.on("/brightnessRead", infoVoltage);
  server.on("/vPWMRead", infoPWM);
  server.on("/lastSliderRead", infoLastSlider);
  server.on("/sliderValue", infoSliderNow);
  server.on("/JWSWeb", infoJWSWEB);
  server.on("/IDEsp", infoIDChip);
  server.on("/set7Seg", TurnOnAll);
  server.on("/RestartESP", RestartESP);
  //http://192.168.1.99/action_page?Jam=15&Tanggal=7&Menit=41&Bulan=8&Detik=7&Tahun=2022&vPWM=15&nowPWM=valuePWM
  server.begin();  //Start server
  //Serial.println("HTTP server started");

  //=============================================================================================================================================================
  //PrayerTimes Setup
  set_calc_method(MWL);
  set_asr_method(Shafii);
  set_high_lats_adjust_method(AngleBased);
  // 1 derajat kurleb 4 menit
  set_fajr_angle(20);
  set_isha_angle(18);
  set_imsak_angle(22.38); // imsk angel - fjr angel =  22.3 -20 = 2.3 degree * 4mnt/degree = 9.2 menit dibulatkan menjadi 10mnt
  set_dhuha_angle(-5.8); // 6.7 degree * 4mnt/degree = 27.3 mnt dibulatkan 27mnt

  //=============================================================================================================================================================
  //Setup Try/dummy
  tAlarms[0][0] = 6;
  tAlarms[0][1] = 50;
  tAlarms[1][0] = 12;
  tAlarms[1][1] = 0;

  // Konfigurasi buzzer 1 (pin 9): 5 waktu, aktif low, dengan long pause, durasi 1 menit
  BuzzerJWS.setPattern(3, 250, 250, 1500, false, 90000); //cntBeep, timeon, timeoff, longP, configActive, duration, jika ingin flipflop cntBeep dan longP diisi 0 
  BuzzerJWS.addAlarm(tAlarms[0][0], tAlarms[0][1], 0);   // HH,MM,SS
  BuzzerJWS.addAlarm(tAlarms[1][0], tAlarms[1][1], 0);   // Max 5 alarmTime 1 pin
  BuzzerJWS.addAlarm(16, 30, 0);

}


void loop() {
  server.handleClient();  //Handle client requests

  if (lastvalPWM != valuePWM()) {
    lastvalPWM = valuePWM();
    analogWrite(15, valuePWM());  //16
  }
  if ((millis() - forMillis[1]) >= 1000) {
    refreshTime(button.toInt());
    forMillis[1] = millis();
  }

  if ((millis() - forMillis[2]) >= 5*1000) {
    DateTime praytms = rtc.now();
    get_prayer_times(praytms.year(), praytms.month(), praytms.day(), -7.3357650, 112.7669901, 7, jamSholat); //-7.3357650, 112.7669901 -7.2575, 112.7521
    get_hijri_date(praytms.year(),praytms.month(), praytms.day(), hijriYear, hijriMonth, hijriDay, &namaBulanHijriah);
    get_pasaranjawa_date(praytms.year(), praytms.month(), praytms.day(), pasarDay, &namaHariJawa);
    for (int i=0; i<8; i++) {
      int pryHour, pryMnt;
      get_float_time_parts(jamSholat[i], pryHour, pryMnt);
      PrayTimetoWeb[i]  = String(TimeName[i]) + " " + String(pryHour/10) + String(pryHour%10) + ":" + String(pryMnt/10) + String(pryMnt%10);
    }
    forMillis[2] = millis();
  }

   if ((millis() - forMillis[3]) >= 100) {
    DateTime alarm = rtc.now();
    BuzzerJWS.checkAlarm(alarm);
    forMillis[3] = millis();
   }
}
