//message format:
//password#command*time
//  3425#start*30
// 1
// 2

#include <MsTimer2.h>
//#include <SimpleTimer.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#define maxLength 256

String inData = String(maxLength);
char data[maxLength];
char phone_number[]="+79222620280";
int x;
int onModulePin = 2;
int led = 13;
boolean logg = true;
boolean first_loop = true;
unsigned long prevMillis = 0;
unsigned long currMillis;

// GPS
//bool newdata = false;
unsigned long age, date, time, chars = 0, sat, hdopl;
float flat, flon;
int alt, cou, spd;
//char c_lat[10], c_lon[10], c_cou[6], c_alt[6], c_spd[6];

TinyGPS gps;

//SimpleTimer timer;

SoftwareSerial nss(9, 8);
SoftwareSerial debug = SoftwareSerial(11, 10);

//**************************************************************
void setup() {
  // GSM
  Serial.begin(9600);
  delay(2000);
    
  // Debug port
  debug.begin(9600);
  delay(200);
  
  // GPS
  if (logg) {
    debug.println("Setting up GPS...");
  }
  nss.begin(4800);
  if (logg) {
    debug.println("Done setting up GPS!");
  }  
  delay(200);

  pinMode(led, OUTPUT);
  pinMode(onModulePin, OUTPUT);

  HeaterStop();
  if (logg) {
    debug.println("Starting GSM module...");
  }
  switchModule();
  for (int i=0;i < 5;i++){
      delay(4000);
  }
  Serial.println("AT+CLIP=1");
  delay(1000);
  Serial.println("AT+CMGF=1");
  delay(1000);
  Serial.println("AT+CNMI=2,2,0,0,0");
  delay(1000);
   
  InitGPRS();
  
  if (logg) {
    debug.println("Started!");
  }  
}

//**************************************************************
void InitGPRS() { 
  
  if (logg) {
    debug.println("Setting up GPRS...");
  }  

  if (logg) {debug.println("AT&k3...");}
  Serial.println("AT&k3");
  delay(1000); 
  getSerialChars();

  if (logg) {debug.println("AT+KCNXCFG...");}
  Serial.print("AT+KCNXCFG=0,");
  Serial.write(34);
  Serial.print("GPRS");
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.print("internet");
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.print("0.0.0.0");
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.print("0.0.0.0");
  Serial.write(34);
  Serial.print(",");
  Serial.write(34);
  Serial.print("0.0.0.0");
  Serial.write(34);
  Serial.println();
  delay(2000);
  getSerialChars();

  if (logg) {debug.println("AT+KUDPCFG..."); } 
  Serial.println("AT+KUDPCFG=0,0");
  delay(4000);
  getSerialChars();  
  
//  if (logg) {debug.println("close opened connections...");}
//  Serial.println("AT+KUDPCLOSE=1");
//  delay(1000);
//  getSerialChars();
//
//  if (logg) {debug.println("opened connections closed!");}    
  
  if (logg) {debug.println("GPRS initiated!");}  

}

//**************************************************************
void loop() {
  getSerialChars();
  // SMS
  if (inData.indexOf('3425#') != -1) {
    int IdxCmd = inData.indexOf('#');
    int IdxTim = inData.indexOf('*');
    String command = String(10);
    String TimeStr = String(2);
    command = inData.substring(IdxCmd + 1, IdxTim);
    if (logg) {
      debug.print("Cmd: ");
      debug.println(command);
    }
    TimeStr = inData.substring(IdxTim + 1, inData.length());
    char this_char[TimeStr.length() + 1];
    TimeStr.toCharArray(this_char, sizeof(this_char));
    long Time = atoi(this_char);
    Time = Time * 1000 * 60;
    
    if (command == "start") {
      HeaterStart();
      MsTimer2::set(Time, HeaterStop);
      MsTimer2::start();
      if (logg) {
        debug.print("start on ");
        debug.println(Time);
      }
    }
    if (command == "stop") {
      HeaterStop();
    }
    if (command == "getpos") {
      GetGPSData(gps);
      sendSMSGPS();
    }
    if (command == "int") {
      SendGPSDataToServer(true);
    }
    
    deleteAllMsgs();
  }
  // Incoming ring
  if (inData.indexOf("+CLIP") != -1) {
    int IdxCmd = inData.indexOf('+CLIP');
    String incnum = inData.substring(IdxCmd + 4, IdxCmd + 16);
    if (logg) {
      debug.print("number: ");
      debug.println(incnum);
    }
    if (incnum == "+79222620280") {
      HeaterStart();
      MsTimer2::set(1200000, HeaterStop);
      MsTimer2::start();
      //timer.setTimeout(1200000, HeaterStop);
      if (logg) {debug.println("start on ring 20 min.");}
      delay(1000);
      Serial.println("ATH");              // disconnect the call
    }
  }
  //timer.run();
  delay(1000);
  currMillis = millis();
  if (first_loop) {
    prevMillis = 4000;
    first_loop = false;
  }
  
  if (currMillis - prevMillis > 60000) {
    prevMillis = currMillis;
    SendGPSDataToServer(false);
  }
}

//*****************************************************************************************
void switchModule(){
  digitalWrite(onModulePin,HIGH);
  delay(2000);
  digitalWrite(onModulePin,LOW);
}

//**************************************************************
void HeaterStart() {
  digitalWrite(led, HIGH);
}

//**************************************************************
void HeaterStop() {
  digitalWrite(led, LOW);
  MsTimer2::stop();
  if (logg) {debug.println("Heater stop.");}

//  Serial.print("AT+CMGS=\"");
//  Serial.print(phone_number);
//  Serial.println("\"");
//  while(Serial.read()!='>');
//  Serial.print("heater stopped");
//  Serial.println("");
//  delay(500);
//  Serial.write(0x1A);       //sends ++
//  Serial.write(0x0D);
//  Serial.write(0x0A);

}

//**************************************************************
void getSerialChars() {
  for (x=0;x <= 255;x++){
    data[x]='\0';                        
  }
  x=0;
  while (Serial.available() > 0) {
    data[x] = Serial.read();
    x++;
  }
  inData = data;
  if (logg && inData.length() > 0) {debug.println(inData);}
}

//**************************************************************
static bool feedgps()
{
  while (nss.available())
  {
    if (gps.encode(nss.read()))
      return true;
  }
  return false;
}

//**************************************************************
void GetGPSData(TinyGPS &gps) {
    unsigned long start = millis();
    while (millis() - start < 1000)
    {
      if (feedgps()){
      }
    }  
    sat = gps.satellites();
    hdopl = gps.hdop();
    gps.f_get_position(&flat, &flon, &age);
    alt = gps.f_altitude();
    cou = gps.f_course();
    spd = gps.f_speed_kmph();
    
    if (logg) {
      debug.print("sat: "); debug.print(sat);
      debug.print(" hdop: "); debug.print(hdopl);
      debug.print(" lat: "); debug.print(flat, 6);
      debug.print(" lon: "); debug.print(flon, 6);
      debug.print(" alt: "); debug.print(alt);
      debug.print(" cou: "); debug.print(cou);
      debug.print(" spd: "); debug.print(spd);
      debug.println();
    }
}

//**************************************************************
void sendSMSGPS() {
  Serial.print("AT+CMGS=\"");
  Serial.print(phone_number);
  Serial.println("\"");
  while(Serial.read()!='>');

  Serial.print("http://maps.google.com/maps?ll=");
  Serial.print(flat, 6);
  Serial.print(flon, 6);
  Serial.print("&q=");
  Serial.print(flat, 6);
  Serial.print(",");
  Serial.print(flon, 6);
  Serial.println("");

  Serial.print("lat: ");
  Serial.print(flat,6);
  Serial.print(" lon: ");
  Serial.print(flon,6);
  Serial.print(" cou: ");
  Serial.print(cou);
  Serial.print(" spd: ");
  Serial.print(spd);
  Serial.print(" alt: ");
  Serial.print(alt);
  Serial.println("");//

  delay(500);
  Serial.write(0x1A);       //sends ++
  Serial.write(0x0D);
  Serial.write(0x0A);
}

//**************************************************************
void SendGPSDataToServer(boolean nospd) {

  GetGPSData(gps);

  //nospd = true;
  
  if (spd < 5 && !nospd) {
    if (logg) {debug.println("speed is less then 5 km/h");} 
    prevMillis = millis();
    return;
  }
  
  if (logg) {debug.println("trying sending gps data...");} 
  if (logg) {debug.println("AT+KUDPSND..."); }
  Serial.print("AT+KUDPSND=1,");
  Serial.write(34);
  Serial.print("87.254.138.72");
  Serial.write(34);
  Serial.print(",30080");
  Serial.println(",70");
  delay(4000);
  getSerialChars();

  if (logg) {debug.println("sending udp data..."); }
  Serial.print("GET /test.php?lat=");
  Serial.print(flat,6);
  Serial.print("&lon=");
  Serial.print(flon,6);
  Serial.print("&cou=");
  Serial.print(cou);
  Serial.print("&alt=");
  Serial.print(alt);
  Serial.print("&spd=");
  Serial.print(spd);
  Serial.println(" HTTP/1.0");
  Serial.write(10);
  Serial.write(13);
  Serial.print("--EOF--Pattern--");
  delay(1000);
  Serial.println();
  //getSerialChars();

//  if (logg) {debug.println("AT+KUDPRCV...");}
//  Serial.println("AT+KUDPRCV=1,70");
//  delay(4000);
//  getSerialChars();

//  if (logg) {debug.println("close connections...");}
//  Serial.println("AT+KUDPCLOSE=1");
//  delay(1000);
//  getSerialChars();

  prevMillis = millis();
  if (logg) {debug.println("data sending complete.");}
}

//**************************************************************
void deleteAllMsgs() {
  delay(1000);
  Serial.println("AT+CMGD=1,4");
  delay(1000);
}

