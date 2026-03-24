
#include "WebServer.h"

// 全局实例
WebServer WebComm;

// 构造函数
WebServer::WebServer() :
  server(nullptr),
  port(80),
  isServerRunning(false) {
}

// 析构函数
WebServer::~WebServer() {
  stop();
  if (server) {
    delete server;
    server = nullptr;
  }
}

// 初始化Web服务器
bool WebServer::init(int port) {
  this->port = port;

  // 创建服务器实例
  server = new WiFiServer(port);

  if (!server) {
    Serial.println("无法创建Web服务器实例");
    return false;
  }

  Serial.printf("Web服务器初始化成功，端口: %d\n", port);
  return true;
}

// 启动Web服务器
bool WebServer::start() {
  if (!server) {
    Serial.println("Web服务器未初始化");
    return false;
  }

  server->begin();
  isServerRunning = true;
  Serial.printf("Web服务器已启动，端口: %d\n", port);

  // 输出访问信息
  Serial.printf("访问地址: http://%s/device-id\n", WiFi.localIP().toString().c_str());
  return true;
}

// 停止Web服务器
void WebServer::stop() {
  if (server && isServerRunning) {
    server->stop();
    isServerRunning = false;
    Serial.println("Web服务器已停止");
  }
}

// 检查服务器是否正在运行
bool WebServer::isRunning() {
  return isServerRunning;
}

// 获取服务器端口
int WebServer::getPort() {
  return port;
}

// 重置服务器
bool WebServer::restart() {
  stop();
  delay(100); // 等待服务器完全停止
  return start();
}

// 获取设备唯一ID（MAC地址）
String WebServer::getDeviceId() {
  // 获取WiFi的MAC地址作为设备唯一ID
  uint8_t macAddress[6];
  esp_read_mac(macAddress, ESP_MAC_WIFI_STA);

  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          macAddress[0], macAddress[1], macAddress[2],
          macAddress[3], macAddress[4], macAddress[5]);

  return String(macStr);
}

// 处理HTTP请求
void WebServer::handleRequest(WiFiClient& client) {
  // 读取请求
  String request = client.readStringUntil('\r');
  // 忽略其余的请求内容
  while (client.available()) {
    client.read();
  }

  // 解析请求路径
  int firstSpace = request.indexOf(' ');
  int secondSpace = request.indexOf(' ', firstSpace + 1);
  if (firstSpace != -1 && secondSpace != -1) {
    String path = request.substring(firstSpace + 1, secondSpace);

    // 处理根路径
    if (path == "/") {
      String html = "<html><body>";
      html += "<h1>ESP32 S3 Web服务</h1>";
      html += "<p>访问 /device-id 以获取设备唯一ID</p>";
      html += "</body></html>";
      sendResponse(client, 200, "text/html", html);
    }
    // 处理设备ID路径
    else if (path == "/device-id") {
      String deviceId = getDeviceId();
      String json = "{\"device_id\": \"" + deviceId + "\"}";
      sendResponse(client, 200, "application/json", json);
    }
    // 处理未找到的路径
    else {
      sendNotFound(client);
    }
  }
}

// 发送HTTP响应
void WebServer::sendResponse(WiFiClient& client, int code, const String& contentType, const String& content) {
  String response = "HTTP/1.1 ";
  response += code;

  switch (code) {
    case 200:
      response += " OK";
      break;
    case 404:
      response += " Not Found";
      break;
    default:
      response += " Unknown";
  }

  response += "\r\n";
  response += "Content-Type: " + contentType + "\r\n";
  response += "Content-Length: " + String(content.length()) + "\r\n";
  response += "Connection: close\r\n";
  response += "\r\n";
  response += content;

  client.print(response);
}

// 发送404错误响应
void WebServer::sendNotFound(WiFiClient& client) {
  String html = "<html><body><h1>404 Not Found</h1></body></html>";
  sendResponse(client, 404, "text/html", html);
}

// 处理客户端请求
void WebServer::handleClient() {
  if (!isServerRunning || !server) {
    return;
  }

  // 检查是否有新的客户端连接
  client = server->available();
  if (client) {
    // 处理客户端请求
    handleRequest(client);

    // 确保响应已发送
    client.flush();
    // 延迟关闭客户端，以便客户端有足够的时间接收响应
    delay(1);
    // 关闭连接
    client.stop();
  }
}

