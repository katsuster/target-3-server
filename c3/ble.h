#ifndef C3_BLE_H__
#define C3_BLE_H__

#define LOCAL_NAME                "Target3"
// Nordic nRF
#define SERVICE_UUID              "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define RX_QUEUE_LEN    8
#define RX_CMD_MAX      128
#define ADV_RESTART_MS  1000

void initBLE(void);
void loopBLE(void);
void txBLE(const char *buf);
bool isBLEConnected(void);

#endif /* C3_BLE_H__ */
