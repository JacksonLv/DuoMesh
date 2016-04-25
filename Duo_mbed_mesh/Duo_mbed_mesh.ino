#include "ChainableLED.h"

#if defined(ARDUINO) 
SYSTEM_MODE(MANUAL);//do not connect to cloud
#else
SYSTEM_MODE(AUTOMATIC);//connect to cloud
#endif

/******************************************************
 *                      Type Define
 ******************************************************/
typedef struct {
    uint16_t  connected_handle;
    uint8_t   addr_type;
    bd_addr_t addr;
    struct {
        gatt_client_service_t service;
        struct{
            gatt_client_characteristic_t chars;
            gatt_client_characteristic_descriptor_t descriptor[2];
        }chars[2];
    }service;
}Device_t;

// Modified the following for your AP/Router.
//#define AP "AP-02_2.4G"
//#define PIN "0098019777"

static advParams_t adv_params;
//static uint8_t adv_data[31]={0x0A,0x16,0xE4,0xFE,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static uint8_t rx_temp[100]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static uint8_t rx_len;
static uint8_t reboca_count_low = 0;
static uint8_t reboca_count_high = 0;

#define MAX_CLIENT_NUM   3
// Server Port
TCPServer server = TCPServer(8888);
TCPClient client[MAX_CLIENT_NUM];
Device_t device;
#define NUM_LEDS  1
ChainableLED leds(D4, D5, NUM_LEDS);

static uint8_t PID = 0;
static uint8_t GID = 0;

static uint8_t advdata_temp[31]={0};
static uint8_t client_adv_temp[31]={0};

static uint8_t adv_update_flag = 0;


static btstack_timer_source_t scan_timer;
static btstack_timer_source_t client_timer;

/******************************************************
 *               Function Definitions
 ******************************************************/
 /**
 * @brief Find the data given the type in advertising data.
 *
 * @param[in]  type          The type of field data.
 * @param[in]  advdata_len   Length of advertising data.
 * @param[in]  *p_advdata    The pointer of advertising data.
 * @param[out] *len          The length of found data.
 * @param[out] *p_field_data The pointer of buffer to store field data.
 *
 * @retval 0 Find the data
 *         1 Not find.
 */

uint32_t ble_advdata_decode(uint8_t type, uint8_t advdata_len, uint8_t *p_advdata, uint8_t *len, uint8_t *p_field_data)
{
    uint8_t index=0;
    uint8_t field_length, field_type;

    while(index<advdata_len)
    {
        field_length = p_advdata[index];
        field_type   = p_advdata[index+1];
        if(field_type == type)
        {
            memcpy(p_field_data, &p_advdata[index+2], (field_length-1));
            *len = field_length - 1;
            return 0;
        }
        index += field_length + 1;
    }
    return 1;
}

/**
 * @brief Callback for scanning device.
 *
 * @param[in]  *report
 *
 * @retval None
 */

void reportCallback(advertisementReport_t *report)
{
    uint8_t index;
//    Serial.println("BLE scan ");
//    Serial.print("The ADV data: ");
//    for(index=0; index<report->advDataLen; index++)
//        {
//            Serial.print(report->advData[index], HEX);
//            Serial.print(" ");
//        }
//        
//        Serial.println(" ");
    
    if((report->advData[2] ==  0x0D)&&(report->advData[3] == 0x16)&&(report->advData[4] == 0xE4)&&(report->advData[5] == 0xFE))
    {
        if(memcmp(report->advData,client_adv_temp,sizeof(report->advData)) != 0)
        {
            if((client_adv_temp[10]==report->advData[2])&&(client_adv_temp[11]==report->advData[3])&&(client_adv_temp[12]==report->advData[4])&&(client_adv_temp[13]==report->advData[5])&&(client_adv_temp[14]==report->advData[6])&&(client_adv_temp[15]==report->advData[7]))
            {
              Serial.println("same data");
              return;
            }
            memcpy(advdata_temp,report->advData,report->advDataLen);
            
            adv_update_flag = 1;
        }
    }
    else if((report->advData[2] ==  0x07)&&(report->advData[3] == 0x16)&&(report->advData[4] == 0xE4)&&(report->advData[5] == 0xFE))
    {
        ble.stopScanning();
      
        /*reboca_count_low++;
        if(reboca_count_low==0xFF)
        {
          reboca_count_low = 0;
          reboca_count_high++;
          if(reboca_count_high==0xFF)
          {
            reboca_count_high = 0;
          }
        }*/
        ble.setAdvData(sizeof(client_adv_temp), client_adv_temp);
        ble.startAdvertising();
//        ble.startAdvertising(); 
//        ble.startAdvertising(); 
        Serial.println("BLE start advertising.");
        ble.stopAdvertising();
        ble.startScanning();
    }
//    else 
//    {
//          uint8_t adv_data[8]={0x07,0x16,0xE4,0xFE,0,0,0,0};
//          
//          
//          adv_data[4]= reboca_count_low;
//          adv_data[5]= reboca_count_high;
//          reboca_count_low++;//= reboca_count_low+11;
//          if(reboca_count_low==0xFF)
//          {
//            reboca_count_low = 0;
//            reboca_count_high++;
//            if(reboca_count_high==0xFF)
//            {
//              reboca_count_high = 0;
//            }
//          }
//          ble.setAdvData(sizeof(adv_data), adv_data);
//          ble.startAdvertising();
//      
//          Serial.println("BLE start advertising.");
//          ble.stopAdvertising();
//          
//    }

}


static void scan_timer_intrp(btstack_timer_source_t *ts)
{  
        
    if(adv_update_flag == 1)
    {
        ble.stopScanning();
        
        if(advdata_temp[10]== 0x03)
        {
            memcpy(client_adv_temp,advdata_temp,sizeof(advdata_temp));
            switch(advdata_temp[11])
            {
              case 1:
              {
                if(PID == advdata_temp[12])
                {
                    leds.setColorRGB(0, advdata_temp[13], advdata_temp[14], advdata_temp[15]);                 // write LED data
                }
              }
              break;
              case 2:
              {
                if(GID == advdata_temp[12])
                {
                    leds.setColorRGB(0, advdata_temp[13], advdata_temp[14], advdata_temp[15]);                 // write LED data
                }
              }
              break;
              case 0xFF:
              {
                    leds.setColorRGB(0, advdata_temp[13], advdata_temp[14], advdata_temp[15]);                 // write LED data
              }
              break;
            }
        }
        /*reboca_count_low++;
        if(reboca_count_low==0xFF)
        {
          reboca_count_low = 0;
          reboca_count_high++;
          if(reboca_count_high==0xFF)
          {
            reboca_count_high = 0;
          }
        }*/
        ble.setAdvData(sizeof(client_adv_temp), client_adv_temp);
        ble.startAdvertising();   
//        ble.startAdvertising(); 
//        ble.startAdvertising();      
        ble.stopAdvertising();
        adv_update_flag = 0;
        ble.startScanning();
    }
    

    // reset
    ble.setTimer(ts, 48);
    ble.addTimer(ts);
}


static void client_timer_intrp(btstack_timer_source_t *ts)
{
         
    // reset
    ble.setTimer(ts, 58);
    ble.addTimer(ts);
}
void setup()
{

    char addr[16];

    Serial.begin(115200);
    delay(5000);
    
    ble.init();
    ble.onScanReportCallback(reportCallback);


    adv_params.adv_int_min = 0x00A0;
    adv_params.adv_int_max = 0x01A0;
    adv_params.adv_type    = 0;
    adv_params.dir_addr_type = 0;
    memset(adv_params.dir_addr,0,6);
    adv_params.channel_map = 0x07;//channel 38
    adv_params.filter_policy = 0x00;
    
    ble.setAdvParams(&adv_params);
    // Set scan parameters.
    ble.setScanParams(0, 4, 0x0030);//interval[0x0004,0x4000],unit:0.625ms
    ble.startScanning();
    Serial.println("Start scanning ");
    
    WiFi.on();
    //WiFi.setCredentials(AP, PIN, WPA2);
    WiFi.connect();
  
    IPAddress localIP = WiFi.localIP();
    
    while (localIP[0] == 0)
    {
        localIP = WiFi.localIP();
        Serial.println("waiting for an IP address");
        delay(1000);
    }
  
    sprintf(addr, "%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);
  
    Serial.println(addr);

    server.begin();  

    scan_timer.process = &scan_timer_intrp;
    ble.setTimer(&scan_timer, 48);//100ms
    ble.addTimer(&scan_timer);

//    client_timer.process = &client_timer_intrp;
//    ble.setTimer(&client_timer, 58);//100ms
//    ble.addTimer(&client_timer);

    PID = EEPROM.read(0);
    Serial.print("PID");
    Serial.println(PID);
    GID = EEPROM.read(1);
    Serial.print("GID");
    Serial.println(GID);
    
    
    uint8_t adv_data[10]={0x0F,0xFF,0x07,0x16,0xE4,0xFE,0,0,0,0};
    ble.setAdvData(sizeof(adv_data), adv_data);
    ble.startAdvertising();

    Serial.println("BLE start advertising.");
    ble.stopAdvertising();
}

void loop()
{
char c ;
    uint8_t adv_data[16]={0x0f,0xFF,0x0D,0x16,0xE4,0xFE,0,0,0,0,0,0,0,0,0,0};
    for(uint8_t client_num = 0;client_num<MAX_CLIENT_NUM;client_num++)
    {
        if (client[client_num].connected())
        {
            if (client[client_num].available()) 
            {
              ble.stopScanning();
              uint8_t i = 0;
              memset(rx_temp,0,sizeof(rx_temp));              
              
              rx_len =client[client_num].available();
              while(client[client_num].available())
              {
                c = client[client_num].read();             // read a byte, then
                rx_temp[i] = c; 
                adv_data[10+i] = rx_temp[i];
                Serial.print(rx_temp[i],HEX);              
                i++;
              }
              Serial.println("");
              if((client_adv_temp[10]==rx_temp[0])&&(client_adv_temp[11]==rx_temp[1])&&(client_adv_temp[12]==rx_temp[2])&&(client_adv_temp[13]==rx_temp[3])&&(client_adv_temp[14]==rx_temp[4])&&(client_adv_temp[15]==rx_temp[5]))
              {
                Serial.println("same data");
                return;
              }
              delay(100);
              for(uint8_t h = 0;h<sizeof(adv_data);h++)
              {
                  Serial.print(adv_data[h],HEX);  
              }   
              Serial.println("");
              
              /*reboca_count_low++;
              if(reboca_count_low==0xFF)
              {
                reboca_count_low = 0;
                reboca_count_high++;
                if(reboca_count_high==0xFF)
                {
                  reboca_count_high = 0;
                }
              }*/
              switch(rx_temp[0])
              {
                case 1:
                {
                    Serial.print("set PID  ");
                    
                    EEPROM.write(0, rx_temp[1]); 
                    PID = EEPROM.read(0);
                    Serial.println(PID);
                }
                break;
                case 2:
                {
                      Serial.print("set GID  ");
                      
                      EEPROM.write(1, rx_temp[1]); 
                      GID = EEPROM.read(1);
                      Serial.println(GID);
                }
                break;
                case 3:
                {
                    memset(client_adv_temp,0,sizeof(client_adv_temp));
                    Serial.println("set RGB");                    
                    /*adv_data[6] = reboca_count_low;
                    adv_data[7] = reboca_count_high;*/
//                    for(uint8_t h=0;h<rx_len;rx_len++)
//                    {
//                        adv_data[8+h] = rx_temp[i];
//                    }
                    switch(rx_temp[1])
                    {
                      case 1:
                      {
                        if(PID == rx_temp[2])
                        {
                            leds.setColorRGB(0, rx_temp[3], rx_temp[4], rx_temp[5]);                 // write LED data
                        }
                      }
                      break;
                      case 2:
                      {
                        if(GID == rx_temp[3])
                        {
                            leds.setColorRGB(0, rx_temp[3], rx_temp[4], rx_temp[5]);                   // write LED data
                        }
                      }
                      break;
                      case 0xFF:
                      {
                            leds.setColorRGB(0, rx_temp[3], rx_temp[4], rx_temp[5]);                  // write LED data
                      }
                      break;
                    }
                    memcpy(client_adv_temp,adv_data,sizeof(adv_data));
                    ble.setAdvData(sizeof(adv_data), adv_data);
                    ble.startAdvertising();
//                    ble.startAdvertising(); 
//                    ble.startAdvertising(); 
                    Serial.println("BLE start advertising.");
                    ble.stopAdvertising();
                }
                break;
                default: break;
              }
                           

              
            }

            ble.startScanning();
            
        }
        else
        {
            client[client_num] = server.available();
            
        }
    }
    delay(100);
 
}

