
/************************************************************************/
/*                       Supported hardware:                            */
/*                                                                      */
/*  uC32 with a WiFiShield                                              */
/*  WF32                                                                */
/*  WiFIRE                                                              */
/*                                                                      */
/************************************************************************/

//******************************************************************************************
//******************************************************************************************
//***************************** SET YOUR CONFIGURATION *************************************
//******************************************************************************************
//******************************************************************************************

/************************************************************************/
/*                                                                      */
/*              Include ONLY 1 hardware library that matches            */
/*              the network hardware you are using                      */
/*                                                                      */
/*              Refer to the hardware library header file               */
/*              for supported boards and hardware configurations        */
/*                                                                      */
/************************************************************************/
#include <MRF24G.h>                     // This is for the MRF24WGxx on a pmodWiFi or WiFiShield

/************************************************************************/
/*                    Required libraries, Do NOT comment out            */
/************************************************************************/
#include <DEIPcK.h>
#include <DEWFcK.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define OLED_MOSI   32    //11
#define OLED_CLK    33    //13
#define OLED_DC     30    //39
#define OLED_CS     29    //38
#define OLED_RESET  31    //10
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
/************************************************************************/
/*                                                                      */
/*              SET THESE VALUES FOR YOUR NETWORK                       */
/*                                                                      */
/************************************************************************/

char * szIPServer = "http://api.openweathermap.org";    //server to connect to
uint16_t portServer = 80;

// Specify the SSID
const char * szSsid = "redmi1";

// select 1 for the security you want, or none for no security
#define USE_WPA2_PASSPHRASE
//#define USE_WPA2_KEY
//#define USE_WEP40
//#define USE_WEP104
//#define USE_WF_CONFIG_H

// modify the security key to what you have.
#if defined(USE_WPA2_PASSPHRASE)

const char * szPassPhrase = "87654321";
#define WiFiConnectMacro() deIPcK.wfConnect(szSsid, szPassPhrase, &status)

#elif defined(USE_WPA2_KEY)

    WPA2KEY key = { 0x27, 0x2C, 0x89, 0xCC, 0xE9, 0x56, 0x31, 0x1E, 
                    0x3B, 0xAD, 0x79, 0xF7, 0x1D, 0xC4, 0xB9, 0x05, 
                    0x7A, 0x34, 0x4C, 0x3E, 0xB5, 0xFA, 0x38, 0xC2, 
                    0x0F, 0x0A, 0xB0, 0x90, 0xDC, 0x62, 0xAD, 0x58 };
    #define WiFiConnectMacro() deIPcK.wfConnect(szSsid, key, &status)

#elif defined(USE_WEP40)

    const int iWEPKey = 0;
    WEP40KEY keySet = { 0xBE, 0xC9, 0x58, 0x06, 0x97,     // Key 0
                        0x00, 0x00, 0x00, 0x00, 0x00,     // Key 1
                        0x00, 0x00, 0x00, 0x00, 0x00,     // Key 2
                        0x00, 0x00, 0x00, 0x00, 0x00 };   // Key 3
    #define WiFiConnectMacro() deIPcK.wfConnect(szSsid, keySet, iWEPKey, &status)

#elif defined(USE_WEP104)

    const int iWEPKey = 0;
    WEP104KEY keySet = { 0x3E, 0xCD, 0x30, 0xB2, 0x55, 0x2D, 0x3C, 0x50, 0x52, 0x71, 0xE8, 0x83, 0x91,   // Key 0
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // Key 1
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // Key 2
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Key 3
    #define WiFiConnectMacro() deIPcK.wfConnect(szSsid, keySet, iWEPKey, &status)

#elif defined(USE_WF_CONFIG_H)

    #define WiFiConnectMacro() deIPcK.wfConnect(0, &status)

#else   // no security - OPEN

    #define WiFiConnectMacro() deIPcK.wfConnect(szSsid, &status)

#endif
   
//******************************************************************************************
//******************************************************************************************
//***************************** END OF CONFIGURATION ***************************************
//******************************************************************************************
//******************************************************************************************

typedef enum
{
    NONE = 0,
    CONNECT,
    TCPCONNECT,
    WRITE,
    READ,
    CLOSE,
    DONE,
} STATE;

STATE state = CONNECT;

unsigned tStart = 0;
unsigned tWait = 30000;

TCPSocket tcpSocket;
byte rgbRead[1024];

// this is for tcpSocket.writeStream to print
byte rgbWriteStream[] = "GET /data/2.5/weather?q=Kiev,ua&units=metric&appid=076b34eafcb4c2fcf0923018a3cf7fb6\nHost: api.openweathermap.org\nConnection: close\n";
int cbWriteStream = sizeof(rgbWriteStream);

const char *weather;
double temp;
double temp_min;
double temp_max;
int pressure;
int humidity;
double wind_speed;
int wind_deg;
const char *city; 

/***        void setup()
 *
 *        Parameters:
 *          None
 *              
 *        Return Values:
 *          None
 *
 *        Description: 
 *        
 *      Arduino setup function.
 *      
 *      Initialize the Serial Monitor, and initializes the
 *      connection to the TCPEchoServer
 *      Use DHCP to get the IP, mask, and gateway
 *      by default we connect to port 44300
 *      
 * ------------------------------------------------------------ */
void setup()
{
//    Serial.begin(9600);
//    Serial.println("WiFiTCPEchoClient 3.0");
//    Serial.println("Digilent, Copyright 2014");
//    Serial.println("");

    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
    display.begin(SSD1306_SWITCHCAPVCC);
    // Clear the buffer.
    display.clearDisplay();
    // text display tests
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("WiFi weather");
    display.display();
    delay(500);
    //display.clearDisplay();
}

/***        void loop()
 *
 *        Parameters:
 *          None
 *              
 *        Return Values:
 *          None
 *
 *        Description: 
 *        
 *      Arduino loop function.
 *      
 *      We are using the default timeout values for the DEIPcK and TcpClient class
 *      which usually is enough time for the Tcp functions to complete on their first call.
 *
 *      This code will write  some stings to the server and have the server echo it back
 *      
 * ------------------------------------------------------------ */
void loop() {
    IPSTATUS status;
    int cbRead = 0;

    switch(state)
    {

        case CONNECT:
            if(WiFiConnectMacro())
            {
//                Serial.println("WiFi connected");
                display.setCursor(0,10);
                display.print("Connected to ");
                display.println(szSsid);
                display.display();
                delay(2000);
                //display.clearDisplay();
                deIPcK.begin();
                state = TCPCONNECT;
            }
            else if(IsIPStatusAnError(status))
            {
//                Serial.print("Unable to connection, status: ");
//                Serial.println(status, DEC);
                display.setCursor(0,10);
                display.println("Unable to connection, status: ");
                display.println(status, DEC);
                display.display();
                delay(500);
                state = CLOSE;
            }
            break;

        case TCPCONNECT:
            //Serial.println("____");
            if(deIPcK.tcpConnect(szIPServer, portServer, tcpSocket))
            {
//                Serial.println("Connected to server.");
                display.setCursor(0,20);
                display.println("Loading weather from server");
                display.display();
                delay(2000);
                state = WRITE;
            }
        break;

        // write out the strings
        case WRITE:
            if(tcpSocket.isEstablished())
            {     
                tcpSocket.writeStream(rgbWriteStream, cbWriteStream);
 
//                Serial.println("Bytes Read Back:");
                state = READ;
                tStart = (unsigned) millis();
            }
            break;

            // look for the echo back
        case READ:
            // see if we got anything to read
            if((cbRead = tcpSocket.available()) > 0)
            {
                cbRead = cbRead < (int) sizeof(rgbRead) ? cbRead : sizeof(rgbRead);
                cbRead = tcpSocket.readStream(rgbRead, cbRead);

//                for(int i=0; i < cbRead; i++)
//                {
//                    Serial.print((char) rgbRead[i]);
//                }
                json_parse();
                displ_weather();
            }

            // give us some time to get everything echo'ed back
            else if( (((unsigned) millis()) - tStart) > tWait )
            {
//                Serial.println("");
                state = CLOSE;
            }
            break;

        // done, so close up the tcpSocket
        case CLOSE:
            tcpSocket.close();
//            Serial.println("Closing TcpClient, Done with sketch.");
            state = TCPCONNECT;
            break;

        case DONE:
        default:
            break;
    }

    // keep the stack alive each pass through the loop()
    DEIPcK::periodicTasks();
}

void displ_weather(void) {
//    Serial.println(weather);
//    Serial.println(temp);
//    Serial.println(temp_min);
//    Serial.println(temp_max);
//    Serial.println(pressure);
//    Serial.println(humidity);
//    Serial.println(wind_speed);
//    Serial.println(wind_deg);
//    Serial.println(city);

    display.clearDisplay();
    display.setCursor(0,0);
    display.print(city);
    display.print(": ");
    display.println(weather);
    
    display.setCursor(0,10);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");
    
    display.setCursor(0,20);
    display.print("Max temp: ");
    display.print(temp_max);
    display.println(" C");
    
    display.setCursor(0,30);
    display.print("Min temp: ");
    display.print(temp_min);
    display.println(" C");
        
    display.setCursor(0,40);
    display.print("Preassure: ");
    display.println(pressure);
    
    display.setCursor(0,50);
    display.print("Humidity: ");
    display.println(humidity);
    
    display.display();
    delay(2000);
    display.clearDisplay();
}

void json_parse() {
    // Memory pool for JSON object tree.
    //
    // Inside the brackets, 200 is the size of the pool in bytes.
    // Don't forget to change this value to match your JSON document.
    // Use arduinojson.org/assistant to compute the capacity.
    StaticJsonBuffer<1200> jsonBuffer;
    // StaticJsonBuffer allocates memory on the stack, it can be
    // replaced by DynamicJsonBuffer which allocates in the heap.
    //
    // DynamicJsonBuffer  jsonBuffer(200);
  
    // JSON input string.
    //
    // It's better to use a char[] as shown here.
    // If you use a const char* or a String, ArduinoJson will
    // have to make a copy of the input in the JsonBuffer.
    //char json[] =
    //    "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
    // Root of the object tree.
    //
    // It's a reference to the JsonObject, the actual bytes are inside the
    // JsonBuffer with all the other nodes of the object tree.
    // Memory is freed when jsonBuffer goes out of scope.
    JsonObject& root = jsonBuffer.parseObject((char *)rgbRead);
  
    // Test if parsing succeeds.
    if (!root.success()) {
//      Serial.println("parseObject() failed");
      return;
    }
  
    weather      = root["weather"][0]["description"];
    temp         = root["main"]["temp"];
    temp_min     = root["main"]["temp_min"];
    temp_max     = root["main"]["temp_max"];
    pressure     = root["main"]["pressure"];
    humidity     = root["main"]["humidity"];
    wind_speed   = root["wind"]["speed"];
    wind_deg     = root["wind"]["deg"];
    city         = root["name"]; 
}

