#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class StorageManager {
public:
  // 构造函数和析构函数
  StorageManager();
  ~StorageManager();

  // 初始化存储
  bool init();

  // WiFi账号密码存储
  bool saveWiFiCredentials(const String& ssid, const String& password);
  bool loadWiFiCredentials(String& ssid, String& password);
  bool clearWiFiCredentials();
  bool hasWiFiCredentials();

  // 图片地址存储
  bool saveImageUrl(const String& imageUrl);
  bool loadImageUrl(String& imageUrl);
  bool clearImageUrl();
  bool hasImageUrl();

  // BASE_HOST存储
  bool saveBaseHost(const String& baseHost);
  bool loadBaseHost(String& baseHost);
  bool clearBaseHost();
  bool hasBaseHost();

  // 获取所有存储信息（JSON格式）
  String getStorageInfo();

private:
  Preferences preferences;

  // 存储键名
  static const char* WIFI_SSID_KEY;
  static const char* WIFI_PASSWORD_KEY;
  static const char* IMAGE_URL_KEY;
  static const char* BASE_HOST_KEY;
};

extern StorageManager StorageComm;

#endif // STORAGE_MANAGER_H

