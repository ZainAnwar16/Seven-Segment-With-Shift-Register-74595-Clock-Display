#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "RTClib.h"
#include "HTML_WithPWM.h"


RTC_DS3231 rtc;

IPAddress local_IP(192,168,1,99);
IPAddress gateway(192,168,1,99);
IPAddress subnet(255,255,255,0);

byte segment[]={ 0x80,0xF2,0x48,0x60,0x32,0x24,0x04,0xF0,0x00,0x20}; //CA
byte pinSR[]={15,2,0}; //C,D,L
byte calculateDot [2];
byte forMinute;
int PWM,lastPWM,lastvalPWM;
unsigned long forMillis[] = {0,0,0,0,0};

ESP8266WebServer server(80); 

void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 s += "<h4 style = \"color: gold; font-size: 75%;\"><p>Value PWM Now: ";
 s += String(valuePWM()) + " or " + String(valueVoltage(valuePWM())) + " volt";
 s += "</p></h4>";
 //server.send_P(200, "text/html", MAIN_page,sizeof(MAIN_page)); //Send web page
 server.send_P(200, "text/html", s.c_str());
}

void handleForm() {
 String hasilJam = server.arg("Jam"); //name pada inputan 
 String hasilMenit = server.arg("Menit");
 String hasilDetik = server.arg("Detik"); //name pada inputan 
 String hasilPWM = server.arg("vPWM");
 String hasilTanggal = server.arg("Tanggal");
 String hasilBulan = server.arg("Bulan"); //name pada inputan 
 String hasilTahun = server.arg("Tahun");
 
// Serial.println("HH:MM:SS");
// Serial.print(hasilJam);
// Serial.print(":");
// Serial.print(hasilMenit);
// Serial.print(":");
// Serial.println(hasilDetik);
//
// Serial.print("PWM :");
// Serial.println(hasilPWM);
// 
// Serial.println("DD:MM:YYYY");
// Serial.print(hasilTanggal);
// Serial.print("/");
// Serial.print(hasilBulan);
// Serial.print("/");
// Serial.println(hasilTahun);

 lastPWM = map(100-hasilPWM.toInt(),0,100,0,1023);
 if (lastPWM!=PWM){
   PWM = lastPWM;
   EEPROM.write(1,PWM/1000);
   EEPROM.write(2,((PWM/100)%10));
   EEPROM.write(3,((PWM/10)%10));
   EEPROM.write(4,PWM%10);
   EEPROM.commit(); 
 } 
  
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
 if( hasilJam.length() > 0 && hasilMenit.length() > 0 && hasilDetik.length() > 0 && hasilTanggal.length() > 0 && hasilBulan.length() > 0 && hasilTahun.length() > 0){
  rtc.adjust(DateTime(hasilTahun.toInt(),hasilBulan.toInt(),hasilTanggal.toInt(),hasilJam.toInt(),hasilMenit.toInt(),(hasilDetik.toInt())));
  //rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
 }

 //server.send(200, "text/html", "<a href='/'> BackForSettingAgain</a>"); //Send web page
 server.sendHeader("Location", "/");
 server.send(302,"text/plain","update-- Press Back Button");
}

void refreshTime(DateTime now = rtc.now()){
      
   if((millis()-forMillis[0])>=1000){
    calculateDot[0] = (now.second()%10)%2;
      if (calculateDot[0] == 0){
        calculateDot[1] = 0;
      }
      else {
        calculateDot[1] = 1;
      }
    forMinute=segment[now.minute()%10]|(calculateDot[1] << 0);
    forMillis[0]=millis();
  }

  if(now.hour()==0 && now.minute()==0 && now.second()==1){
    rtc.adjust(DateTime(now.year(),now.month(),now.day(),now.hour(),now.minute(),(now.second()+1)));
  }
  
   
   digitalWrite(pinSR[2], LOW);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.second()%10]);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.second()/10]);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, forMinute);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.minute()/10]);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.hour()%10]);
   shiftOut(pinSR[1], pinSR[0], MSBFIRST, segment[now.hour()/10]);
   digitalWrite(pinSR[2], HIGH);  
}

int valuePWM(){
  String valueString = String(EEPROM.read(1))+String(EEPROM.read(2))+String(EEPROM.read(3))+String(EEPROM.read(4));
  return valueString.toInt();
}
double valueVoltage(int input){
  return (input*3.3)/1023;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  for(int p=0; p<3; p++){
    pinMode(pinSR[p],1);
  }
  rtc.begin();
  WiFi.mode(WIFI_AP);
  //WiFi.softAP(ssid, password, channel(1), hidden(T(1)/F(0)), max_connection)
  WiFi.softAP("ESP826612F","ESP12FZAINANWAR");     //Connect to your WiFi router
  WiFi.softAPConfig(local_IP, gateway, subnet);
  /*Serial.println("");
 
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println("WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());  //IP address assigned to your ESP*/
 
  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on("/zainanwar.webserver", handleForm); //form action is handled here
 //http://192.168.1.99/action_page?Jam=15&Tanggal=7&Menit=41&Bulan=8&Detik=7&Tahun=2022&vPWM=15&nowPWM=valuePWM
  server.begin();                  //Start server
  //Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();          //Handle client requests
  if(lastvalPWM!=valuePWM()){
    lastvalPWM=valuePWM();
    analogWrite(16,valuePWM());
  }
  if((millis()-forMillis[1])>=1000){
    refreshTime();
    forMillis[1]=millis();
  }
}
