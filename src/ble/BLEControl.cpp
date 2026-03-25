#include "BLEControl.h"
#include "BLECommunication.h"

// 初始化状态标志
static bool bleInitialized = false;
static bool bleRunning = false;

// BLE 控制函数实现

void bleControlInit() {
  if (!bleInitialized) {
    if (BLEComm.init()) {
      bleInitialized = true;
      Serial.println("BLE初始化成功");
    } else {
      Serial.println("BLE初始化失败");
    }
  }
}

void bleControlStart() {
  // 确保已初始化
  if (!bleInitialized) {
    bleControlInit();
  }
  
  if (bleInitialized) {
    BLEComm.start();
    bleRunning = true;
  }
}

void bleControlStop() {
  BLEComm.stop();
  bleRunning = false;
  // 注意：不重置初始化状态，因为BLE库的deinit()可能有问题
}

bool bleControlIsInitialized() {
  return bleInitialized;
}

bool bleControlIsConnected() {
  if (!bleInitialized) {
    return false;
  }
  return BLEComm.isConnected();
}

bool bleControlIsRunning() {
  return bleRunning;
}