#include "StorageManager.h"

// 全局实例
StorageManager StorageComm;

// 存储键名
const char* StorageManager::WIFI_SSID_KEY = "wifi_ssid";
const char* StorageManager::WIFI_PASSWORD_KEY = "wifi_password";
const char* StorageManager::IMAGE_URL_KEY = "image_url";
const char* StorageManager::BASE_HOST_KEY = "base_host";

// 构造函数
StorageManager::StorageManager() {
}

// 析构函数
StorageManager::~StorageManager() {
}

// 初始化存储
bool StorageManager::init() {
  Serial.println("正在初始化存储系统");

  // 开始使用Preferences
  if (!preferences.begin("device_settings", false)) {
    Serial.println("存储系统初始化失败");
    return false;
  }

  Serial.println("存储系统初始化成功");
  return true;
}

// 保存WiFi账号密码
bool StorageManager::saveWiFiCredentials(const String& ssid, const String& password) {
  Serial.printf("正在保存WiFi账号密码: SSID=%s, Password=%s\n", ssid.c_str(), password.c_str());

  preferences.putString(WIFI_SSID_KEY, ssid);
  preferences.putString(WIFI_PASSWORD_KEY, password);

  Serial.println("WiFi账号密码保存成功");
  return true;
}

// 加载WiFi账号密码
bool StorageManager::loadWiFiCredentials(String& ssid, String& password) {
  Serial.println("正在加载WiFi账号密码");

  ssid = preferences.getString(WIFI_SSID_KEY, "");
  password = preferences.getString(WIFI_PASSWORD_KEY, "");

  if (ssid.isEmpty() || password.isEmpty()) {
    Serial.println("未找到WiFi账号密码");
    return false;
  }

  Serial.printf("WiFi账号密码加载成功: SSID=%s, Password=%s\n", ssid.c_str(), password.c_str());
  return true;
}

// 清除WiFi账号密码
bool StorageManager::clearWiFiCredentials() {
  Serial.println("正在清除WiFi账号密码");

  preferences.remove(WIFI_SSID_KEY);
  preferences.remove(WIFI_PASSWORD_KEY);

  Serial.println("WiFi账号密码清除成功");
  return true;
}

// 检查是否有WiFi账号密码
bool StorageManager::hasWiFiCredentials() {
  String ssid = preferences.getString(WIFI_SSID_KEY, "");
  String password = preferences.getString(WIFI_PASSWORD_KEY, "");

  return !ssid.isEmpty() && !password.isEmpty();
}

// 保存图片地址
bool StorageManager::saveImageUrl(const String& imageUrl) {
  Serial.printf("正在保存图片地址: %s\n", imageUrl.c_str());

  preferences.putString(IMAGE_URL_KEY, imageUrl);

  Serial.println("图片地址保存成功");
  return true;
}

// 加载图片地址
bool StorageManager::loadImageUrl(String& imageUrl) {
  Serial.println("正在加载图片地址");

  imageUrl = preferences.getString(IMAGE_URL_KEY, "");

  if (imageUrl.isEmpty()) {
    Serial.println("未找到图片地址");
    return false;
  }

  Serial.printf("图片地址加载成功: %s\n", imageUrl.c_str());
  return true;
}

// 清除图片地址
bool StorageManager::clearImageUrl() {
  Serial.println("正在清除图片地址");

  preferences.remove(IMAGE_URL_KEY);

  Serial.println("图片地址清除成功");
  return true;
}

// 检查是否有图片地址
bool StorageManager::hasImageUrl() {
  String imageUrl = preferences.getString(IMAGE_URL_KEY, "");
  return !imageUrl.isEmpty();
}

// 保存BASE_HOST
bool StorageManager::saveBaseHost(const String& baseHost) {
  Serial.printf("正在保存BASE_HOST: %s\n", baseHost.c_str());

  preferences.putString(BASE_HOST_KEY, baseHost);

  Serial.println("BASE_HOST保存成功");
  return true;
}

// 加载BASE_HOST
bool StorageManager::loadBaseHost(String& baseHost) {
  Serial.println("正在加载BASE_HOST");

  baseHost = preferences.getString(BASE_HOST_KEY, "");

  if (baseHost.isEmpty()) {
    Serial.println("未找到BASE_HOST");
    return false;
  }

  Serial.printf("BASE_HOST加载成功: %s\n", baseHost.c_str());
  return true;
}

// 清除BASE_HOST
bool StorageManager::clearBaseHost() {
  Serial.println("正在清除BASE_HOST");

  preferences.remove(BASE_HOST_KEY);

  Serial.println("BASE_HOST清除成功");
  return true;
}

// 检查是否有BASE_HOST
bool StorageManager::hasBaseHost() {
  String baseHost = preferences.getString(BASE_HOST_KEY, "");
  return !baseHost.isEmpty();
}

// 获取所有存储信息（JSON格式）
String StorageManager::getStorageInfo() {
  Serial.println("正在获取存储信息");

  String ssid = preferences.getString(WIFI_SSID_KEY, "");
  String password = preferences.getString(WIFI_PASSWORD_KEY, "");
  String imageUrl = preferences.getString(IMAGE_URL_KEY, "");
  String baseHost = preferences.getString(BASE_HOST_KEY, "");

  // 构建JSON响应
  String json = "{";
  json += "\"wifi\": {\"ssid\": \"" + ssid + "\", \"password\": \"" + password + "\"},";
  json += "\"image_url\": \"" + imageUrl + "\",";
  json += "\"base_host\": \"" + baseHost + "\"";
  json += "}";

  Serial.printf("存储信息: %s\n", json.c_str());
  return json;
}

