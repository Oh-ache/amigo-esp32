
#ifndef MY_WEB_SERVER_H
#define MY_WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "../elnk/EPD_3in7g.h"
#include "../elnk/GUI_Paint.h"

class MyWebServer {
public:
  // 构造函数和析构函数
  MyWebServer();
  ~MyWebServer();

  // 初始化Web服务器
  bool init(int port = 80);

  // 启动Web服务器
  bool start();

  // 停止Web服务器
  void stop();

  // 检查服务器是否正在运行
  bool isRunning();

  // 获取服务器端口
  int getPort();

  // 重置服务器
  bool restart();

  // 处理请求（需要在loop中调用）
  void handleClient();

private:
  WiFiServer* server;
  WiFiClient client;
  int port;
  bool isServerRunning;

  // 获取设备唯一ID（MAC地址）
  String getDeviceId();

  // 处理HTTP请求
  void handleRequest(WiFiClient& client, const String& request);

  // 发送HTTP响应
  void sendResponse(WiFiClient& client, int code, const String& contentType, const String& content);

  // 发送404错误响应
  void sendNotFound(WiFiClient& client);

  // 处理图片显示请求
  void handleDisplayImage(WiFiClient& client, const String& request);

  // 处理心跳检测请求
  void handleHeartbeat(WiFiClient& client);

  // 处理WiFi信息请求
  void handleGetWiFiInfo(WiFiClient& client);

  // 处理资源信息请求
  void handleResourceInfo(WiFiClient& client);

  // 获取资源信息（JSON格式）
  String getResourceInfo();

  // 处理状态查询请求
  void handleStatus(WiFiClient& client);

  // 获取设备状态信息（JSON格式）
  String getStatusInfo();

  // 处理存储信息请求
  void handleGetStorageInfo(WiFiClient& client);

  // 从URL下载图片
  std::vector<uint8_t> downloadImage(const String& url);

  // 解码BMP图片
  std::vector<uint8_t> decodeBMP(const std::vector<uint8_t>& bmpData, int& width, int& height);

  // 在墨水屏上显示图片
  bool displayImageOnEPD(const std::vector<uint8_t>& pixelData, int width, int height);

  // 获取外网IP地址
  String getExternalIP();

  // 处理BLE蓝牙启动请求
  void handleBLEStart(WiFiClient& client);

  // 处理BLE蓝牙停止请求
  void handleBLEStop(WiFiClient& client);

  // 处理更新图片地址请求
  void handleUpdateImageUrl(WiFiClient& client, const String& request);

  // 处理刷新图片显示请求（根据存储的image_url）
  void handleRefreshImage(WiFiClient& client);
};

extern MyWebServer WebComm;

#endif // MY_WEB_SERVER_H

