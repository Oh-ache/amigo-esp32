
#include "WebServer.h"
#include "../wifi/WiFiManager.h"
#include "../ble/BLEControl.h"
#include "../storage/StorageManager.h"

// 全局实例
MyWebServer WebComm;

// 构造函数
MyWebServer::MyWebServer() :
  server(nullptr),
  port(80),
  isServerRunning(false) {
}

// 析构函数
MyWebServer::~MyWebServer() {
  stop();
  if (server) {
    delete server;
    server = nullptr;
  }
}

// 初始化Web服务器
bool MyWebServer::init(int port) {
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
bool MyWebServer::start() {
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
void MyWebServer::stop() {
  if (server && isServerRunning) {
    server->stop();
    isServerRunning = false;
    Serial.println("Web服务器已停止");
  }
}

// 检查服务器是否正在运行
bool MyWebServer::isRunning() {
  return isServerRunning;
}

// 获取服务器端口
int MyWebServer::getPort() {
  return port;
}

// 重置服务器
bool MyWebServer::restart() {
  stop();
  delay(100); // 等待服务器完全停止
  return start();
}

// 获取设备唯一ID（MAC地址）
String MyWebServer::getDeviceId() {
  // 获取WiFi的MAC地址作为设备唯一ID
  uint8_t macAddress[6];
  esp_read_mac(macAddress, ESP_MAC_WIFI_STA);

  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          macAddress[0], macAddress[1], macAddress[2],
          macAddress[3], macAddress[4], macAddress[5]);

  return String(macStr);
}

// 从URL下载图片并返回图片数据
std::vector<uint8_t> MyWebServer::downloadImage(const String& url) {
  std::vector<uint8_t> imageData;

  Serial.printf("正在下载图片: %s\n", url.c_str());

  WiFiClient client;

  // 解析URL
  int protocolIndex = url.indexOf("://");
  if (protocolIndex == -1) {
    Serial.println("无效的URL格式");
    return imageData;
  }

  String protocol = url.substring(0, protocolIndex);
  String hostAndPath = url.substring(protocolIndex + 3);

  int pathIndex = hostAndPath.indexOf("/");
  if (pathIndex == -1) {
    Serial.println("无效的URL格式");
    return imageData;
  }

  String host = hostAndPath.substring(0, pathIndex);
  String path = hostAndPath.substring(pathIndex);

  Serial.printf("主机: %s, 路径: %s\n", host.c_str(), path.c_str());

  // 连接到服务器
  if (!client.connect(host.c_str(), 80)) {
    Serial.println("无法连接到服务器");
    return imageData;
  }

  // 发送HTTP请求
  client.print(String("GET ") + path + " HTTP/1.1\r\n");
  client.print(String("Host: ") + host + "\r\n");
  client.print("User-Agent: ESP32\r\n");
  client.print("Connection: close\r\n\r\n");

  // 等待响应
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println("连接超时");
      client.stop();
      return imageData;
    }
  }

  // 读取响应头
  bool headerEnd = false;
  bool contentLengthFound = false;
  int contentLength = 0;

  while (client.available()) {
    String line = client.readStringUntil('\n');

    if (line.startsWith("Content-Length: ")) {
      contentLength = line.substring(16).toInt();
      contentLengthFound = true;
    }

    if (line == "\r") {
      headerEnd = true;
      break;
    }
  }

  if (!headerEnd || !contentLengthFound) {
    Serial.println("无法解析响应头");
    client.stop();
    return imageData;
  }

  Serial.printf("图片大小: %d字节\n", contentLength);

  // 读取图片数据
  imageData.reserve(contentLength);

  timeout = millis();
  while (imageData.size() < contentLength) {
    if (client.available()) {
      imageData.push_back(client.read());
    } else if (millis() - timeout > 10000) {
      Serial.println("下载超时");
      client.stop();
      return imageData;
    }
  }

  Serial.println("图片下载成功");
  client.stop();
  return imageData;
}

// 解码BMP图片
std::vector<uint8_t> MyWebServer::decodeBMP(const std::vector<uint8_t>& bmpData, int& width, int& height) {
  std::vector<uint8_t> pixelData;

  if (bmpData.size() < 54) {
    Serial.println("无效的BMP文件");
    return pixelData;
  }

  // 检查BMP文件头
  if (bmpData[0] != 'B' || bmpData[1] != 'M') {
    Serial.println("不是有效的BMP文件");
    return pixelData;
  }

  // 获取图片尺寸
  width = *((uint32_t*)&bmpData[18]);
  height = *((uint32_t*)&bmpData[22]);

  Serial.printf("图片尺寸: %d x %d\n", width, height);

  // 获取像素数据偏移量
  uint32_t pixelOffset = *((uint32_t*)&bmpData[10]);

  // 获取位深度
  uint16_t bitDepth = *((uint16_t*)&bmpData[28]);

  if (bitDepth != 24) {
    Serial.printf("不支持的位深度: %d\n", bitDepth);
    return pixelData;
  }

  // 计算每行字节数
  int bytesPerLine = (width * 3 + 3) & ~3;

  // 分配内存
  pixelData.reserve(width * height);

  // 读取像素数据
  for (int y = height - 1; y >= 0; y--) {
    int rowOffset = pixelOffset + y * bytesPerLine;

    for (int x = 0; x < width; x++) {
      int pixelIndex = rowOffset + x * 3;

      if (pixelIndex + 2 >= bmpData.size()) {
        Serial.println("像素数据不足");
        return pixelData;
      }

      // BMP格式是BGR，我们需要转换为灰度
      uint8_t blue = bmpData[pixelIndex];
      uint8_t green = bmpData[pixelIndex + 1];
      uint8_t red = bmpData[pixelIndex + 2];

      uint8_t gray = (red * 0.299 + green * 0.587 + blue * 0.114);
      pixelData.push_back(gray);
    }
  }

  return pixelData;
}

// 在墨水屏上显示图片
bool MyWebServer::displayImageOnEPD(const std::vector<uint8_t>& pixelData, int width, int height) {
  Serial.println("准备显示图片");

  // 初始化硬件模块
  DEV_Module_Init();

  // 初始化墨水屏
  EPD_3IN7G_Init();
  EPD_3IN7G_Clear(EPD_3IN7G_WHITE);

  // 创建图像缓冲区
  std::vector<uint8_t> imageBuffer;
  int bufferSize = ((EPD_3IN7G_WIDTH % 4 == 0) ? (EPD_3IN7G_WIDTH / 4) : (EPD_3IN7G_WIDTH / 4 + 1)) * EPD_3IN7G_HEIGHT;
  imageBuffer.resize(bufferSize, EPD_3IN7G_WHITE);

  // 设置绘图区域
  Paint_NewImage(imageBuffer.data(), EPD_3IN7G_WIDTH, EPD_3IN7G_HEIGHT, 0, EPD_3IN7G_WHITE);
  Paint_Clear(EPD_3IN7G_WHITE);

  // 重要：设置缩放比例为4，因为EPD_3IN7G是4灰度级显示
  Paint_SetScale(4);

  // 检查图片尺寸是否匹配屏幕
  if (width != EPD_3IN7G_WIDTH || height != EPD_3IN7G_HEIGHT) {
    Serial.println("图片尺寸不匹配，需要调整");
    // 简单的缩放逻辑（可以根据需要优化）
    int newWidth = EPD_3IN7G_WIDTH;
    int newHeight = EPD_3IN7G_HEIGHT;

    std::vector<uint8_t> resizedData;
    resizedData.resize(newWidth * newHeight);

    double xRatio = (double)width / newWidth;
    double yRatio = (double)height / newHeight;

    for (int y = 0; y < newHeight; y++) {
      for (int x = 0; x < newWidth; x++) {
        int srcX = (int)(x * xRatio);
        int srcY = (int)(y * yRatio);

        srcX = constrain(srcX, 0, width - 1);
        srcY = constrain(srcY, 0, height - 1);

        resizedData[y * newWidth + x] = pixelData[srcY * width + srcX];
      }
    }

    // 将灰度数据转换为墨水屏格式
    for (int y = 0; y < newHeight; y++) {
      for (int x = 0; x < newWidth; x++) {
        int index = y * newWidth + x;
        UBYTE gray = resizedData[index];

        // 简单的灰度转换为二值图像
        UBYTE color = (gray < 128) ? EPD_3IN7G_BLACK : EPD_3IN7G_WHITE;
        Paint_SetPixel(x, y, color);
      }
    }
  } else {
    // 图片尺寸与屏幕匹配
    // 将灰度数据转换为墨水屏格式
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        int index = y * width + x;
        UBYTE gray = pixelData[index];

        // 简单的灰度转换为二值图像
        UBYTE color = (gray < 128) ? EPD_3IN7G_BLACK : EPD_3IN7G_WHITE;
        Paint_SetPixel(x, y, color);
      }
    }
  }

  // 显示图片
  EPD_3IN7G_Display(Paint.Image);

  // 进入睡眠模式
  EPD_3IN7G_Sleep();

  Serial.println("图片显示完成");
  return true;
}

// 处理图片显示请求
void MyWebServer::handleDisplayImage(WiFiClient& client, const String& request) {
  Serial.println("收到图片显示请求");

  // 解析请求体
  int bodyIndex = request.indexOf("\r\n\r\n");
  if (bodyIndex == -1) {
    sendResponse(client, 400, "application/json", "{\"success\": false, \"message\": \"无效的请求格式\"}");
    return;
  }

  String body = request.substring(bodyIndex + 4);
  Serial.printf("请求体长度: %d\n", body.length());

  // 解析JSON数据
  if (body.startsWith("{") && body.endsWith("}")) {
    // 简单的JSON解析，寻找"url"字段
    int urlStart = body.indexOf("\"url\"");
    if (urlStart != -1) {
      urlStart = body.indexOf(":", urlStart) + 1;

      // 跳过空格
      while (urlStart < body.length() && (body[urlStart] == ' ' || body[urlStart] == '\t')) {
        urlStart++;
      }

      if (body[urlStart] == '"') {
        urlStart++;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd != -1) {
          String imageUrl = body.substring(urlStart, urlEnd);
          Serial.printf("图片URL: %s\n", imageUrl.c_str());

          // 下载图片
          std::vector<uint8_t> bmpData = downloadImage(imageUrl);
          if (bmpData.empty()) {
            sendResponse(client, 500, "application/json", "{\"success\": false, \"message\": \"无法下载图片\"}");
            return;
          }

          // 解码图片
          int width, height;
          std::vector<uint8_t> pixelData = decodeBMP(bmpData, width, height);
          if (pixelData.empty()) {
            sendResponse(client, 500, "application/json", "{\"success\": false, \"message\": \"无法解码图片\"}");
            return;
          }

          // 保存图片地址到Flash存储
          StorageComm.saveImageUrl(imageUrl);

          // 显示图片
          if (displayImageOnEPD(pixelData, width, height)) {
            sendResponse(client, 200, "application/json", "{\"success\": true, \"message\": \"图片显示成功\"}");
          } else {
            sendResponse(client, 500, "application/json", "{\"success\": false, \"message\": \"无法显示图片\"}");
          }

          return;
        }
      }
    }
  }

  sendResponse(client, 400, "application/json", "{\"success\": false, \"message\": \"无法解析请求\"}");
}

// 发送HTTP响应
void MyWebServer::sendResponse(WiFiClient& client, int code, const String& contentType, const String& content) {
  String response = "HTTP/1.1 ";
  response += code;

  switch (code) {
    case 200:
      response += " OK";
      break;
    case 404:
      response += " Not Found";
      break;
    case 400:
      response += " Bad Request";
      break;
    case 500:
      response += " Internal Server Error";
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
void MyWebServer::sendNotFound(WiFiClient& client) {
  String html = "<html><body><h1>404 Not Found</h1></body></html>";
  sendResponse(client, 404, "text/html", html);
}

// 处理心跳检测请求
void MyWebServer::handleHeartbeat(WiFiClient& client) {
  Serial.println("收到心跳检测请求");
  sendResponse(client, 200, "text/plain", "ok");
}

// 获取外网IP地址
String MyWebServer::getExternalIP() {
  Serial.println("正在获取外网IP");

  WiFiClient client;

  // 使用 ipify.org 服务获取外网IP（免费且无需API密钥）
  if (!client.connect("api.ipify.org", 80)) {
    Serial.println("无法连接到IP查询服务器");
    return "";
  }

  // 发送HTTP请求
  client.print(String("GET /?format=text HTTP/1.1\r\n"));
  client.print(String("Host: api.ipify.org\r\n"));
  client.print(String("User-Agent: ESP32\r\n"));
  client.print(String("Connection: close\r\n\r\n"));

  // 等待响应
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println("获取外网IP超时");
      client.stop();
      return "";
    }
  }

  // 读取响应
  String response = "";
  while (client.available()) {
    response += client.readStringUntil('\n');
  }

  client.stop();

  // 解析响应，获取IP地址
  int bodyIndex = response.indexOf("\r\n\r\n");
  if (bodyIndex != -1) {
    String ip = response.substring(bodyIndex + 4);
    ip.trim(); // 去除可能的空白字符
    Serial.printf("外网IP: %s\n", ip.c_str());
    return ip;
  }

  Serial.println("无法解析IP响应");
  return "";
}

// 处理WiFi信息请求
void MyWebServer::handleGetWiFiInfo(WiFiClient& client) {
  Serial.println("收到WiFi信息请求");

  // 获取内网IP
  String localIP = WiFi.localIP().toString();

  // 获取外网IP
  String externalIP = getExternalIP();

  // 获取信号强度
  int rssi = WiFi.RSSI();

  // 获取MAC地址
  String macAddress = WiFi.macAddress();

  // 构建JSON响应
  String json = "{\"local_ip\": \"" + localIP + "\", \"external_ip\": \"" + externalIP + "\", \"rssi\": " + String(rssi) + ", \"mac_address\": \"" + macAddress + "\"}";

  sendResponse(client, 200, "application/json", json);
}

// 获取设备资源信息（JSON格式）
String MyWebServer::getResourceInfo() {
  Serial.println("正在获取设备资源信息");

  // 1. 内存信息
  size_t freeHeap = ESP.getFreeHeap();
  size_t totalHeap = ESP.getHeapSize();
  size_t usedHeap = totalHeap - freeHeap;
  float heapUsage = (float)usedHeap / totalHeap * 100;

  // 2. PSRAM信息（如果有PSRAM）
  size_t freePsram = 0;
  size_t totalPsram = 0;
  size_t usedPsram = 0;
  float psramUsage = 0;
  #ifdef CONFIG_SPIRAM_SUPPORT
    freePsram = ESP.getFreePsram();
    totalPsram = ESP.getPsramSize();
    usedPsram = totalPsram - freePsram;
    if (totalPsram > 0) {
      psramUsage = (float)usedPsram / totalPsram * 100;
    }
  #endif

  // 3. 闪存信息
  size_t flashSize = ESP.getFlashChipSize();
  size_t flashSpeed = ESP.getFlashChipSpeed();

  // 4. CPU信息
  uint32_t cpuFreq = ESP.getCpuFreqMHz();

  // 构建JSON响应
  String json = "{";
  json += "\"memory\": {";
  json += "\"heap\": {";
  json += "\"total\": " + String(totalHeap) + ", ";
  json += "\"used\": " + String(usedHeap) + ", ";
  json += "\"free\": " + String(freeHeap) + ", ";
  json += "\"usage\": " + String(heapUsage, 2);
  json += "}, ";
  json += "\"psram\": {";
  json += "\"total\": " + String(totalPsram) + ", ";
  json += "\"used\": " + String(usedPsram) + ", ";
  json += "\"free\": " + String(freePsram) + ", ";
  json += "\"usage\": " + String(psramUsage, 2);
  json += "}";
  json += "}, ";
  json += "\"flash\": {";
  json += "\"size\": " + String(flashSize) + ", ";
  json += "\"speed\": " + String(flashSpeed);
  json += "}, ";
  json += "\"cpu\": {";
  json += "\"frequency\": " + String(cpuFreq);
  json += "}";
  json += "}";

  Serial.printf("资源信息: %s\n", json.c_str());
  return json;
}

// 处理资源信息请求
void MyWebServer::handleResourceInfo(WiFiClient& client) {
  Serial.println("收到资源信息请求");

  String resourceInfo = getResourceInfo();
  sendResponse(client, 200, "application/json", resourceInfo);
}

// 获取设备状态信息（JSON格式）
String MyWebServer::getStatusInfo() {
  Serial.println("正在获取设备状态信息");

  // 1. BLE蓝牙状态 - 暂时注释掉，因为无法包含BLE头文件
  // bool bleConnected = BLEComm.isConnected();

  // 2. WiFi状态
  bool wifiConnected = WiFiComm.isConnected();

  // 3. Web服务器状态
  bool webRunning = isRunning();

  // 4. 墨水屏状态（检查busy pin）
  bool epdBusy = false;

  // 检查墨水屏是否在工作（通过busy引脚判断）
  // 注意：需要先初始化GPIO引脚，我们可以在第一次调用时初始化
  static bool pinInitialized = false;
  if (!pinInitialized) {
    pinMode(EPD_BUSY_PIN, INPUT);
    pinInitialized = true;
  }

  // 读取busy引脚状态（HIGH表示繁忙）
  epdBusy = digitalRead(EPD_BUSY_PIN) == HIGH;

  // 5. 运行时间（秒）
  unsigned long uptime = millis() / 1000;

  // 6. 芯片温度（摄氏度）
  float chipTemperature = temperatureRead();

  // 7. 重启次数（通过ESP32的重启原因计数）
  // 使用RTC_DATA_ATTR变量保存重启次数，因为这个变量会在重启后保留
  static RTC_DATA_ATTR int restartCount = 0;
  // 每次重启都会增加重启次数
  static bool firstRun = true;
  if (firstRun) {
    restartCount++;
    firstRun = false;
  }

  // 构建JSON响应
  String json = "{";
  json += "\"ble\": {\"connected\": " + String(bleControlIsConnected() ? "true" : "false") + ", \"initialized\": " + String(bleControlIsInitialized() ? "true" : "false") + ", \"running\": " + String(bleControlIsRunning() ? "true" : "false") + "},";
  json += "\"wifi\": {\"connected\": " + String(wifiConnected ? "true" : "false") + "},";
  json += "\"web_server\": {\"running\": " + String(webRunning ? "true" : "false") + "},";
  json += "\"e_paper\": {\"busy\": " + String(epdBusy ? "true" : "false") + "},";
  json += "\"uptime\": " + String(uptime) + ",";
  json += "\"chip_temperature\": " + String(chipTemperature, 2) + ",";
  json += "\"restart_count\": " + String(restartCount);
  json += "}";

  Serial.printf("设备状态信息: %s\n", json.c_str());
  return json;
}

// 处理存储信息请求
void MyWebServer::handleGetStorageInfo(WiFiClient& client) {
  Serial.println("收到存储信息请求");

  String storageInfo = StorageComm.getStorageInfo();
  sendResponse(client, 200, "application/json", storageInfo);
}

// 处理状态查询请求
void MyWebServer::handleStatus(WiFiClient& client) {
  Serial.println("收到状态查询请求");

  String statusInfo = getStatusInfo();
  sendResponse(client, 200, "application/json", statusInfo);
}

// 处理客户端请求
void MyWebServer::handleClient() {
  if (!isServerRunning || !server) {
    return;
  }

  // 检查是否有新的客户端连接
  WiFiClient newClient = server->available();
  if (newClient) {
    Serial.println("有新的客户端连接");

    // 读取完整请求
    String request = "";
    unsigned long start = millis();

    // 首先读取请求头，直到找到请求结束标记
    while (millis() - start < 5000) {
      if (newClient.available()) {
        char c = newClient.read();
        request += c;

        // 检查是否到达请求头结束
        if (request.indexOf("\r\n\r\n") != -1) {
          break;
        }
      }
    }

    // 检查是否有Content-Length头部，如果有，继续读取请求体
    int contentLength = 0;
    int clIndex = request.indexOf("Content-Length: ");
    if (clIndex != -1) {
      clIndex += 16; // 跳过"Content-Length: "
      int clEnd = request.indexOf("\r\n", clIndex);
      if (clEnd != -1) {
        contentLength = request.substring(clIndex, clEnd).toInt();
        Serial.printf("Content-Length: %d\n", contentLength);
      }
    }

    // 计算已读取的请求体长度
    int bodyStart = request.indexOf("\r\n\r\n");
    int bodyRead = 0;
    if (bodyStart != -1) {
      bodyRead = request.length() - (bodyStart + 4);
    }

    // 如果需要读取更多请求体数据
    if (contentLength > bodyRead) {
      int remaining = contentLength - bodyRead;
      start = millis();
      while (remaining > 0 && millis() - start < 5000) {
        if (newClient.available()) {
          char c = newClient.read();
          request += c;
          remaining--;
        }
      }
    }

    Serial.print("收到请求: ");
    Serial.println(request.substring(0, 100));

    // 处理客户端请求
    handleRequest(newClient, request);

    // 确保响应已发送
    newClient.flush();
    // 延迟关闭客户端，以便客户端有足够的时间接收响应
    delay(1);
    // 关闭连接
    newClient.stop();
  }
}

// 处理HTTP请求
void MyWebServer::handleRequest(WiFiClient& client, const String& request) {
  // 解析请求路径和HTTP方法
  int firstSpace = request.indexOf(' ');
  int secondSpace = request.indexOf(' ', firstSpace + 1);

  if (firstSpace != -1 && secondSpace != -1) {
    String method = request.substring(0, firstSpace);
    String path = request.substring(firstSpace + 1, secondSpace);

    Serial.printf("方法: %s, 路径: %s\n", method.c_str(), path.c_str());

    // 处理根路径
    if (path == "/") {
      String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>";
      html += "<h1>ESP32 S3 Web服务</h1>";
      html += "<h2>可用接口</h2>";
      html += "<ul>";
      html += "<li><strong>GET</strong> / - 显示此页面</li>";
      html += "<li><strong>GET</strong> /device-id - 获取设备唯一ID</li>";
      html += "<li><strong>POST</strong> /display-image - 显示图片（需发送JSON: {\"url\": \"图片URL\"}）</li>";
      html += "<li><strong>POST</strong> /update-image-url - 更新图片地址（需发送JSON: {\"image_url\": \"图片URL\"}）</li>";
      html += "<li><strong>GET</strong> /heartbeat - 心跳检测</li>";
      html += "<li><strong>GET</strong> /wifi - 获取WiFi信息</li>";
      html += "<li><strong>GET</strong> /resource - 获取设备资源信息</li>";
      html += "<li><strong>GET</strong> /status - 获取设备状态信息</li>";
      html += "<li><strong>GET</strong> /storage - 获取存储信息</li>";
      html += "<li><strong>GET</strong> /ble/start - 启动BLE蓝牙</li>";
      html += "<li><strong>GET</strong> /ble/stop - 停止BLE蓝牙</li>";
      html += "</ul>";
      html += "</body></html>";
      sendResponse(client, 200, "text/html; charset=utf-8", html);
    }
    // 处理设备ID路径
    else if (path == "/device-id") {
      String deviceId = getDeviceId();
      String json = "{\"device_id\": \"" + deviceId + "\"}";
      sendResponse(client, 200, "application/json", json);
    }
    // 处理图片显示路径（POST方法）
    else if (path == "/display-image" && method == "POST") {
      handleDisplayImage(client, request);
    }
    // 处理更新图片地址路径（POST方法）
    else if (path == "/update-image-url" && method == "POST") {
      handleUpdateImageUrl(client, request);
    }
    // 处理心跳检测路径（GET方法）
    else if (path == "/heartbeat" && method == "GET") {
      handleHeartbeat(client);
    }
    // 处理WiFi信息路径（GET方法）
    else if (path == "/wifi" && method == "GET") {
      handleGetWiFiInfo(client);
    }
    // 处理资源信息路径（GET方法）
    else if (path == "/resource" && method == "GET") {
      handleResourceInfo(client);
    }
    // 处理状态查询路径（GET方法）
    else if (path == "/status" && method == "GET") {
      handleStatus(client);
    }
    // 处理存储信息路径（GET方法）
    else if (path == "/storage" && method == "GET") {
      handleGetStorageInfo(client);
    }
    // 处理BLE启动路径（GET方法）
    else if (path == "/ble/start" && method == "GET") {
      handleBLEStart(client);
    }
    // 处理BLE停止路径（GET方法）
    else if (path == "/ble/stop" && method == "GET") {
      handleBLEStop(client);
    }
    // 处理未找到的路径
    else {
      Serial.printf("未找到路径: %s\n", path.c_str());
      sendNotFound(client);
    }
  } else {
    // 如果无法解析请求，发送400错误
    sendResponse(client, 400, "application/json", "{\"success\": false, \"message\": \"无效的请求格式\"}");
  }
}

// 处理BLE蓝牙启动请求
void MyWebServer::handleBLEStart(WiFiClient& client) {
  Serial.println("收到BLE蓝牙启动请求");
  
  // 启动BLE蓝牙
  bleControlStart();
  
  // 返回成功响应
  String json = "{\"success\": true, \"message\": \"BLE蓝牙已启动\"}";
  sendResponse(client, 200, "application/json", json);
}

// 处理更新图片地址请求
void MyWebServer::handleUpdateImageUrl(WiFiClient& client, const String& request) {
  Serial.println("收到更新图片地址请求");

  // 解析请求体
  int bodyIndex = request.indexOf("\r\n\r\n");
  if (bodyIndex == -1) {
    sendResponse(client, 400, "application/json", "{\"success\": false, \"message\": \"无效的请求格式\"}");
    return;
  }

  String body = request.substring(bodyIndex + 4);
  Serial.printf("请求体长度: %d\n", body.length());

  // 解析JSON数据
  if (body.startsWith("{") && body.endsWith("}")) {
    // 简单的JSON解析，寻找"image_url"字段
    int urlStart = body.indexOf("\"image_url\"");
    if (urlStart != -1) {
      urlStart = body.indexOf(":", urlStart) + 1;

      // 跳过空格
      while (urlStart < body.length() && (body[urlStart] == ' ' || body[urlStart] == '\t')) {
        urlStart++;
      }

      if (body[urlStart] == '"') {
        urlStart++;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd != -1) {
          String imageUrl = body.substring(urlStart, urlEnd);
          Serial.printf("新图片URL: %s\n", imageUrl.c_str());

          // 保存图片地址到Flash存储
          if (StorageComm.saveImageUrl(imageUrl)) {
            sendResponse(client, 200, "application/json", "{\"success\": true, \"message\": \"图片地址更新成功\"}");
          } else {
            sendResponse(client, 500, "application/json", "{\"success\": false, \"message\": \"图片地址保存失败\"}");
          }

          return;
        }
      }
    }
  }

  sendResponse(client, 400, "application/json", "{\"success\": false, \"message\": \"无法解析请求\"}");
}

// 处理BLE蓝牙停止请求
void MyWebServer::handleBLEStop(WiFiClient& client) {
  Serial.println("收到BLE蓝牙停止请求");
  
  // 停止BLE蓝牙
  bleControlStop();
  
  // 返回成功响应
  String json = "{\"success\": true, \"message\": \"BLE蓝牙已停止\"}";
  sendResponse(client, 200, "application/json", json);
}

