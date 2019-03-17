#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <SPI.h>

#include <driver/adc.h>

#define SSID                  "SSID Name"  // ชื่อ Internet WiFi Router
#define PASSWORD              "password"  // รหัสลับของ WiFi Router

#define FIREBASE_HOST         "https://hydrocube-b8ad7.firebaseio.com/"  // กำหนด host ของฐานข้อมูลบน Firebase
#define FIREBASE_AUTH         "CD5tsVMqjFW5sOdNLeXKUoHPHt5dChQDzC2ugxDJ"  

#define w_pin 33    
#define relay_w 19 //ch3
#define relay_t 23 //ch2
#define ldr_pin 32 
#define led_ldr 17 
#define Neo_PIN 15
#define CS 26
#define SO 25
#define SCK 27

//--------------------------------------------------------
/* ฟังกชั่น สำหรับ รับและส่งข้อมูลไปยัง Firebase ใช้สำหรับ ESP32 */
String  TD32_Get_Firebase(String path );               // รับค่า path จาก Firebase
int     TD32_Set_Firebase(String path, String value, bool push=false ); // ส่งค่าขึ้น Firebase  (ทับข้อมูลเดิมใน path เดิม)
int     TD32_Push_Firebase(String path, String value); // ส่งค่าขึ้น Firebase แบบ Pushing data  (เพิ่มเข้าไปใหม่เรื่อยๆใน path เดิม)
//--------------------------------------------------------

int btn_led = 0;
int LEDSTATE = 0; //check state do only when tricker

BLYNK_CONNECTED(){  
  Blynk.syncVirtual(V4);
}

BLYNK_WRITE(V4){  
  btn_led = param.asInt();
}

Adafruit_NeoPixel strip = Adafruit_NeoPixel(24, Neo_PIN, NEO_GRB + NEO_KHZ800);

char auth[] = "2fce52560daf4938ab95c0f833d7d295";

char ssid[] = "SSID Name";
char pass[] = "Password";

WidgetLCD lcd_ldr(V1);
WidgetLCD lcd_wl(V2);

void setup() {
 
  pinMode(w_pin, INPUT); //water sensor
  pinMode(ldr_pin, INPUT); //ldr sensor
  pinMode(relay_w, OUTPUT); //
  pinMode(relay_t, OUTPUT);
  
   
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Blynk.begin(auth, ssid, pass);
  Serial.begin(9600);

}

int cnt;
uint32_t timer;
void loop() {
  
  int value_ldr =0;
  value_ldr = analogRead(ldr_pin); //Read LDR sensor
  Serial.println(value_ldr);
  
  float temp = readThermocouple(CS,SO,SCK);
  Serial.println(temp);
  
  int value_w;
  value_w = analogRead(w_pin); //Read water sensor
  delay(500);

  //water sensor
  Serial.print("water lavel =");
  Serial.println(value_w);

  if (value_w<=500){ //if value water level <=500 water pump ON
  digitalWrite(relay_w, HIGH);  
    
  }
  else{
    digitalWrite(relay_w, LOW); 
    }

   if (temp>=33){ //if tem >=33 spray pump ON
  digitalWrite(relay_t, HIGH);  
    
  }
  else{
    digitalWrite(relay_t, LOW); 
    }
    

  if(btn_led == 1 ){
    if(LEDSTATE == 0){
      R3B1NeoLED_on();
      LEDSTATE = 1;
    }
  }else if(btn_led == 0){
    if(LEDSTATE == 1){
      R3B1NeoLED_off();
      LEDSTATE = 0;
    } 
  }


    //LDR Sensor Blynk
  lcd_ldr.print(0,0, "LDR=");
  lcd_ldr.print(5,0, value_ldr);

  //Water level Sensor Blynk
  lcd_wl.print(0,0, "WaterLevel= ");
  lcd_wl.print(0,1, value_w);

  Blynk.virtualWrite(V3,temp);

   delay(50);
  if( millis() -timer > 60000) {       // ทำการอ่านค่าและส่งทุกๆ 20 วินาที
    timer = millis();
  
    //float t = (float)random(2000,4000)/100;  // อ่านค่า อุณหภูมิ (สมมติใช้ค่า random แทน)
    //float h = (float)random(1000,9000)/100;  // อ่านค่า ความชื้น (สมมติใช้ค่า random แทน)

    TD32_Push_Firebase("Sensor/Temperature", String(temp));
    TD32_Push_Firebase("Sensor/Waterlevel", String(value_w));
    TD32_Push_Firebase("Sensor/LDR", String(value_ldr));
    //TD32_Set_Firebase("Sensor/temp", String(temp));// ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Temp
    //TD32_Push_Firebase("Sensor/Humid", String(h)); // ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Humid แบบ Pushing data

  }

 
  Blynk.run();
  
}


void R3B1NeoLED_on(){
    for(int i=0; i<24; i+=4){ //เลขที่หาร24ลงตัว
      strip.setPixelColor(i, strip.Color(255,0,0));
      strip.setPixelColor(i+1, strip.Color(255,0,0));
      strip.setPixelColor(i+2, strip.Color(255,0,0));
      strip.setPixelColor(i+3, strip.Color(0,0,255));
      strip.show();
      delay(10);
    }
}

void R3B1NeoLED_off() { //ปิดไฟทั้งหมด
  for(int i=0; i<24; i+=4) {
    strip.setPixelColor(i, strip.Color(0,0,0));
    strip.setPixelColor(i+1, strip.Color(0,0,0));
    strip.setPixelColor(i+2, strip.Color(0,0,0));
    strip.setPixelColor(i+3, strip.Color(0,0,0));
    strip.show();
    delay(10);  
  }
}

double readThermocouple(int MAX6675_CS, int MAX6675_SO, int MAX6675_SCK) {
  uint16_t v;
  pinMode(MAX6675_CS, OUTPUT);
  pinMode(MAX6675_SO, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);
  digitalWrite(MAX6675_CS, LOW);
  delay(1);  
  v = shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
  v <<= 8;
  v |= shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST); 
  digitalWrite(MAX6675_CS, HIGH);
  if (v & 0x4) 
  {    
    // Bit 2 indicates if the thermocouple is disconnected
    return NAN;     
  }
  // The lower three bits (0,1,2) are discarded status bits
  v >>= 3;
  // The remaining bits are the number of 0.25 degree (C) counts
  return v*0.25;
}

/**********************************************************
 * ฟังกชั่น TD32_Set_Firebase
 * สำหรับ ESP32 ใช้กำหนด ค่า value ให้ path ของ Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคืนค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
// Root CA ของ https://www.firebaseio.com  
// ใช้ได้ตั้งแต่ 01/08/2018 17:21:49 GMT ถึง หมดอายุสิ้นสุด 27/03/2019 00:00:00 GMT
const char* FIREBASE_ROOT_CA= \
        "-----BEGIN CERTIFICATE-----\n" \
        "Insert Code Herer\n" \
        "-----END CERTIFICATE-----\n";

int TD32_Set_Firebase(String path, String value, bool push ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  int httpCode = 404; // Not Found

  String firebase_method = (push)? "POST " : "PUT ";
  ssl_client.setCACert(FIREBASE_ROOT_CA);
  if( ssl_client.connect( host.c_str(), 443)){
    String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
    String request = "";
          request +=  firebase_method + uri +" HTTP/1.1\r\n";
          request += "Host: " + host + "\r\n";
          request += "User-Agent: TD_ESP32\r\n";
          request += "Connection: close\r\n";
          request += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n";
          request += "Content-Length: "+String( value.length())+"\r\n\r\n";
          request += value;

    ssl_client.print(request);
    while( ssl_client.connected() && !ssl_client.available()) delay(10);
    if( ssl_client.connected() && ssl_client.available() ) {
      resp      = ssl_client.readStringUntil('\n');
      httpCode  = resp.substring(resp.indexOf(" ")+1, resp.indexOf(" ", resp.indexOf(" ")+1)).toInt();
    }
    ssl_client.stop();    
  }
  else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return httpCode;
}

/**********************************************************
 * ฟังกชั่น TD32_Push_Firebase
 * สำหรับ ESP32 ใช้กำหนด ค่า value ให้ path ของ Firebase
 * แบบ Pushing data (เติมเข้าไปที่ path เรื่อยๆ ไม่ทับของเดิม)
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคืนค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
int TD32_Push_Firebase(String path, String value){
    return TD32_Set_Firebase(path,value,true);
}
/**********************************************************
 * ฟังกชั่น TD32_Get_Firebase 
 * ใช้สำหรับ EPS32 รับ ค่า value ของ path ที่อยู่บน Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * path เป็น ข้อมูลประเภท String
 * คืนค่าของฟังกชั่น คือ value ของ path ที่กำหนด ในข้อมูลประเภท String
 * 
 **********************************************************/
String TD32_Get_Firebase(String path ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  String value = "";
  ssl_client.setCACert(FIREBASE_ROOT_CA);
  if( ssl_client.connect( host.c_str(), 443)){
    String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
    String request = "";
          request += "GET "+ uri +" HTTP/1.1\r\n";
          request += "Host: " + host + "\r\n";
          request += "User-Agent: TD_ESP32\r\n";
          request += "Connection: close\r\n";
          request += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n\r\n";

    ssl_client.print( request);
    while( ssl_client.connected() && !ssl_client.available()) delay(10);
    if( ssl_client.connected() && ssl_client.available() ) {
      while( ssl_client.available()) resp += (char)ssl_client.read();
      value = resp.substring( resp.lastIndexOf('\n')+1, resp.length()-1);
    }
    ssl_client.stop();    
  } else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return value;
}


