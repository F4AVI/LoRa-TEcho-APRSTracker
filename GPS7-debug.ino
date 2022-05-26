//Libraries for GPS
#include <TinyGPS++.h>

//Libraries for Radio Module
#include <RadioLib.h>
//#include <SX126x.h>

#include "utilities.h"
#include "images.h"
#include "config.h"

#include <SPI.h>
#include <Wire.h>

//Libraries for E-paper Display
#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>  // 1.54" b/w
#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

TinyGPSPlus MyGPS;


int txPower = 0;
char LatStr[10] = "";
char LongStr[11] = "";
char Lat[9] = "";
char Long[10] = "";
char IBatt[8] = "";
char Courant[25] = "";
char HLat;
char MLong;
float current;
int voltage;
char UBatt[6] = "";
char Tension[25] = "";
int sat;
int heure_gps;
char Heure[3];
int minute_gps;
char Minute[3];
String CourantCharge;
String APRSmsg = "";
String LoRaheader = "";
String APRSheader = "";
String APRSheadertlm = "";
String APRStlm0 = "";
String APRStlm1 = "";
String APRStlm2 = "";
String APRStlm3 = "";
String APRStlm4 = "";
int N_tlm = 0;
int D_par = 0;
char TLM[4] = "";
char APRStlmpar1[5] = "";
char APRStlmpar2[5] = "";
char APRStlmpar3[5] = "";
char APRStlmpar6[2] = "";
int N_trame = 0;
bool charge = false;
bool disp = false;
bool Pwr_en = false;
bool Pwr_on = false;
bool prot_bat = false;


uint32_t        blinkMillis = 0;
uint32_t        last = 0;

SPIClass dispPort(NRF_SPIM2, ePaper_Miso, ePaper_Sclk, ePaper_Mosi);
SPIClass rfPort(NRF_SPIM3, RADIO_MISO_PIN, RADIO_SCLK_PIN, RADIO_MOSI_PIN);
SPISettings spiSettings; 
GxIO_Class io(dispPort, ePaper_Cs, ePaper_Dc, ePaper_Rst);
GxEPD_Class display(io, ePaper_Rst, ePaper_Busy);

SX1262 radio = nullptr;

void setupLora();
void setupDisplay();
bool setupGPS();
void loopGPS();
void boardInit();
void LilyGo_logo();
void Veille_logo();
void enableBacklight(bool en);

void setup()
{
    Serial.begin(115200);
    delay(200);
    boardInit();
    delay(200);
    LilyGo_logo();
    enableBacklight(true);

    delay(2000);
    enableBacklight(false);
    display.fillScreen(GxEPD_WHITE);
    display.setRotation(3);
    display.update();

}


void loop()
{
    if (millis() - blinkMillis > 2500) {
        blinkMillis = millis();
        disp = true;
    }

    if ((digitalRead(UserButton_Pin) == LOW )&&(Pwr_on == false))
       {
        // Alimentation périphériques
        digitalWrite(Power_On_Pin,HIGH);
        enableBacklight(true);
        display.fillScreen(GxEPD_WHITE);
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
        enableBacklight(false);        
        //digitalWrite(GreenLed_Pin,HIGH);
        //digitalWrite(RedLed_Pin,LOW);
        Pwr_on = true;
        SerialMon.println("PWR ON");
        while (digitalRead(UserButton_Pin) == LOW);
       }
    else if ((digitalRead(UserButton_Pin) == LOW) && (Pwr_on == true))
       {
        Veille_logo();
        // Arrêt alimentation périphériques 
        digitalWrite(Power_On_Pin,LOW);
        //digitalWrite(GreenLed_Pin,LOW);
        //digitalWrite(RedLed_Pin,HIGH);        
        Pwr_on = false;
        SerialMon.println("PWR OFF");
        while (digitalRead(UserButton_Pin) == LOW);
       }


 
    byte bytetab[] = {0x3C, 0xFF, 0x01};
    char bytechar[3];
    bytechar[0] = char(bytetab[0]);
    bytechar[1] = char(bytetab[1]);
    bytechar[2] = char(bytetab[2]);
    LoRaheader = String(bytechar); 
    APRSheader = LoRaheader + Call + String(">") + "APLT00-1" + String(",") + "WIDE1-1" + String(":");
    APRSheadertlm = LoRaheader + Call + String(">") + "APMI04" + String(",") + "WIDE1-1" + String(":");
            
    voltage = analogRead(Adc_Pin);
   
    float volt_tmp = float(voltage)/1000;
    volt_tmp = volt_tmp * 7.0125;           // coefficent à appliquer depuis lecture ADC ( Ubat /2 /3.3V /1.0625)
    
    sprintf (UBatt,"%.2f",volt_tmp);

    if (volt_tmp > 4.6)
      charge = true;
    else
      charge = false;

    if (volt_tmp < 3.05)
      prot_bat = true;
    else
      prot_bat = false;       
    
    sprintf (Tension,"Batterie : %.3f V",volt_tmp);
    

    // GPS Data

    while ((SerialGPS.available() > 0) && (disp == false))
        MyGPS.encode(SerialGPS.read());    

    if (disp)
     {
      if (MyGPS.location.isUpdated())
       {
        sat = MyGPS.satellites.value();
        heure_gps  = MyGPS.time.hour();
        sprintf(Heure,"%02d",heure_gps);
        minute_gps = MyGPS.time.minute();
        sprintf(Minute,"%02d",minute_gps);        
        display.fillScreen(GxEPD_WHITE);
       
        display.setCursor(0,15);
        display.print(Heure);
        display.print(":");
        display.print(Minute);
        display.setCursor(90,15);
        display.print("Sat : ");
        display.println(sat);
        //SerialMon.println(sat);
        
        float latitude_mdeg = MyGPS.location.lat();
        float longitude_mdeg = MyGPS.location.lng();

        sprintf(LatStr,"%.6f",latitude_mdeg);
        sprintf(LongStr,"%.6f",longitude_mdeg);
        
        // Conversion Latitude
        int lat_tmp1 = latitude_mdeg;
        float lat_tmp2 = abs(latitude_mdeg) - abs(lat_tmp1);
        float lat_tmp3 = lat_tmp2 *60;
        char lat_tmp4[6] = "";
        snprintf(lat_tmp4, sizeof(lat_tmp4),"%.2f",lat_tmp3);
        
        if (latitude_mdeg < 0)
        {
          HLat = 'S';
        }
        else
        {
          HLat = 'N';        
        }
        snprintf(Lat, sizeof(Lat), "%02d%05s%c", abs(lat_tmp1),lat_tmp4,HLat);

        if (Debug == 1)
         {
          SerialMon.print("latitude_mdeg : ");
          SerialMon.println(latitude_mdeg);
          SerialMon.print("lat_tmp1 : ");
          SerialMon.println(lat_tmp1);
          SerialMon.print("lat_tmp2 : ");
          SerialMon.println(lat_tmp2);             
          SerialMon.print("lat_tmp3 : ");
          SerialMon.println(lat_tmp3);
          SerialMon.print("lat_tmp4 : ");
          SerialMon.println(lat_tmp4);        
          SerialMon.print("Lat : ");
          SerialMon.println(Lat);
         }
        
        // Conversion Longitude
        int long_tmp1 = longitude_mdeg;
        float long_tmp2 = abs(longitude_mdeg) - abs(long_tmp1);
        float long_tmp3 = float(long_tmp2) * 60;
        char long_tmp4[6] = "";
        snprintf(long_tmp4, sizeof(long_tmp4),"%.2f",long_tmp3);        
        
        if (longitude_mdeg < 0)
        {
          MLong = 'W';
        }
        else
        {
          MLong = 'E';
        }
        snprintf(Long, sizeof(Long), "%03d%05s%c", abs(long_tmp1),long_tmp4,MLong);

        if (Debug == 1)
         {
          SerialMon.print("longitude_mdeg : ");
          SerialMon.println(longitude_mdeg);
          SerialMon.print("long_tmp1 : ");
          SerialMon.println(long_tmp1);
          SerialMon.print("long_tmp2 : ");
          SerialMon.println(long_tmp2);             
          SerialMon.print("long_tmp3 : ");
          SerialMon.println(long_tmp3);
          SerialMon.print("long_tmp4 : ");
          SerialMon.println(long_tmp4);        
          SerialMon.print("Long : ");
          SerialMon.println(Long);   
         }
                                  
        display.setCursor(0,40);
        display.print("Lat. : ");
        display.println(LatStr);

        display.print("Long. : ");
        display.print(LongStr);

        APRSmsg = APRSheader;

        APRSmsg += "!" + String(Lat) + "/" + String(Long) + BEACON_SYMBOL; 
        APRSmsg += "LoRa Test Tracker by F4AVI";
        int stlong = APRSmsg.length();
 
        APRStlm0 = APRSheadertlm;
        APRStlm0 += ":" + Call + "  :PARM.UBat,IBat,Sat,-,-,Batt.Charge,B2,B3,B4,B5,B6,B7,B8";
        int stlong0 = APRStlm0.length(); 
 
        APRStlm1 = APRSheadertlm;
        APRStlm1 += ":" + Call + "  :UNIT.V,mA,-,-,-,O/N,O/N,O/N,O/N,O/N,O/N,O/N,O/N";
        int stlong1 = APRStlm1.length();

        APRStlm2 = APRSheadertlm;
        APRStlm2 += ":" + Call + "  :EQNS.0,0.005,0,0,1,0,0,1,0,0,1,0,0,1,0";
        int stlong2 = APRStlm2.length();

        APRStlm3 = APRSheadertlm;
        APRStlm3 += ":" + Call + "  :BITS.11111111,LoRa Tracker Telemetry";
        int stlong3 = APRStlm3.length();

        APRStlm4 = APRSheadertlm;
        // Numero trame
        sprintf(TLM,"%03d",N_tlm);
        // Tension Batterie
        int tlmpar1 = int(voltage * 1.4025);  // 7.0125 / 5
                
        sprintf(APRStlmpar1,"%03d",tlmpar1);
        
        // Courant Batterie
        int tlmpar2 = int(current * 2);
        sprintf(APRStlmpar2,"%03d",tlmpar2);
        // Nombre de satellites visibles
        sprintf(APRStlmpar3,"%03d",sat);
        // Charge ou Decharge
        if (charge){
          APRStlmpar6[1] = '1';
        }
        else
        {
          APRStlmpar6[1] = '0';
        }
        APRStlm4 += "T#" + String(TLM) + "," + String(APRStlmpar1) + "," + String(APRStlmpar2) + "," + String(APRStlmpar3) + "," + "0" + "," + "0" + "," + String(APRStlmpar6[1]);
        int stlong4 = APRStlm4.length();  

        // Envoi données via LoRa selon la valeur du Timer.
        if (millis() - last > 60000){
 
           //Send LoRa packet to receiver

           if (D_par != N_tlm-10)
            {
             // transmission parametres = false
             if ( N_trame > 0 && N_trame < 5 )
             {
              N_trame = 5;
             }
            }
            else 
            {
             //transmission parametres = true
             if (N_trame == 4)
             {
              D_par = N_tlm;
             } 
            }

           switch (N_trame){
              case 0:
                  //APRS Data
                  radio.startTransmit(APRSmsg,stlong);
                  Serial.println(APRSmsg);
                  N_trame++;
                  break;
              case 1:
                  //APRS Telemetry
                  radio.startTransmit(APRStlm0,stlong0);
                  Serial.println(APRStlm0);
                  N_trame++; 
                  break;
              case 2:
                  //APRS Telemetry
                  radio.startTransmit(APRStlm1,stlong1);
                  Serial.println(APRStlm1);
                  N_trame++;
                  break;                                    
              case 3:
                  //APRS Telemetry
                  radio.startTransmit(APRStlm2,stlong2);
                  Serial.println(APRStlm2);
                  N_trame++;
                  break;
              case 4:
                  //APRS Telemetry
                  radio.startTransmit(APRStlm3,stlong3);
                  Serial.println(APRStlm3);
                  N_trame++;
                  break;
              case 5:
                  //APRS Telemetry
                  radio.startTransmit(APRStlm4,stlong4);
                  Serial.println(APRStlm4);
                  N_tlm++;
                  N_trame++;
                  break;                  
           }

           display.setCursor(12,170);
           display.setFont(&FreeMonoBold18pt7b);
           display.println("<< TX >>");
           
           //LoRa.endPacket();
           if (N_trame > 5){
            N_trame = 0;
           }
           if (N_tlm > 999){
            N_tlm = 0;
           }
           last = millis();
        }
       else
        {
         display.setCursor(0,192);
         display.print("TX dans : ");
         display.print(60-((millis() - last)/1000));
         display.println("s");
        } 

        display.setCursor(0,100);
        display.setFont(&FreeMonoBold18pt7b);
        display.println(Call);
        display.setFont(&FreeMonoBold9pt7b);    
    
        display.setCursor(0,120);
        display.println(Tension);

        display.setCursor(0,140);
        if (charge)
           display.println("Charge");
        else 
           display.println("Decharge");         

        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
       }
      else
       {
        SerialMon.println("No Fix");
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(0,100);
        display.setFont(&FreeMonoBold24pt7b);
        display.println("No Fix");
        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(0,192);
        display.println(Tension);
        display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
        last = millis();
       }
     disp = false;   
     }

    if (prot_bat)
     {
      display.setRotation(0);
      display.fillScreen(GxEPD_WHITE);
      display.drawExampleBitmap(Batterie_Vide, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
      display.setRotation(3);
      display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);      
  
      Pwr_en = false;
      digitalWrite(Power_Enable_Pin,LOW);
     }
}
//**************************************/

void setupDisplay()
{
    dispPort.begin();
    display.init(/*115200*/); // enable diagnostic output on Serial
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
}


bool setupGPS()
{
    SerialMon.println("[GPS] Initializing ... ");
    SerialGPS.setPins(GPS_RX_PIN, GPS_TX_PIN);
    SerialMon.println("Pins ... OK");
    SerialGPS.begin(GPS_BAUD_RATE);
    SerialMon.println("Serial Port2  ... OK");

    pinMode(Gps_pps_Pin, INPUT);
    pinMode(Gps_Wakeup_Pin, OUTPUT);
    digitalWrite(Gps_Wakeup_Pin, HIGH);

    delay(10);
    pinMode(Gps_Reset_Pin, OUTPUT);
    digitalWrite(Gps_Reset_Pin, HIGH); delay(10);
    digitalWrite(Gps_Reset_Pin, LOW); delay(10);
    digitalWrite(Gps_Reset_Pin, HIGH);

    SerialMon.println("GPS reset ..... OK");


    SerialMon.println("GPS Started");
    return true;
}

void setupLora()
{
  SerialMon.println("[LoRa] Initializing ... ");
  rfPort.begin();
  radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN , RADIO_BUSY_PIN, rfPort, spiSettings);
  delay(200);

  if (!radio.begin(433.775,125.0,SF,5,0x12,TX_OUTPUT_POWER))
    {
      SerialMon.println(".......... OK");
      delay(600);
    }
  else
    {
      SerialMon.println(".......... KO");
      delay(600);
    }
   
  SerialMon.println("LoRa Initializing OK!");    
}

void boardInit()
{
    uint8_t rlst = 0;

    SerialMon.begin(MONITOR_SPEED);
    SerialMon.println("Start\n");

    uint32_t reset_reason;
    sd_power_reset_reason_get(&reset_reason);
    SerialMon.print("sd_power_reset_reason_get:");
    SerialMon.println(reset_reason, HEX);

    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    Pwr_en = true;

    pinMode(Power_On_Pin, OUTPUT);
    digitalWrite(Power_On_Pin, HIGH);
    Pwr_on = true;

    pinMode(ePaper_Backlight, OUTPUT);

    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);

    pinMode(UserButton_Pin, INPUT_PULLUP);
    pinMode(Touch_Pin, INPUT_PULLUP);

    setupDisplay();
    setupGPS();
    setupLora();

    display.update();
    delay(500);


}

void enableBacklight(bool en)
{
    digitalWrite(ePaper_Backlight, en);
}

void LilyGo_logo(void)
{
    display.setRotation(2);
    display.fillScreen(GxEPD_WHITE);

    display.drawExampleBitmap(BitmapCallSign, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display.update();
}

void Veille_logo(void)
{
    display.setRotation(0);
    display.fillScreen(GxEPD_WHITE);
    display.drawExampleBitmap(T_Echo_OFF, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display.setRotation(3);
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
}
