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

static uint8_t rgb_temp = 0;


void setup()
{

    Serial.begin(115200);
    delay(5000);
    
    ble.init();

    adv_params.adv_int_min = 0x00A0;
    adv_params.adv_int_max = 0x01A0;
    adv_params.adv_type    = 0;
    adv_params.dir_addr_type = 0;
    memset(adv_params.dir_addr,0,6);
    adv_params.channel_map = 0x07;//channel 38
    adv_params.filter_policy = 0x00;
    
    ble.setAdvParams(&adv_params);

}

void loop()
{
    char c ;
    uint8_t adv_data[14]={0x0D,0x16,0xE4,0xFE,0,0,0,0,0,0,0,0,0,0};
    for(uint8_t i = 1;i<5;i++)
    {
        reboca_count_low++;
        if(reboca_count_low==0xFF)
        {
          reboca_count_low = 0;
          reboca_count_high++;
          if(reboca_count_high==0xFF)
          {
            reboca_count_high = 0;
          }
        }
        adv_data[6] = reboca_count_low;
        adv_data[7] = reboca_count_high;
        adv_data[8] = 3;
        adv_data[9] = 1;
        adv_data[10] = i;
        /*rgb_temp =rgb_temp +50;
        adv_data[11] = rgb_temp;
        if(rgb_temp == 250
        {
            rgb_temp = 0;
        }*/
        adv_data[12] = rgb_temp;
        leds.setColorRGB(0, adv_data[11], adv_data[12], adv_data[13]);   
        //memcpy(client_adv_temp,adv_data,sizeof(adv_data));
        ble.setAdvData(sizeof(adv_data), adv_data);
        ble.startAdvertising();
        Serial.println("BLE start advertising.");
        delay(10);
        ble.stopAdvertising();
        delay(190);        
        
    }
    rgb_temp = 255-rgb_temp;
    

 
}

