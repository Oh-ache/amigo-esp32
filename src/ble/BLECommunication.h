
#ifndef BLE_COMMUNICATION_H
#define BLE_COMMUNICATION_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE服务和特征UUID
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class BLECommunication {
public:
  // 构造函数和析构函数
  BLECommunication();
  ~BLECommunication();

  // 初始化BLE蓝牙
  bool init();

  // 开启蓝牙
  void start();

  // 关闭蓝牙
  void stop();

  // 设置蓝牙名称
  void setDeviceName(const char* name);

  // 检查是否有数据可用
  bool hasData();

  // 读取接收到的数据
  String readData();

  // 发送数据
  void sendData(const String& data);

  // 检查蓝牙是否已连接
  bool isConnected();

private:
  BLEServer* pServer;
  BLEService* pService;
  BLECharacteristic* pCharacteristic;
  bool deviceConnected;
  bool oldDeviceConnected;
  std::string receivedData;
  bool dataAvailable;

  // BLE服务器回调类
  class MyServerCallbacks: public BLEServerCallbacks {
    BLECommunication* bleComm;
  public:
    MyServerCallbacks(BLECommunication* instance) : bleComm(instance) {}
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
  };

  // BLE特征回调类
  class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    BLECommunication* bleComm;
  public:
    MyCharacteristicCallbacks(BLECommunication* instance) : bleComm(instance) {}
    void onWrite(BLECharacteristic* pCharacteristic);
  };

  MyServerCallbacks* pServerCallbacks;
  MyCharacteristicCallbacks* pCharCallbacks;
};

extern BLECommunication BLEComm;

#endif // BLE_COMMUNICATION_H

