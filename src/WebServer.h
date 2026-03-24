
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebServer {
public:
  // 构造函数和析构函数
  WebServer();
  ~WebServer();

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
  void handleRequest(WiFiClient& client);

  // 发送HTTP响应
  void sendResponse(WiFiClient& client, int code, const String& contentType, const String& content);

  // 发送404错误响应
  void sendNotFound(WiFiClient& client);
};

extern WebServer WebComm;

#endif // WEB_SERVER_H

