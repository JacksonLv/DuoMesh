/******************************************************
 *                      Macros Define
 ******************************************************/
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

/******************************************************
 *               Variable Definitions
 ******************************************************/

 // Modified the following for your AP/Router.
#define AP "AP-02_2.4G"
#define PIN "0098019777"

#define MAX_CLIENT_NUM   3
// Server Port
TCPServer server = TCPServer(8888);
TCPClient client[MAX_CLIENT_NUM];

// Connect handle.
static uint16_t connected_id = 0xFFFF;
Device_t device;

uint8_t  chars_index=0;
uint8_t  desc_index=0;

// The service uuid to be discovered.
static uint16_t service1_uuid =0xFEE4;
static uint16_t mesh_vlue_uuid[16] ={0x2A,0x1E,0x00,0x05,0xFD,0x51,0xD8,0x82,0x8B,0xA8,0xB9,0x8C,0x00,0x00,0xCD,0x1E};

static uint8_t gatt_notify_flag=0;

static uint8_t ble_connected = 0;

static uint8_t led_state = 0;

static uint8_t data[5]= {0,0,0,1,0};


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

    Serial.println("reportCallback: ");
    Serial.print("The advEventType: ");
    Serial.println(report->advEventType, HEX);
    Serial.print("The peerAddrType: ");
    Serial.println(report->peerAddrType, HEX);
    Serial.print("The peerAddr: ");
    for(index=0; index<6; index++)
    {
        Serial.print(report->peerAddr[index], HEX);
        Serial.print(" ");
    }
    Serial.println(" ");

    Serial.print("The rssi: ");
    Serial.println(report->rssi, DEC);

    Serial.print("The ADV data: ");
    for(index=0; index<report->advDataLen; index++)
    {
        Serial.print(report->advData[index], HEX);
        Serial.print(" ");
    }
    Serial.println(" ");
    Serial.println(" ");

    uint8_t len;
    uint8_t adv_name[31];
    // Find short local name.
    if(0x00 == ble_advdata_decode(0x09, report->advDataLen, report->advData, &len, adv_name))
    {
        Serial.print("  The length of Short Local Name : ");
        Serial.println(len);
        Serial.print("  The Short Local Name is        : ");
        Serial.println((const char *)adv_name);
        if(0x00 == memcmp("nano_mesh",adv_name, 8))
        {
            ble.stopScanning();
            device.addr_type = report->peerAddrType;
            memcpy(device.addr, report->peerAddr, 6);

            ble.connect(report->peerAddr, BD_ADDR_TYPE_LE_RANDOM);
            
        }
    }
}

/**
 * @brief Connect handle.
 *
 * @param[in]  status   BLE_STATUS_CONNECTION_ERROR or BLE_STATUS_OK.
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void deviceConnectedCallback(BLEStatus_t status, uint16_t handle) {
    switch (status){
        case BLE_STATUS_OK:
            Serial.println("Device connected!");
            //connected_id = handle;
            device.connected_handle = handle;
            ble.discoverPrimaryServices(handle);
            break;
        default:
            break;
    }
}

/**
 * @brief Disconnect handle.
 *
 * @param[in]  handle   Connect handle.
 *
 * @retval None
 */
void deviceDisconnectedCallback(uint16_t handle){
    Serial.print("Disconnected handle:");
    Serial.println(handle,HEX);
    if(connected_id == handle)
    {
        Serial.println("Restart scanning.");
        connected_id = 0xFFFF;
        ble.startScanning();
    }
    ble_connected = 0;
}

/**
 * @brief Callback for handling result of discovering service.
 *
 * @param[in]  status      BLE_STATUS_OK/BLE_STATUS_DONE
 * @param[in]  con_handle  
 * @param[in]  *service    Discoverable service.
 *
 * @retval None
 */
static void discoveredServiceCallback(BLEStatus_t status, uint16_t con_handle, gatt_client_service_t *service)
{
    if(status == BLE_STATUS_OK)
    {
        Serial.println(" ");
        Serial.print("Service start handle: ");
        Serial.println(service->start_group_handle, HEX);
        Serial.print("Service end handle: ");
        Serial.println(service->end_group_handle, HEX);
        Serial.print("Service uuid16: ");
        Serial.println(service->uuid16, HEX);
        uint8_t index;
        Serial.print("The uuid128 : ");
        for(index=0; index<16; index++)
        {
            Serial.print(service->uuid128[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
        if(service->uuid16==service1_uuid)
        {
            Serial.println("Target uuid16");
            device.service.service = *service;
        }
    }
    else if(status == BLE_STATUS_DONE)
    {
        Serial.println("Discovered service done");
        // All sevice have been found, start to discover characteristics.
        // Result will be reported on discoveredCharsCallback.
        ble.discoverCharacteristics(device.connected_handle, &device.service.service);
    }
}

/**
 * @brief Callback for handling result of discovering characteristic.
 *
 * @param[in]  status           BLE_STATUS_OK/BLE_STATUS_DONE
 * @param[in]  con_handle  
 * @param[in]  *characteristic  Discoverable characteristic.
 *
 * @retval None
 */
static void discoveredCharsCallback(BLEStatus_t status, uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
    if(status == BLE_STATUS_OK)
    {
      // Found a characteristic.
        Serial.println(" ");
        Serial.print("characteristic start handle: ");
        Serial.println(characteristic->start_handle, HEX);
        Serial.print("characteristic value handle: ");
        Serial.println(characteristic->value_handle, HEX);
        Serial.print("characteristic end_handle: ");
        Serial.println(characteristic->end_handle, HEX);
        Serial.print("characteristic properties: ");
        Serial.println(characteristic->properties, HEX);
        Serial.print("characteristic uuid16: ");
        Serial.println(characteristic->uuid16, HEX);
        uint8_t index;
        Serial.print("characteristic uuid128 : ");
        for(index=0; index<16; index++)
        {
            Serial.print(characteristic->uuid128[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
        if(chars_index < 2)
        { 
            device.service.chars[chars_index].chars= *characteristic;
            chars_index++;
        }
    }
    else if(status == BLE_STATUS_DONE)
    {
        Serial.println("Discovered characteristic done");
        chars_index = 0;
        // All characteristics have been found, start to discover descriptors.
        // Result will be reported on discoveredCharsDescriptorsCallback.
        ble.discoverCharacteristicDescriptors(device.connected_handle, &device.service.chars[chars_index].chars);
    }
}


/**
 * @brief Callback for handling result of discovering descriptor.
 *
 * @param[in]  status         BLE_STATUS_OK/BLE_STATUS_DONE
 * @param[in]  con_handle  
 * @param[in]  *descriptor    Discoverable descriptor.
 *
 * @retval None
 */
static void discoveredCharsDescriptorsCallback(BLEStatus_t status, uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor)
{
    if(status == BLE_STATUS_OK)
    {
      // Found a descriptor.
        Serial.println(" ");
        Serial.print("descriptor handle: ");
        Serial.println(descriptor->handle, HEX);
        Serial.print("descriptor uuid16: ");
        Serial.println(descriptor->uuid16, HEX);
        uint8_t index;
        Serial.print("descriptor uuid128 : ");
        for(index=0; index<16; index++)
        {
            Serial.print(descriptor->uuid128[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
        if(desc_index < 2)
        {
            device.service.chars[chars_index].descriptor[desc_index++] = *descriptor;
        }
    }
    else if(status == BLE_STATUS_DONE)
    {
        // finish.
        Serial.println("Discovered descriptor done");
        ble_connected = 1;
        chars_index++;
        if(chars_index < 2)
        {
            desc_index=0;
            ble.discoverCharacteristicDescriptors(device.connected_handle, &device.service.chars[chars_index].chars);
        }
        else
        {
            // Read value of characteristic, 
            // Result will be reported on gattReadCallback.
            ble.readValue(device.connected_handle,&device.service.chars[0].chars);
        }
    }
}

/**
 * @brief Callback for handling result of reading.
 *
 * @param[in]  status         BLE_STATUS_OK/BLE_STATUS_DONE/BLE_STATUS_OTHER_ERROR
 * @param[in]  con_handle  
 * @param[in]  value_handle   
 * @param[in]  *value
 * @param[in]  length
 *
 * @retval None
 */
void gattReadCallback(BLEStatus_t status, uint16_t con_handle, uint16_t value_handle, uint8_t *value, uint16_t length)
{
    if(status == BLE_STATUS_OK)
    {
        Serial.println(" ");
        Serial.println("Read characteristic ok");
        Serial.print("conn handle: ");
        Serial.println(con_handle, HEX);
        Serial.print("value handle: ");
        Serial.println(value_handle, HEX);
        uint8_t index;
        Serial.print("The value : ");
        for(index=0; index<length; index++)
        {
            Serial.print(value[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
    }
    else if(status == BLE_STATUS_DONE)
    {
        // Result will be reported on gattWrittenCallback.
        // If use ble.writeValueWithoutResponse, will no response.
        //uint8_t data[]= {0x01,0x02,0x03,0x04,0x05,1,2,3,4,5};
        //ble.writeValue(device.connected_handle, device.service.chars[1].chars.value_handle, sizeof(data), data);
    }
}


/**
 * @brief Callback for handling result of writting.
 *
 * @param[in]  status         BLE_STATUS_DONE/BLE_STATUS_OTHER_ERROR
 * @param[in]  con_handle  
 *
 * @retval None
 */
void gattWrittenCallback(BLEStatus_t status, uint16_t con_handle)
{
    if(BLE_STATUS_DONE)
    {
        Serial.println(" ");
        Serial.println("Write characteristic done");
        ble.readDescriptorValue(device.connected_handle, device.service.chars[0].descriptor[0].handle);
    }
}


/**
 * @brief Callback for handling result of reading descriptor.
 *
 * @param[in]  status         BLE_STATUS_DONE/BLE_STATUS_OTHER_ERROR
 * @param[in]  con_handle  
 * @param[in]  value_handle   
 * @param[in]  *value
 * @param[in]  length
 *
 * @retval None
 */
void gattReadDescriptorCallback(BLEStatus_t status, uint16_t con_handle, uint16_t value_handle, uint8_t *value, uint16_t length)
{
    if(status == BLE_STATUS_OK)
    {
        Serial.println(" ");
        Serial.println("Read descriptor ok");
        Serial.print("conn handle: ");
        Serial.println(con_handle, HEX);
        Serial.print("value handle: ");
        Serial.println(value_handle, HEX);
        uint8_t index;
        Serial.print("The value : ");
        for(index=0; index<length; index++)
        {
            Serial.print(value[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
    }
    else if(status == BLE_STATUS_DONE)
    {
        // Enable notify.
        ble.writeClientCharsConfigDescritpor(device.connected_handle, &device.service.chars[0].chars, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
    }
}


/**
 * @brief Callback for handling result of writting client characteristic configuration.
 *
 * @param[in]  status         BLE_STATUS_DONE/BLE_STATUS_OTHER_ERROR
 * @param[in]  con_handle
 *
 * @retval None
 */
void gattWriteCCCDCallback(BLEStatus_t status, uint16_t con_handle)
{
    if(status == BLE_STATUS_DONE)
    {
        Serial.println("gattWriteCCCDCallback done");
        if(gatt_notify_flag == 0)
        { 
            gatt_notify_flag = 1;
            ble.writeClientCharsConfigDescritpor(device.connected_handle, &device.service.chars[1].chars, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
        }
        else if(gatt_notify_flag == 1)
        {
            gatt_notify_flag = 2;
        }
    }
}

/**
 * @brief Callback for handling notify event from remote device.
 *
 * @param[in]  status         BLE_STATUS_OK
 * @param[in]  con_handle  
 * @param[in]  value_handle   
 * @param[in]  *value
 * @param[in]  length 
 *
 * @retval None
 */
void gattNotifyUpdateCallback(BLEStatus_t status, uint16_t con_handle, uint16_t value_handle, uint8_t *value, uint16_t length)
{
    Serial.println(" ");
    Serial.println("Notify Update value ");
    Serial.print("conn handle: ");
    Serial.println(con_handle, HEX);
    Serial.print("value handle: ");
    Serial.println(value_handle, HEX);
    uint8_t index;
    Serial.print("The value : ");
    for(index=0; index<length; index++)
    {
        Serial.print(value[index], HEX);
        Serial.print(" ");
    }
    Serial.println(" ");
}

/**
 * @brief ble_setting.
 */
void ble_setting()
{
      //ble.debugLogger(true);
    //ble.debugError(true);
    //ble.enablePacketLogger();
    Serial.println("BLE central demo!");
    // Initialize ble_stack.
    ble.init();

    // Register callback functions.
    ble.onConnectedCallback(deviceConnectedCallback);
    ble.onDisconnectedCallback(deviceDisconnectedCallback);
    ble.onScanReportCallback(reportCallback);

    ble.onServiceDiscoveredCallback(discoveredServiceCallback);
    ble.onCharacteristicDiscoveredCallback(discoveredCharsCallback);
    ble.onDescriptorDiscoveredCallback(discoveredCharsDescriptorsCallback);
    ble.onGattCharacteristicReadCallback(gattReadCallback);
    ble.onGattCharacteristicWrittenCallback(gattWrittenCallback);
    ble.onGattDescriptorReadCallback(gattReadDescriptorCallback);

    ble.onGattWriteClientCharacteristicConfigCallback(gattWriteCCCDCallback);
    ble.onGattNotifyUpdateCallback(gattNotifyUpdateCallback);

    // Set scan parameters.
    ble.setScanParams(0, 0x0030, 0x0030);
    // Start scanning.
    ble.startScanning();
    Serial.println("Start scanning ");
}
/**
 * @brief Setup.
 */
void setup()
{
    char addr[16];
    Serial.begin(115200);
    delay(5000);
    
    ble_setting();
    //WiFi.clearCredentials();
    WiFi.on();
    WiFi.setCredentials(AP, PIN, WPA2);
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
}

/**
 * @brief Loop.
 */
void loop()
{
    
    for(uint8_t i = 0;i<MAX_CLIENT_NUM;i++)
    {
        if (client[i].connected())
        {

            //client[i].println("Hello!");
            if (client[i].available())              // if there's bytes to read from the client,
          {
              Serial.print("client read: ");
              data[4] = client[i].read() ;
              Serial.println(data[4]);
              if(ble_connected == 1)
              {
                 //ble.readValue(device.connected_handle,&device.service.chars[1].chars);
                 //led_state = !led_state;
                 //data[4] = led_state;
                 Serial.print("ble write ");
                 ble.writeValue(device.connected_handle, device.service.chars[1].chars.value_handle, sizeof(data), data);
              }
          }
        }
        else
        {
            client[i] = server.available();
        }
    }
    delay(1000);

}


