#define min(a,b) ((a)<(b)?(a):(b))
const String siteID="27101976";
char servername[] = "davidemercanti.altervista.org"; //change it

// variabili per Base64
#include <rBase64.h>
rBase64generic<512> mybase64;

//varibili per ritardi
#include <AsyncDelay.h>
 AsyncDelay delay_1m;
 
// variabili per Wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
const char* ssid1     = "dlinkMeteo"; 
const char* ssid2    = "d_mercanti_2570"; 
const char* wifi_password = "0192837465";
IPAddress ip(192, 168, 1, 35);
IPAddress ipDNS(8, 8, 8, 8);
IPAddress ipGateway(192, 168, 1, 1);
IPAddress ipMask(255, 255, 255, 0);
String currentIP = "";
String currentSSID="";
  WiFiClient client;  //per upload foto

// variabili per camera
#include <Adafruit_VC0706.h>
#include <SoftwareSerial.h>         
SoftwareSerial CameraConnection(D4, D3);
Adafruit_VC0706 Camera = Adafruit_VC0706(&CameraConnection);

//variabili per webserver
#include <ESP8266WebServer.h>
ESP8266WebServer WEBServer(80);
/***** Web Pages ************************************************************************/

String HeaderPage = "<!DOCTYPE html><html><body><header>-=:: Davide Mercanti meteoCam2 ::=-</header>";
String FooterPage = "<footer>powerd by http://davidemercanti.altervista.it/centr/webcam.html</br>d.mercanti@gmail.com</footer></body></html>";
String rootPageBody = "<h2>Stazione Meteo</h2>";

String statusManager = "<script language=\"javascript\">\r\n"
                       "window.onload = function(e){Pinstatus();}\r\n"
                       "function Pinstatus(){ morestatus();}\r\n"
                       "function morestatus(){"
                       "setTimeout(morestatus, 5000);"
                       "request = new XMLHttpRequest();"
                       "request.onreadystatechange = updateasyncstatus;"
                       "request.open(\"GET\", \"/status\", true);"
                       "request.send(null);"
                       "}\r\n"
                       "</script>\r\n";
String rootWebPage;
String webStatus = "";

//Variabili Barometro
#include <EnvironmentCalculations.h>
#include <BME280I2C.h>
#include <Wire.h>
BME280I2C::Settings settingsBme(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);

BME280I2C BME_Sensor(settingsBme);
// Assumed environmental values:
float referencePressure =  1013.25;  // hPa local QFF (official meteor-station reading)
float outdoorTemp = 25;           // °C  measured local outdoor temp.
float barometerAltitude = 195;  // meters ... map readings + barometer position
float temp(NAN), hum(NAN), pres(NAN);

  
//variabili RTC
#include <MCP7940.h>
MCP7940_Class MCP7940;   


#include <ESP8266HTTPClient.h>


//Variabili applicazione
unsigned int sensori[10];
volatile unsigned int contaTick=0;

  int TIME_HH = 0;
  int TIME_MM = 0;
  int TIME_SS = 0;
  int TIME_GG = 0;
  int TIME_ME = 0;
  int TIME_AA = 0;
  int TIME_DD = 0;
  unsigned int prevIDX=-1;
  int current_HH = 0;
  int current_MM = 0;
  int current_DD = 0;

  bool uploadFotoInProgress=false;
int conteggioMinutiOra=60;
//==================================================================================================
//  splitString : splitta la stringa usando il separatore sep resituendo la posizione index
//==================================================================================================
String splitString(String str, char sep, int index){

  int found = 0;
  int strIdx[] = { 0, -1 };
  int maxIdx = str.length() - 1;

  for (int i = 0; i <= maxIdx && found <= index; i++)
  {
    if (str.charAt(i) == sep || i == maxIdx)
    {
      found++;
      strIdx[0] = strIdx[1] + 1;
      strIdx[1] = (i == maxIdx) ? i + 1 : i;
    }
  }
  return found > index ? str.substring(strIdx[0], strIdx[1]) : "";
}
//==================================================================================================
//  string2char : converte String a Char*
//==================================================================================================
char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}


//==================================================================================================
//  IPTOSTR : converte IP a stringa
//==================================================================================================
String ipToStr (  IPAddress IP) {
  if (IP[0] > 0)
    return  String(IP[0]) + '.' + String(IP[1]) + '.' + String(IP[2]) + '.' + String(IP[3]);
  else return "";
}
//==================================================================================================
//  SETUPNETWORK : Inizializza la rete WiFi
//==================================================================================================
void setupNetwork(void) {
 if (WiFi.status() == WL_CONNECTED) return;

  wifi_set_sleep_type(NONE_SLEEP_T);
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.persistent(false);
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
 byte cnt = 11;
   WiFi.mode(WIFI_STA);

 // important delay, it doesn't send data to the server without it (only needed if using lwIP v2)
 delay(4000); 

 WiFi.config(ip,ipGateway,ipMask,ipDNS); 
 // tentativo al primo access point
 Serial.print("Connecting to 1) "); Serial.println(ssid1);
 WiFi.begin(ssid1, wifi_password);
  while ((WiFi.status() != WL_CONNECTED) && (cnt != 0)) {
    yield(); delay(1000);
    Serial.print(".");
    cnt=cnt-1;
  }
  Serial.println();
   // tentativo al secondo access point
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to 2) "); Serial.println(ssid2);
    WiFi.begin(ssid2, wifi_password);
    cnt = 11;
    while ((WiFi.status() != WL_CONNECTED) && (cnt != 0)) {
      yield(); delay(1000);
      Serial.print("*");
    }
  }

  Serial.println("");
  currentIP = ipToStr(WiFi.localIP());
  currentSSID=WiFi.SSID();
  WiFi.printDiag(Serial);
  Serial.println("*** setupNetwork FINE");
  Serial.print("WiFi connesso con IP address: "); Serial.print(currentIP + " "); Serial.println(currentSSID);
}
//==================================================================================================
//  SETUPCAMERA : Inizializza la CAMERA
//==================================================================================================
void setupCamera() {

   Serial.println("*** setupCamera INIZIO");
  if (Camera.begin()) {
    Serial.println("    Camera Found:");
  } else {
    Serial.println("    No camera found????");
    return;
  }
  // Print out the camera version information (optional)
  char *reply = Camera.getVersion();
  if (reply == 0) {
    Serial.print("    Failed to get camera version");
  } else {
    Serial.println("----CameraInfo---");
    Serial.print(reply);
    Serial.println();
   
  }
  Camera.setImageSize(VC0706_640x480);        // biggest

  uint8_t imgsize = Camera.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");
   Serial.println("-----------------");
 Serial.println("*** setupCamera FINE");

}
//==================================================================================================
//  HANDLEROOT :  pagina principale del webserver
//==================================================================================================
void handleRoot() {
  WEBServer.send(200, "text/html", rootWebPage);
}
//==================================================================================================
//  HANDLEGETSTATUS :  pagina dello stato dei sensori
//==================================================================================================
void handleGetStatus() {

  String vo = "{\"sensori\": [" + String(sensori[0]) + "," + String(sensori[1]) + "," + String(sensori[2]) + "," + String(sensori[3]) + "," + String(sensori[4]) + "," + String(sensori[5]) + "," + String(sensori[6]) + "," + String(sensori[7]) + "," + String(sensori[8]) + "," + String(sensori[9]) + "],\"IP\": \"" + currentIP + "\",\"status\": \"" + webStatus + "\",\"SSID\": \""+ currentSSID + "\"}";
  WEBServer.send(200, "text/plain", vo);

}
//==================================================================================================
//  COMPOSEWEBPAGE :  generazione pagine root
//==================================================================================================
void composeWebPage() {
  rootWebPage = HeaderPage;
  rootWebPage += rootPageBody;
  rootWebPage += "<div id =\"description\"></div>";
  rootWebPage += FooterPage;
  rootWebPage += statusManager;
  rootWebPage += "<script language=\"javascript\">";
  rootWebPage += "function updateasyncstatus(){\r\n";
  rootWebPage += "if ((request.readyState == 4) && (request.status == 200))\r\n";
  rootWebPage += "{document.getElementById(\"description\").innerHTML = request.responseText;}";
  rootWebPage += "}\r\n";

  rootWebPage += "</script>";


}
//==================================================================================================
//  SendHTMLSimple :  pagina principale
//==================================================================================================
void SendHTMLSimple(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
ptr +="<title>Centrale Meteo Merco</title>\n";
ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
ptr +="</style>\n";
ptr +="</head>\n";
ptr +="<body>\n";
ptr +="<div id=\"webpage\">\n";
ptr +="<h1>CENTRALE METEO MERCO</h1>\n";
ptr +="<p>Temperature: ";
ptr +=temp;
ptr +="&deg;C</p>";
ptr +="<p>Humidity: ";
ptr +=hum;
ptr +="%</p>";
ptr +="<p>Pressure: ";
ptr +=pres;
ptr +="hPa</p>";

ptr +="</div>\n";
ptr +="</body>\n";
ptr +="</html>\n";
  WEBServer.send(200, "text/html", ptr);
}
//==================================================================================================
//  SETUPWEBSERVER :  Setup del server web
//==================================================================================================
void SetupWebServer() {
  composeWebPage();
  WEBServer.on("/", handleRoot);
   WEBServer.on("/meteo", SendHTMLSimple);
  WEBServer.on("/status", handleGetStatus);
  WEBServer.begin();
   Serial.println("*** SetupWebServer");

}
//==================================================================================================
//  SETUPBme :  Setup del sensore barometrico
//==================================================================================================
void SetupBme() {
   Serial.println("*** SetupBme INIZIO");
  Wire.begin();

  while(!BME_Sensor.begin())
  {
    Serial.println("    Non trovo BME280 sensor!");
    delay(1000);
  }

  switch(BME_Sensor.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("    Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("    Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("    Found UNKNOWN sensor! Error!");
  }
 
   Serial.println("*** SetupBme FINE");
}

//==================================================================================================
//  setupRTC :  Setup del sensore RTC
//==================================================================================================
void setupRTC() {
   Serial.println("*** setupRTC INIZIO");
    while (!MCP7940.begin()) {                                                  // Initialize RTC communications    //
      Serial.println(F("    Unable to find MCP7940M. Checking again in 1s."));      // Show error text                  //
      delay(1000);                                                              // wait a second                    //
    } // of loop until device is located                                        //                                  //
    Serial.println(F("    MCP7940 initialized."));                                  //                                  //
    while (!MCP7940.deviceStatus()) {                                           // Turn oscillator on if necessary  //
      Serial.println(F("    Oscillator is off, turning it on."));                   //                                  //
      bool deviceStatus = MCP7940.deviceStart();                                // Start oscillator and return state//
      if (!deviceStatus) {                                                      // If it didn't start               //
        Serial.println(F("    Oscillator did not start, trying again."));           // Show error and                   //
        delay(1000);                                                            // wait for a second                //
      } // of if-then oscillator didn't start                                   //                                  //
    } // of while the oscillator is off                                         //                                  //
    MCP7940.adjust();      
  
     DateTime now = MCP7940.now();  
     Serial.println("    DATA setup");
     Serial.print("    "); Serial.print(now.year()); Serial.print(now.month());
      Serial.print(now.day()); Serial.print(now.hour()); Serial.print(now.minute());
      Serial.println("----");
    Serial.println("*** setupRTC FINE");     
}
//==================================================================================================
//  SETUP : Inizializzazione 
//==================================================================================================
void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println("==================");
    Serial.println("Centrale Meteo ][");
    Serial.println("==================");
    setupNetwork();
    setupCamera();
    SetupBme();
    setupRTC();
    SetupWebServer();
    Serial.println("=================================");
    Serial.println(" Starting Centrale Meteo ][ ...");
    Serial.println("=================================");
     sincroNTP(1);
    leggiSensori();
    // setup per interrupt
    pinMode(D5, INPUT); 
   
    //Serial.setDebugOutput(false);
  delay_1m.start(60000, AsyncDelay::MILLIS);


}
//==================================================================================================
//  leggiSensore0123 : Lettura dei sensori del barometro 
//==================================================================================================
void leggiSensore0123() {
   Serial.println("====BME sensor reading...");
   

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);
   BME_Sensor.read(pres, temp, hum, tempUnit, presUnit);
  

   Serial.print("Temp: ");
   Serial.print(temp);
   Serial.print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? "C" :"F"));
   Serial.print("\t\tHumidity: ");
   Serial.print(hum);
   Serial.print("% RH");
   Serial.print("\t\tPressure: ");
   Serial.print(pres);
   Serial.print(String(presUnit == BME280::PresUnit_hPa ? "hPa" : "Pa")); // expected hPa and Pa only



    EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

   /// To get correct local altitude/height (QNE) the reference Pressure
   ///    should be taken from meteorologic messages (QNH or QFF)
   float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, outdoorTemp, envTempUnit);

   float dewPoint = EnvironmentCalculations::DewPoint(temp, hum, envTempUnit);

   /// To get correct seaLevel pressure (QNH, QFF)
   ///    the altitude value should be independent on measured pressure.
   /// It is necessary to use fixed altitude point e.g. the altitude of barometer read in a map
   float seaLevel = EnvironmentCalculations::EquivalentSeaLevelPressure(barometerAltitude, temp, pres, envAltUnit, envTempUnit);

   float absHum = EnvironmentCalculations::AbsoluteHumidity(temp, hum, envTempUnit);

   Serial.print("\t\tAltitude: ");
   Serial.print(altitude);
   Serial.print((envAltUnit == EnvironmentCalculations::AltitudeUnit_Meters ? "m" : "ft"));
   Serial.print("\t\tDew point: ");
   Serial.print(dewPoint);
   Serial.print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   Serial.print("\t\tEquivalent Sea Level Pressure: ");
   Serial.print(seaLevel);
   Serial.print(String( presUnit == BME280::PresUnit_hPa ? "hPa" :"Pa")); // expected hPa and Pa only

   Serial.print("\t\tHeat Index: ");
   float heatIndex = EnvironmentCalculations::HeatIndex(temp, hum, envTempUnit);
   Serial.print(heatIndex);
   Serial.print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));

   Serial.print("\t\tAbsolute Humidity: ");
   Serial.println(absHum);
   

   sensori[0]=(temp+50)*100;
   sensori[1]=hum*100;
   sensori[2]=(pres-500)*100;
   sensori[3]=(heatIndex+50)*100;
   sensori[4]=absHum*100;
   sensori[5]=(seaLevel-500)*100;
   sensori[6]=(dewPoint+50)*100;

   Serial.println("====BME sensor Done!");
}
//==================================================================================================
//  leggiSensori : Lettura di tutti i sensori
//==================================================================================================
void leggiSensori() {
  unsigned long sStart = millis();
  leggiSensore0123();
  sensori[9]=contaTick;

}
//==================================================================================================
//  getTimeWeb : Recupera orario da una pagina PHP
//==================================================================================================
void getTimeWeb() {

   TIME_HH = 0;
   TIME_MM = 0;
   TIME_SS = 0;
   TIME_GG = 0;
   TIME_ME = 0;
   TIME_AA = 0;
   TIME_DD = 0;
   Serial.println("*** getTimeWeb INIZIO");
  HTTPClient http;
  http.begin("http://davidemercanti.altervista.org/nownowday.php");
  int httpCode = http.GET();
  String payload = "";

  if (httpCode == HTTP_CODE_OK) {

    payload = http.getString();
    Serial.println(payload);
    int ind1 = payload.indexOf(' ');
    if (ind1 > 0) {

      String payload1 = splitString(payload, ' ', 0);
      TIME_GG = splitString(payload1, '-', 0).toInt();
      TIME_ME = splitString(payload1, '-', 1).toInt();
      TIME_AA = splitString(payload1, '-', 2).toInt();
      Serial.println(TIME_GG);
      Serial.println(TIME_ME);
      Serial.println(TIME_AA);

      String payload2 = splitString(payload, ' ', 1);
      Serial.println(payload2);
      TIME_HH = splitString(payload2, ':', 0).toInt();
      TIME_MM = splitString(payload2, ':', 1).toInt();
      TIME_SS = splitString(payload2, ':', 2).toInt();
      Serial.println(TIME_HH);
      Serial.println(TIME_MM);
      Serial.println(TIME_SS);


      payload = splitString(payload, ' ', 2);
      Serial.println(payload);
      TIME_DD = payload.toInt();
       Serial.println(TIME_DD);
    }
  }
  http.end();
     Serial.println("*** getTimeWeb FINE");
}
//==================================================================================================
//  sincroNTP : Aggiorna MCP7940
//==================================================================================================
void sincroNTP (int forceRTC) {
Serial.println("*** sincroNTP INIZIO");
   DateTime nowo = MCP7940.now();  
  Serial.println("    Sincro: DATA MCP7940");
  Serial.print(nowo.year());
  Serial.print(nowo.month());
  Serial.print(nowo.day());
  Serial.print(nowo.hour());
  Serial.print(nowo.minute());
   Serial.println("");
  getTimeWeb();

 
  


if ((TIME_HH!=0) && (TIME_MM!=0) && (forceRTC==1)) {
    MCP7940.adjust(DateTime(TIME_AA,TIME_ME,TIME_GG,TIME_HH,TIME_MM,TIME_SS)); 
    nowo = MCP7940.now();  
     Serial.println("   DATA reimpostata");
      Serial.print(nowo.year());
      Serial.print(nowo.month());
      Serial.print(nowo.day());
      Serial.print(nowo.hour());
      Serial.print(nowo.minute());
       Serial.println("");
        current_HH =TIME_HH;
      current_MM = TIME_MM;
     current_DD = TIME_DD;

}
else {

       current_HH =nowo.hour();
      current_MM = nowo.minute();
     current_DD = nowo.dayOfTheWeek();

}
}
//==================================================================================================
//  getSlotNumber : numero di slot  per salvataggio
//==================================================================================================
unsigned int getSlotNumber() {

  return (current_HH * 12) + (current_MM / 5);

}
//==================================================================================================
//  getIDX : indice per salvataggio web
//==================================================================================================
unsigned int getIDX() {


  return (current_DD * 288) + getSlotNumber() + 1;
}
//==================================================================================================
//  wwwSQL : salvataggio web
//==================================================================================================
void wwwSQL(String SQL, String Command) {
  HTTPClient http;
  SQL = Command + SQL;
  mybase64.encode(SQL);
  http.begin("http://davidemercanti.altervista.org/centr/s.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String data = mybase64.result();
  data = "SQLDATA=" + data + "&KEY="+siteID;
  int httpCode = http.POST(data);

  if (httpCode == HTTP_CODE_OK) {
    Serial.print("wwwSQL HTTP response code "); Serial.println(httpCode);
    String response = http.getString();
    Serial.println(response);
   
  }
  http.end();

}

//==================================================================================================
//  getDateTimeStamp : stringa time per salvataggio WEB nel formato AAAAMMGG
//==================================================================================================
String getDateTimeStamp() {


   uint16_t year =TIME_AA ;
   String yearStr = String(year);

   uint8_t month = TIME_ME;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = TIME_GG;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

  

   return yearStr + "" + monthStr + "" + dayStr ;
  
}

//==================================================================================================
//  insertValuesCloud : salvataggio sulla tabella centrale_meteo (attuali) e  centrale_meteo_storico
//==================================================================================================
void insertValuesCloud() {


  String tsd=getDateTimeStamp();
  String idx=String(getIDX());
     Serial.print("*** insertValuesCloud "); Serial.print("IDX ");  Serial.print(idx);  Serial.print(" TS ");  Serial.println(tsd);
  String SQL="";
  SQL="DELETE FROM centrale_meteo WHERE site="+siteID+" AND id="+idx+";;;";
  SQL=SQL+"INSERT INTO centrale_meteo (site,id,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)"; 
  SQL=SQL+" VALUES ("+siteID+","+idx+","+String(sensori[0])+","+String(sensori[1])+","+String(sensori[2])+","+String(sensori[3])+","+String(sensori[4])+","+String(sensori[5])+","+String(sensori[6])+","+String(sensori[7])+","+String(sensori[8])+","+String(sensori[9])+");;;";
  SQL=SQL+"INSERT INTO centrale_meteo_storico (datarif,site,id,s0,s1,s2,s3,s4,s5,s6,s7,s8,s9)"; 
  SQL=SQL+" VALUES ("+tsd+","+siteID+","+idx+","+String(sensori[0])+","+String(sensori[1])+","+String(sensori[2])+","+String(sensori[3])+","+String(sensori[4])+","+String(sensori[5])+","+String(sensori[6])+","+String(sensori[7])+","+String(sensori[8])+","+String(sensori[9])+")";
    
    
  Serial.println(SQL);
  wwwSQL(SQL,"!");
}
//==================================================================================================
//  storicizza : richiama la storicizza php
//==================================================================================================
void storicizza() {
  HTTPClient http;
  http.begin("http://davidemercanti.altervista.org/centr/storicizza.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String data = "";
   String idx=String(getIDX());
  
  data = "ID=" + String(current_DD) + "&KEY="+siteID;

    Serial.print("*** storicizza "); Serial.print("IDX ");  Serial.print(idx);  Serial.print(" Post data ");  Serial.println(data);
  int httpCode = http.POST(data);
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("     storicizza DONE ");
  }
  else {Serial.print("     storicizza ERROR ");}
  http.end();
}
//==================================================================================================
//  shadowCopyWebCam : aggiorna l'immagine sul server con informazioni
//==================================================================================================
void shadowCopyWebCam() {
 HTTPClient http;
  http.begin("http://davidemercanti.altervista.org/centr/webcamManager.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

   String idx=String(getIDX());
   String data = "";
  data = "SC=" + idx + "&KEY="+siteID;
  Serial.print("*** shadowCopyWebCam "); Serial.print("IDX ");  Serial.print(idx);  Serial.print(" Post data ");  Serial.println(data);
  
  int httpCode = http.POST(data);
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("    Shadow copy DONE ");
    Serial.println(data);
    
  }
  else {Serial.print("    Shadow copy ERROR ");}
  http.end();
}

//==================================================================================================
//  savePicture : salvataggio foto su web
//==================================================================================================
void savePicture() { 
  Camera.resumeVideo();
 uploadFotoInProgress=true;
  Serial.println("*** savePicture ");
      delay(3000);
  if (!Camera.takePicture()) {
     uploadFotoInProgress=false;
    Serial.println("   savePicture Failed to snap!");
     return;
  }
  else 
    { 
      Serial.println("   savePicture Picture taken!"); 
     
     }
  

// Get the size of the image (frame) taken  
  uint16_t jpglen = Camera.frameLength();
  Serial.print("    Storing ");   Serial.print(jpglen, DEC); Serial.println(" byte image.");
  int32_t time = millis();
  //client.setNoDelay(false);
  //client.setSync(true);
      client.setTimeout(15000);
  if (client.connect(servername, 80)) {
      
    
      Serial.println("    Connected to Server...");
      client.println("POST /centr/i.php HTTP/1.1");
      client.println("Host: davidemercanti.altervista.org"); 
      client.println("Connection: close");
      client.println("Cache-Control: max-age=0");
      client.println("Content-Type: multipart/form-data; boundary=---------------------------220743574125969096022348026247");
      client.println("Content-Length: " + String(jpglen+244)); //file size  //change it
      //client.println("Content-Length: 0"); //file size  //change it
      client.println();
      client.println("-----------------------------220743574125969096022348026247");
      client.println("Content-Disposition: form-data; name=\"fileToUpload\"; filename=\"" + siteID +".jpg\""); //change it
      client.println("Content-Type: application/octet-stream");
     client.println();
      
    // Read all the data up to # bytes!
    byte wCount = 0; // For counting # of writes
     uint8_t bytesToRead=0;
     uint8_t *buffer;
     uint8_t bytesW =0;
    while (jpglen > 0) {
      // read 32 bytes at a time;
      
     bytesToRead = min(64, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
       
        //yield();
       
      buffer = Camera.readPicture(bytesToRead);


      if(++wCount >= 64) { // Every 4K, give a little feedback so it doesn't appear locked up
         Serial.print('.');
         Serial.println(jpglen, DEC);
        wCount = 0;
      }
  
     bytesW  =  client.write((const uint8_t *) buffer, bytesToRead);
  


   if (bytesToRead!=bytesW)  {
    Serial.println("    SCRITTI BYTE DIVERSI");
    Serial.print("    Volevo scriverne ");  Serial.print(bytesToRead , DEC);   Serial.print(" ---- Ne ho scritti ");  Serial.print(bytesW , DEC); 
   }
      jpglen -= bytesToRead;
     
    }
    client.println(); //file end


      client.println("-----------------------------220743574125969096022348026247--");
     // client.println();
      Serial.print("    byte image upload end...wait for response");
      if(!eRcv());
      
       client.stop();

        uploadFotoInProgress=false;
 }

  time = millis() - time;
  Serial.print("    done! "); Serial.print(time); Serial.println(" ms elapsed");
    
   Serial.println("*** savePicture DONE");

   shadowCopyWebCam();
} //savePicture



//==================================================================================================
//  LOOP : Loop principale
//==================================================================================================
void loop() {

WEBServer.handleClient();
  if (!uploadFotoInProgress) {
      setupNetwork();
    

       if (delay_1m.isExpired()) {
              sincroNTP(0);
              leggiSensori();
              Serial.print("Tick 1m!    "); Serial.print("prevIDX ");  Serial.println(prevIDX);
              conteggioMinutiOra=conteggioMinutiOra-1;
              if (prevIDX!=getIDX()) {
                 Serial.print("AGGIORNAMENTO INDEX : "); Serial.println(getIDX());
                 insertValuesCloud();
                 storicizza();
                 savePicture();
                 prevIDX=getIDX();
                 sincroNTP(1);
              }
             if (conteggioMinutiOra==0) {
                Serial.println("!!!!!!!!!ESP Restart...!!!!");
                 ESP.restart();
             }
              delay_1m.repeat(); // Count from when the delay expired, not now
       } //delay 1m expired
  }
  
}//loop


byte eRcv()
{
  byte respCode;
  byte thisByte;
  unsigned long previousMillis =  millis();
  unsigned long currentMillis = 0;
  while(!client.available()) {
     currentMillis = millis();
      if (currentMillis - previousMillis >= 40000) {
        Serial.println("    eRcv TIMEOUT!!!!!!!!!!!!!!");
        return 1;
      }
    delay(1);
     yield();
    
  }
 respCode = client.peek();
  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }
  return 1;
}
