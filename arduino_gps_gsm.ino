//message format:
//password#command*time
//  3425#start*30
//

#include <MsTimer2.h>
//#include <SimpleTimer.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#define maxLength 256

int8_t answer;

String inData = String(maxLength);
char data[maxLength];
char phone_number[]="+79222620280";
int x;
int onModulePin = 2;
int heater = 13;
boolean logg = true;
boolean first_loop = true;
boolean nospd = false; // false = send data if speed > 5
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
  // Debug port
  debug.begin(9600);
  delay(200);

  // GSM <--
  pinMode(onModulePin, OUTPUT);
  pinMode(heater, OUTPUT);
  Serial.begin(9600);
  if (logg) {
    debug.println("Starting...");
  }
  
  power_on();
  
  delay(3000);

  if (logg) {
    debug.println("Connecting to the GSM network...");
  }

  // waiting GSM registration
  while(sendATcommand2("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 1000) == 0);
  
  // Calling line identification presentation. 1: enable 
  Serial.println("AT+CLIP=1");
  delay(1000);
  
  // Select SMS message format. 1: text mode
  Serial.println("AT+CMGF=1");
  delay(1000);
  
  // New SMS message indication.
  Serial.println("AT+CNMI=2,2,0,0,0");
  delay(1000);

  if (logg) {
    debug.println("Connected to the GSM network...");
  }
  
  // deliting SMS messages
  while(sendATcommand2("AT+CMGD=1,4", "OK", "OK", 2000) == 0);

  // GPS
  if (logg) {
    debug.println("Setting up GPS...");
  }
  nss.begin(4800);
  if (logg) {
    debug.println("Done setting up GPS!");
  }  
  delay(200);

  HeaterStop();
   
  InitGPRS();
  
  if (logg) {
    debug.println("Started!");
  }  
}

//**************************************************************
void power_on(){

    uint8_t answer=0;
    
    // checks if the module is started
    answer = sendATcommand2("AT", "OK", "OK", 2000);
    if (answer == 0)
    {
        // power on pulse
        digitalWrite(onModulePin,HIGH);
        delay(3000);
        digitalWrite(onModulePin,LOW);
    
        // waits for an answer from the module
        while(answer == 0){     // Send AT every two seconds and wait for the answer
            answer = sendATcommand2("AT", "OK", "OK", 2000);    
        }
    }
    
}

//**************************************************************
int8_t sendATcommand2(char* ATcommand, char* expected_answer1, char* expected_answer2, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize the string

    delay(100);

    while( Serial.available() > 0) Serial.read();    // Clean the input buffer

    Serial.println(ATcommand);    // Send the AT command 

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        if(Serial.available() != 0){    // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = Serial.read();
            x++;
            if (strstr(response, expected_answer1) != NULL)    // check if the desired answer 1  is in the response of the module
            {
                answer = 1;
            }
            else if (strstr(response, expected_answer2) != NULL)    // check if the desired answer 2 is in the response of the module
            {
                answer = 2;
            }
        }
    }
    while((answer == 0) && ((millis() - previous) < timeout));    // Waits for the asnwer with time out

    return answer;
}

//**************************************************************
void InitGPRS() { 

  if (logg) {debug.println("AT&k3 - Hardware flow control activation.");}
  // Flow control option. 3: Enable bi-directional hardware flow control.
  Serial.println("AT&k3");
  delay(1000);
  getSerialChars();

  // GPRS Connection Configuration.
  if (logg) {debug.println("AT+KCNXCFG - Setting GPRS paraameters (APN, login...).");}
  while(sendATcommand2("AT+KCNXCFG=0,\"GPRS\",\"internet\",\"\",\"\",\"0.0.0.0\",\"0.0.0.0\",\"0.0.0.0\"", "OK", "OK", 2000) == 0);

  // UDP Connection Configuration
  if (logg) {debug.println("AT+KUDPCFG - Creating a new UDP socket."); }
  while(sendATcommand2("AT+KUDPCFG=0,0", "OK", "OK", 4000) == 0);
  
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
    if (command == "nospd") {
      nospd = !nospd;
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
      if (logg) {debug.println("start by ring for 20 min.");}
      delay(1000);
      Serial.println("ATH"); // disconnect the call
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
    SendGPSDataToServer(nospd);
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
  digitalWrite(heater, HIGH);
}

//**************************************************************
void HeaterStop() {
  digitalWrite(heater, LOW);
  MsTimer2::stop();
  if (logg) {debug.println("Heater stopped.");}

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
static bool feedgps(){
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
  Serial.write(0x1A); //sends ++
  Serial.write(0x0D);
  Serial.write(0x0A);
}

//**************************************************************
void SendGPSDataToServer(boolean nospd) {

  GetGPSData(gps);

  if (spd < 5 && !nospd) {
    if (logg) {debug.println("Data didn't send. Speed is less 5 km/h");} 
    prevMillis = millis();
    return;
  }
  
  if (logg) {debug.println("Connecting to server...");} 

  while(sendATcommand2("AT+KUDPSND=1,\"87.254.138.72\",8186,70", "OK", "CONNECT", 4000) == 0);

  if (logg) {debug.println("sending data..."); }
  Serial.print("lat=");
  Serial.print(flat,6);
  Serial.print(";lon=");
  Serial.print(flon,6);
  Serial.print(";cou=");
  Serial.print(cou);
  Serial.print(";alt=");
  Serial.print(alt);
  Serial.print(";spd=");
  Serial.println(spd);
  
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
  if (logg) {debug.println("data sent.");}
}

//**************************************************************
void deleteAllMsgs() {
  delay(1000);
  Serial.println("AT+CMGD=1,4");
  delay(1000);
}

