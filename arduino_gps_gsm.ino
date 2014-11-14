//message format:
//password#command*time
//  3425#start*30

#include <MsTimer2.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#define maxLength 256

int8_t answer;

char data[maxLength];
char phone_number[]="+79222620280";
int x;
int onModulePin = 2;
int heater = 13;
boolean nospd = false; // false = send data if speed > 5
boolean hstarted = false; // heater started

// GPS
unsigned long age, date, time, chars = 0, sat, hdopl;
float flat, flon;
int alt, cou, spd;

TinyGPS gps;

SoftwareSerial nss(9, 8); // GPS
SoftwareSerial debug = SoftwareSerial(11, 10); // debug

//*******************************************************************************************************************************
void setup() {
  // Debug port
  debug.begin(9600);
  delay(200);

  // GSM <--
  pinMode(onModulePin, OUTPUT);
  pinMode(heater, OUTPUT);
  Serial.begin(9600);

  debug.println("Starting");

  delay(3000);
  power_on();
  delay(3000);

  debug.println("Connecting to the GSM network...");

  // waiting GSM registration
  while(sendATcommand2("AT+CREG?", "+CREG: 0,1", "+CREG: 0,5", 30000) == 0);
  
  // Calling line identification presentation. 1: enable 
  while(sendATcommand2("AT+CLIP=1", "OK", "OK", 1000) == 0);
  
  // Select SMS message format. 1: text mode
  while(sendATcommand2("AT+CMGF=1", "OK", "OK", 1000) == 0);
  
  // New SMS message indication.
  while(sendATcommand2("AT+CNMI=2,2,0,0,0", "OK", "OK", 1000) == 0);

  debug.println("Connected to the GSM network...");
    
  // deliting SMS messages
  while(sendATcommand2("AT+CMGD=1,4", "OK", "OK", 2000) == 0);

  // GPS
  debug.println("Setting up GPS...");
  nss.begin(4800);
  delay(200);
  debug.println("Done setting up GPS!");
  
  debug.println("Setting up GPRS...");
  InitGPRS();

//  debug.println("Setting up timer...");
//  delay(3000);
//  MsTimer2::set(10000, SendGPSDataToServer);
//  MsTimer2::start();
//  delay(3000);
  
  debug.println("Done!");

}

//*******************************************************************************************************************************
void loop() {
  getSerialChars();
  checksms();
  checkinccall();
  delay(1000);
  //t.update();
}

//*******************************************************************************************************************************
void power_on(){

    uint8_t answer=0;
    
    // checks if the module is started
    answer = sendATcommand2("AT", "OK", "OK", 30000);
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

//*******************************************************************************************************************************
int8_t sendATcommand2(char* ATcommand, char* expected_answer1, char* expected_answer2, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize the string

    delay(100);

    while( Serial.available() > 0) Serial.read();    // Clean the input buffer
    //getSerialChars();

    Serial.println(ATcommand);    // Send the AT command 
    delay(100);
    
    x = 0;
    previous = millis();

    // this loop waits for the answer
    debug.print("Waiting for : ");
    debug.println(ATcommand);
    
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
    
    debug.print("Response: ");
    debug.println(response);
    
    return answer;
}

//*******************************************************************************************************************************
void InitGPRS() { 

  debug.println("Setting Up hardware flow control activation...");
  sendATcommand2("AT&k3", "OK", "OK", 2000);

  // GPRS Connection Configuration.
  debug.println("Setting Up GPRS parameters...");
  while(sendATcommand2("AT+KCNXCFG=0,\"GPRS\",\"internet\",\"\",\"\",\"0.0.0.0\",\"0.0.0.0\",\"0.0.0.0\"", "OK", "OK", 2000) == 0);

  // UDP Connection Configuration
  //if (logg) {debug.println("AT+KUDPCFG - Creating a new UDP socket."); }
  //while(sendATcommand2("AT+KUDPCFG=0,0", "OK", "OK", 4000) == 0);
  
  debug.println("GPRS initiated!");

}

//*******************************************************************************************************************************
void checksms(){
  if (strstr(data,"3425#") != NULL) {
    if (strstr(data,"3425#start") != NULL) {
      HeaterStart();
    }
    if (strstr(data, "3425#stop") != NULL ) {
      HeaterStop();
    }
    if (strstr(data, "3425#getpos") != NULL ) {
      sendSMSGPS();
    }
    if (strstr(data, "3425#int") != NULL ) {
      SendGPSDataToServer();
    }
    if (strstr(data, "3425#nospd") != NULL ) {
      nospd = !nospd;
    }
    if (strstr(data, "3425#status") != NULL ) {
      HeaterStatus();
    }
    
    // deliting SMS messages
    sendATcommand2("AT+CMGD=1,4", "OK", "OK", 60000);
  }
}

//*******************************************************************************************************************************
void checkinccall(){
  // Incoming ring
  if (strstr(data, "+CLIP") != NULL) {
    if (strstr(data, phone_number) != NULL) {
      Heater();
      sendATcommand2("ATH", "OK", "OK", 3000); // disconnect the call
    }
    else {
      sendATcommand2("ATH", "OK", "OK", 3000); // disconnect the call
    }
  }

}

//**********************************************************************************************************************************************************
void switchModule(){
  digitalWrite(onModulePin,HIGH);
  delay(2000);
  digitalWrite(onModulePin,LOW);
}

//*******************************************************************************************************************************
void Heater() {
  
  if (hstarted) {
    HeaterStop();
  }
  else {
    HeaterStart();
  }
  hstarted = !hstarted;
}

//*******************************************************************************************************************************
void HeaterStart() {
  digitalWrite(heater, HIGH);
  debug.println("Heater started.");
}

//*******************************************************************************************************************************
void HeaterStop() {
  digitalWrite(heater, LOW);
  debug.println("Heater stopped.");
}

//*******************************************************************************************************************************
void HeaterStatus() {
  
  Serial.print("AT+CMGS=\"");
  Serial.print(phone_number);
  Serial.println("\"");
  while(Serial.read()!='>');

  Serial.print(hstarted);
  Serial.println("");//

  delay(500);
  Serial.write(0x1A); //sends ++
  Serial.write(0x0D);
  Serial.write(0x0A); 
}

//*******************************************************************************************************************************
void getSerialChars() {
  memset(data, '\0', sizeof(data));
  x=0;
  while (Serial.available() != 0) {
    data[x] = Serial.read();
    x++;
  }
  if (data[0] != '\0') {debug.println(data);}
}

//*******************************************************************************************************************************
static bool feedgps(){
  while (nss.available())
  {
    if (gps.encode(nss.read()))
      return true;
  }
  return false;
}

//*******************************************************************************************************************************
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
    
    debug.println("GPS data:");
    debug.print("sat:"); debug.println(sat);
    debug.print("hdop:"); debug.println(hdopl);
    debug.print("lat:"); debug.println(flat, 6);
    debug.print("lon:"); debug.println(flon, 6);
    debug.print("alt:"); debug.println(alt);
    debug.print("cou:"); debug.println(cou);
    debug.print("spd:"); debug.println(spd);
    debug.println();
}

//*******************************************************************************************************************************
void sendSMSGPS() {
  
  GetGPSData(gps);
  
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

//*******************************************************************************************************************************
void SendGPSDataToServer() {

  GetGPSData(gps);

  if (spd < 5 && !nospd) {
    return;
  }
  
  debug.println("Creating a new UDP socket...");
  if (sendATcommand2("AT+KUDPCFG=0,0", "OK", "OK", 60000) == 0) {
     debug.println("Error creating UDP socket.");
     return; 
  }

  debug.println("Connecting to server..."); 
  //while(sendATcommand2("AT+KUDPSND=1,\"87.254.138.72\",8186,70", "OK", "CONNECT", 4000) == 0);
  if (sendATcommand2("AT+KUDPSND=1,\"87.254.138.72\",8186,70", "CONNECT", "OK", 10000) == 0) {
    return;
  }

  debug.println("Sending data...");
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
  delay(100);
  Serial.println();
  getSerialChars();

//  if (logg) {debug.println("AT+KUDPRCV...");}
//  Serial.println("AT+KUDPRCV=1,70");
//  delay(4000);
//  getSerialChars();

  debug.println("Closing connections...");
  while(sendATcommand2("AT+KUDPCLOSE=1", "OK", "OK", 4000) == 0);
  
  //getSerialChars();

  //prevMillis = millis();
  debug.println("data sent.");
}

//*******************************************************************************************************************************
