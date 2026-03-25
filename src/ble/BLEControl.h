#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include <Arduino.h>

// BLE 控制函数声明 - 避免直接包含BLE头文件

void bleControlInit();
void bleControlStart();
void bleControlStop();
bool bleControlIsInitialized();
bool bleControlIsConnected();
bool bleControlIsRunning();

#endif // BLE_CONTROL_H
