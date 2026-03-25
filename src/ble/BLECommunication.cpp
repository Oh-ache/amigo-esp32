
#include "BLECommunication.h"

// 全局实例
BLECommunication BLEComm;

// 构造函数
BLECommunication::BLECommunication() :
  pServer(nullptr),
  pService(nullptr),
  pCharacteristic(nullptr),
  deviceConnected(false),
  oldDeviceConnected(false),
  dataAvailable(false),
  pServerCallbacks(nullptr),
  pCharCallbacks(nullptr) {
}

// 析构函数
BLECommunication::~BLECommunication() {
  stop();
  if (pServerCallbacks) delete pServerCallbacks;
  if (pCharCallbacks) delete pCharCallbacks;
}

// 初始化BLE蓝牙
bool BLECommunication::init() {
  // 初始化BLE设备
  BLEDevice::init("ESP32-BLE");

  // 创建BLE服务器
  pServer = BLEDevice::createServer();

  if (!pServer) {
    return false;
  }

  // 创建服务器回调
  pServerCallbacks = new MyServerCallbacks(this);
  pServer->setCallbacks(pServerCallbacks);

  // 创建BLE服务
  pService = pServer->createService(SERVICE_UUID);

  if (!pService) {
    return false;
  }

  // 创建BLE特征
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  if (!pCharacteristic) {
    return false;
  }

  // 添加描述符
  pCharacteristic->addDescriptor(new BLE2902());

  // 创建特征回调
  pCharCallbacks = new MyCharacteristicCallbacks(this);
  pCharacteristic->setCallbacks(pCharCallbacks);

  return true;
}

// 开启蓝牙
void BLECommunication::start() {
  if (pService) {
    // 启动服务
    pService->start();

    // 开始广播
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE蓝牙已开启，等待连接...");
  }
}

// 关闭蓝牙
void BLECommunication::stop() {
  BLEDevice::deinit();
  Serial.println("BLE蓝牙已关闭");
}

// 设置蓝牙名称（必须在init()之前调用，或者重新初始化）
void BLECommunication::setDeviceName(const char* name) {
  // 如果还没有初始化，可以直接设置
  if (!pServer) {
    BLEDevice::init(name);
    Serial.printf("蓝牙名称已设置为: %s\n", name);
  } else {
    // 如果已经初始化，需要先停止再重新初始化
    Serial.println("蓝牙已初始化，需要先关闭再重新初始化才能更改名称");
  }
}

// 检查是否有数据可用
bool BLECommunication::hasData() {
  return dataAvailable;
}

// 读取接收到的数据
String BLECommunication::readData() {
  if (dataAvailable) {
    String data(receivedData.c_str());
    dataAvailable = false;
    receivedData.clear();
    return data;
  }
  return String();
}

// 发送数据
void BLECommunication::sendData(const String& data) {
  if (deviceConnected && pCharacteristic) {
    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();
    Serial.printf("已发送数据: %s\n", data.c_str());
  }
}

// 检查蓝牙是否已连接
bool BLECommunication::isConnected() {
  return deviceConnected;
}

// 服务器连接回调
void BLECommunication::MyServerCallbacks::onConnect(BLEServer* pServer) {
  bleComm->deviceConnected = true;
  Serial.println("设备已连接");
}

// 服务器断开连接回调
void BLECommunication::MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  bleComm->deviceConnected = false;
  Serial.println("设备已断开连接");

  // 重新开始广播
  BLEDevice::startAdvertising();
  Serial.println("等待新的连接...");
}

// 特征写入回调
void BLECommunication::MyCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  bleComm->receivedData = pCharacteristic->getValue();
  bleComm->dataAvailable = true;

  Serial.print("收到数据: ");
  Serial.println(bleComm->receivedData.c_str());
}

