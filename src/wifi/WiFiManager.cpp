
#include "WiFiManager.h"

// 全局实例
WiFiManager WiFiComm;

// 构造函数
WiFiManager::WiFiManager() :
  currentSSID(""),
  currentPassword(""),
  isWiFiConnected(false) {
}

// 析构函数
WiFiManager::~WiFiManager() {
  disconnect();
}

// 连接WiFi网络（使用字符串参数）
bool WiFiManager::connect(const String& wifiCredentials) {
  String ssid;
  String password;

  if (parseCredentials(wifiCredentials, ssid, password)) {
    return connect(ssid.c_str(), password.c_str());
  } else {
    Serial.println("WiFi凭证格式错误，应为：SSID,password");
    return false;
  }
}

// 连接WiFi网络
bool WiFiManager::connect(const char* ssid, const char* password) {
  Serial.printf("正在连接到WiFi网络: %s\n", ssid);

  // 如果已经连接到同一网络，返回成功
  if (isConnected() && currentSSID == String(ssid)) {
    Serial.println("已连接到同一WiFi网络");
    return true;
  }

  // 断开当前连接
  disconnect();

  // 保存新的SSID和密码
  currentSSID = ssid;
  currentPassword = password;

  // 开始连接
  WiFi.begin(ssid, password);

  // 等待连接完成
  if (waitForConnection()) {
    isWiFiConnected = true;
    Serial.printf("WiFi连接成功，IP地址: %s\n", getIPAddress().c_str());
    Serial.printf("信号强度: %ddBm\n", getRSSI());
    return true;
  } else {
    Serial.println("WiFi连接失败");
    isWiFiConnected = false;
    return false;
  }
}

// 断开WiFi连接
void WiFiManager::disconnect() {
  if (isWiFiConnected) {
    WiFi.disconnect();
    isWiFiConnected = false;
    Serial.println("已断开WiFi连接");
  }
}

// 检查WiFi是否已连接
bool WiFiManager::isConnected() {
  isWiFiConnected = WiFi.status() == WL_CONNECTED;
  return isWiFiConnected;
}

// 获取当前WiFi连接的SSID
String WiFiManager::getSSID() {
  if (isConnected()) {
    return WiFi.SSID();
  } else {
    return "";
  }
}

// 获取当前WiFi连接的RSSI（信号强度）
int WiFiManager::getRSSI() {
  if (isConnected()) {
    return WiFi.RSSI();
  } else {
    return 0;
  }
}

// 获取当前WiFi连接的IP地址
String WiFiManager::getIPAddress() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  } else {
    return "";
  }
}

// 等待WiFi连接完成（可设置超时时间）
bool WiFiManager::waitForConnection(int timeout) {
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - startTime >= timeout) {
      Serial.println();
      Serial.println("WiFi连接超时");
      return false;
    }
  }

  Serial.println();
  return true;
}

// 解析WiFi凭证字符串（格式：SSID,password）
bool WiFiManager::parseCredentials(const String& wifiCredentials, String& ssid, String& password) {
  int commaIndex = wifiCredentials.indexOf(',');

  if (commaIndex == -1) {
    return false;
  }

  ssid = wifiCredentials.substring(0, commaIndex);
  password = wifiCredentials.substring(commaIndex + 1);

  // 去除首尾的空格
  ssid.trim();
  password.trim();

  if (ssid.length() == 0) {
    return false;
  }

  return true;
}

