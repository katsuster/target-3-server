#include <Arduino.h>
// Bluetooth LE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#if defined(CONFIG_NIMBLE_ENABLED)
#include <host/ble_hs.h>
#endif

#include "ble.h"
#include "cmd.h"

struct rx_cmd {
  char buf[RX_CMD_MAX];
};

static BLEServer *pServer = NULL;
static BLECharacteristic *pTxCharacteristic;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static unsigned long disconnectTime = 0;
static QueueHandle_t rxQueue;

#if defined(CONFIG_NIMBLE_ENABLED)
// Do not use BLECharacteristic::notify() on NimBLE. NimBLE sometimes reports
// BLE_GAP_EVENT_CONNECT with non-zero status (0x1a).
static volatile uint16_t txConnHandle = BLE_HS_CONN_HANDLE_NONE;
#endif

class ConnectCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class WriteCallbacks: public BLECharacteristicCallbacks {
#if defined(CONFIG_NIMBLE_ENABLED)
  void onWrite(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc) {
    // Answer to the client which sends a command
    txConnHandle = desc->conn_handle;
    onWrite(pCharacteristic);
  }
#endif

  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue().c_str();
    struct rx_cmd c;

    if (rxValue.length() == 0) {
      return;
    }

    // Do not handle commands in BLE task, notify() in callback may be lost
    memset(&c, 0, sizeof(c));
    strncpy(c.buf, rxValue.c_str(), sizeof(c.buf) - 1);
    if (xQueueSend(rxQueue, &c, 0) != pdTRUE) {
      //Serial1.printf("rx queue full\n");
    }
  }
};

#if defined(CONFIG_NIMBLE_ENABLED)
class SubscribeCallbacks: public BLECharacteristicCallbacks {
  void onSubscribe(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue) {
    // Client enables notification before sending commands
    if (subValue > 0) {
      txConnHandle = desc->conn_handle;
    }
  }
};
#endif

// Bluetooth LE initialize
void initBLE(void) {
  rxQueue = xQueueCreate(RX_QUEUE_LEN, sizeof(struct rx_cmd));

  // Create the BLE Device
  BLEDevice::init(LOCAL_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ConnectCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
#if defined(CONFIG_NIMBLE_ENABLED)
  // NimBLE adds the 2902 descriptor automatically
  pTxCharacteristic->setCallbacks(new SubscribeCallbacks());
#else
  pTxCharacteristic->addDescriptor(new BLE2902());
#endif

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new WriteCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}

void loopBLE(void) {
    struct rx_cmd c;

    // receiving
    while (xQueueReceive(rxQueue, &c, 0) == pdTRUE) {
        String cmd = String(c.buf);
        onReceive(cmd);
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        disconnectTime = millis();
        oldDeviceConnected = deviceConnected;
    }

    // restart advertising a while after disconnected, without blocking loop()
    if (!deviceConnected && disconnectTime != 0 &&
        millis() - disconnectTime > ADV_RESTART_MS) {
        pServer->startAdvertising();
        //Serial1.println("startAdvertising");
        disconnectTime = 0;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        disconnectTime = 0;
        oldDeviceConnected = deviceConnected;
    }
}

void txBLE(const char *buf) {
  pTxCharacteristic->setValue((uint8_t *)buf, strlen(buf));

#if defined(CONFIG_NIMBLE_ENABLED)
  uint16_t h = txConnHandle;
  struct ble_gap_conn_desc desc;

  if (h != BLE_HS_CONN_HANDLE_NONE && ble_gap_conn_find(h, &desc) == 0) {
    // ble_gatts_notify_custom() consumes om regardless of the result
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, strlen(buf));
    ble_gatts_notify_custom(h, pTxCharacteristic->getHandle(), om);
  }
#else
  pTxCharacteristic->notify();
#endif
  delay(10);
}

bool isBLEConnected(void) {
  return deviceConnected;
}
