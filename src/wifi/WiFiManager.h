
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "../storage/StorageManager.h"

class WiFiManager {
public:
  // 构造函数和析构函数
  WiFiManager();
  ~WiFiManager();

  // 连接WiFi网络
  bool connect(const char* ssid, const char* password);

  // 连接WiFi网络（使用字符串参数）
  bool connect(const String& wifiCredentials);

  // 断开WiFi连接
  void disconnect();

  // 检查WiFi是否已连接
  bool isConnected();

  // 获取当前WiFi连接的SSID
  String getSSID();

  // 获取当前WiFi连接的RSSI（信号强度）
  int getRSSI();

  // 获取当前WiFi连接的IP地址
  String getIPAddress();

  // 等待WiFi连接完成（可设置超时时间）
  bool waitForConnection(int timeout = 30000);

  // 从Flash存储加载WiFi账号密码并连接
  bool connectFromStorage();

private:
  String currentSSID;
  String currentPassword;
  bool isWiFiConnected;

  // 解析WiFi凭证字符串（格式：SSID,password）
  bool parseCredentials(const String& wifiCredentials, String& ssid, String& password);
};

extern WiFiManager WiFiComm;

#endif // WIFI_MANAGER_H

