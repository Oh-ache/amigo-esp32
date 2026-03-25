#include <Arduino.h>
#include "ble/BLECommunication.h"
#include "wifi/WiFiManager.h"
#include "web/WebServer.h"

void setup() {
  // 初始化串口（对于ESP32-S3，我们需要使用Serial0或Serial1）
  // 大多数ESP32-S3开发板使用USB-C作为调试串口
  Serial.begin(115200);

  // 等待串口连接，超时5秒
  unsigned long startMillis = millis();
  while (!Serial && (millis() - startMillis) < 5000) {
    delay(100);
  }

  Serial.println("ESP32 S3 设备初始化");

  // 根据WiFi连接方式进行初始化
#ifdef WIFI_CONNECT_MODE
  switch (WIFI_CONNECT_MODE) {
    case 1:
      // 模式1：通过BLE蓝牙配网（默认流程）
      Serial.println("初始化模式1：通过BLE蓝牙配网");

      // 初始化BLE蓝牙
      if (BLEComm.init()) {
        Serial.println("BLE蓝牙初始化成功");

        // 开启BLE蓝牙
        BLEComm.start();

        // 设置蓝牙名称（可选，默认已设置为ESP32-BLE）
        // BLEComm.setDeviceName("ESP32-BLE");
      } else {
        Serial.println("BLE蓝牙初始化失败");
      }
      break;

    case 2:
      // 模式2：直接连接WiFi（无需BLE蓝牙配网）
      Serial.println("初始化模式2：直接连接WiFi");

      // 从宏定义中获取WiFi账号密码
#ifdef WIFI_SSID
#ifdef WIFI_PASSWORD
      Serial.printf("正在连接到WiFi网络: %s\n", WIFI_SSID);

      // 直接连接WiFi
      if (WiFiComm.connect(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("WiFi连接成功");
        Serial.printf("IP地址: %s\n", WiFiComm.getIPAddress().c_str());

        // 启动Web服务器
        Serial.println("正在启动Web服务器");
        if (WebComm.init()) {
          if (WebComm.start()) {
            Serial.println("Web服务器启动成功");
            Serial.printf("访问地址: http://%s/device-id\n", WiFiComm.getIPAddress().c_str());
          } else {
            Serial.println("Web服务器启动失败");
          }
        } else {
          Serial.println("Web服务器初始化失败");
        }
      } else {
        Serial.println("WiFi连接失败");
      }
#else
      Serial.println("错误：未配置WiFi密码（WIFI_PASSWORD）");
#endif // WIFI_PASSWORD
#else
      Serial.println("错误：未配置WiFi账号（WIFI_SSID）");
#endif // WIFI_SSID
      break;

    default:
      Serial.println("警告：无效的WiFi连接模式，默认使用模式1（BLE配网）");
      // 默认为模式1

      // 初始化BLE蓝牙
      if (BLEComm.init()) {
        Serial.println("BLE蓝牙初始化成功");
        BLEComm.start();
      } else {
        Serial.println("BLE蓝牙初始化失败");
      }
      break;
  }
#else
  // 如果未定义WIFI_CONNECT_MODE，默认使用模式1（BLE配网）
  Serial.println("未配置WiFi连接模式，默认使用模式1（BLE配网）");

  // 初始化BLE蓝牙
  if (BLEComm.init()) {
    Serial.println("BLE蓝牙初始化成功");
    BLEComm.start();
  } else {
    Serial.println("BLE蓝牙初始化失败");
  }
#endif // WIFI_CONNECT_MODE
}

void loop() {
  // 根据WiFi连接方式处理循环逻辑
#ifdef WIFI_CONNECT_MODE
  switch (WIFI_CONNECT_MODE) {
    case 1: {
      // 模式1：BLE蓝牙配网模式
      // 检查是否有数据可用
      if (BLEComm.hasData()) {
        String receivedData = BLEComm.readData();
        Serial.print("收到数据: ");
        Serial.println(receivedData);

        // 检查是否是WiFi连接命令
        if (receivedData.indexOf(',') != -1) {
          Serial.println("尝试连接WiFi网络");

          // 连接WiFi
          if (WiFiComm.connect(receivedData)) {
            Serial.println("WiFi连接成功");
            // 发送WiFi连接成功通知（包含IP地址）
            String notifyMessage = "WiFi连接成功,IP:" + WiFiComm.getIPAddress();
            BLEComm.sendData(notifyMessage);
            // 等待通知发送完成
            delay(1000);

            // 启动Web服务器
            Serial.println("正在启动Web服务器");
            if (WebComm.init()) {
              if (WebComm.start()) {
                Serial.println("Web服务器启动成功");
                Serial.printf("访问地址: http://%s/device-id\n", WiFiComm.getIPAddress().c_str());
              } else {
                Serial.println("Web服务器启动失败");
              }
            } else {
              Serial.println("Web服务器初始化失败");
            }
          } else {
            Serial.println("WiFi连接失败");
            BLEComm.sendData("WiFi连接失败");
            // 等待通知发送完成
            delay(1000);
          }

          // 关闭BLE蓝牙
          Serial.println("正在关闭BLE蓝牙");
          BLEComm.stop();
        }

        // 其他命令处理
        if (receivedData == "turn_off") {
          Serial.println("收到关闭蓝牙命令");
          BLEComm.stop();
          // 可以在这里添加延迟或其他操作
          delay(1000);
          Serial.println("程序结束");
          ESP.restart(); // 重启设备（可选）
        }
      }

      // 检查蓝牙连接状态
      static bool oldConnectionStatus = false;
      bool currentConnectionStatus = BLEComm.isConnected();
      if (currentConnectionStatus != oldConnectionStatus) {
        oldConnectionStatus = currentConnectionStatus;
        if (currentConnectionStatus) {
          Serial.println("蓝牙已连接");
        } else {
          Serial.println("蓝牙已断开");
        }
      }
      break;
    }

    case 2: {
      // 模式2：直接连接WiFi模式

      // 检查WiFi连接状态
      static bool oldWiFiStatus = false;
      bool currentWiFiStatus = WiFiComm.isConnected();
      if (currentWiFiStatus != oldWiFiStatus) {
        oldWiFiStatus = currentWiFiStatus;
        if (currentWiFiStatus) {
          Serial.printf("WiFi已连接到: %s\n", WiFiComm.getSSID().c_str());
          Serial.printf("IP地址: %s\n", WiFiComm.getIPAddress().c_str());
        } else {
          Serial.println("WiFi连接已断开，正在尝试重连");
          // 尝试重新连接
          WiFiComm.connect(WIFI_SSID, WIFI_PASSWORD);
        }
      }

      // 处理Web服务器请求
      if (WiFiComm.isConnected() && WebComm.isRunning()) {
        WebComm.handleClient();
      } else if (WiFiComm.isConnected() && !WebComm.isRunning()) {
        // 如果WiFi已连接但Web服务器未启动，则启动Web服务器
        Serial.println("正在启动Web服务器");
        if (WebComm.init()) {
          if (WebComm.start()) {
            Serial.println("Web服务器启动成功");
            Serial.printf("访问地址: http://%s/device-id\n", WiFiComm.getIPAddress().c_str());
          } else {
            Serial.println("Web服务器启动失败");
          }
        } else {
          Serial.println("Web服务器初始化失败");
        }
      }
      break;
    }

    default: {
      // 默认处理（模式1）
      if (BLEComm.hasData()) {
        String receivedData = BLEComm.readData();
        Serial.print("收到数据: ");
        Serial.println(receivedData);

        if (receivedData.indexOf(',') != -1) {
          Serial.println("尝试连接WiFi网络");

          if (WiFiComm.connect(receivedData)) {
            Serial.println("WiFi连接成功");
            String notifyMessage = "WiFi连接成功,IP:" + WiFiComm.getIPAddress();
            BLEComm.sendData(notifyMessage);
            delay(1000);

            Serial.println("正在启动Web服务器");
            if (WebComm.init()) {
              if (WebComm.start()) {
                Serial.println("Web服务器启动成功");
                Serial.printf("访问地址: http://%s/device-id\n", WiFiComm.getIPAddress().c_str());
              }
            }
          } else {
            Serial.println("WiFi连接失败");
            BLEComm.sendData("WiFi连接失败");
            delay(1000);
          }

          BLEComm.stop();
        }

        if (receivedData == "turn_off") {
          Serial.println("收到关闭蓝牙命令");
          BLEComm.stop();
          delay(1000);
          Serial.println("程序结束");
          ESP.restart();
        }
      }

      static bool oldConnStatus = false;
      bool currConnStatus = BLEComm.isConnected();
      if (currConnStatus != oldConnStatus) {
        oldConnStatus = currConnStatus;
        Serial.println(currConnStatus ? "蓝牙已连接" : "蓝牙已断开");
      }
      break;
    }
  }
#else
  // 默认流程（未定义WIFI_CONNECT_MODE）
  if (BLEComm.hasData()) {
    String receivedData = BLEComm.readData();
    Serial.print("收到数据: ");
    Serial.println(receivedData);

    if (receivedData.indexOf(',') != -1) {
      Serial.println("尝试连接WiFi网络");

      if (WiFiComm.connect(receivedData)) {
        Serial.println("WiFi连接成功");
        String notifyMessage = "WiFi连接成功,IP:" + WiFiComm.getIPAddress();
        BLEComm.sendData(notifyMessage);
        delay(1000);

        Serial.println("正在启动Web服务器");
        if (WebComm.init()) {
          if (WebComm.start()) {
            Serial.println("Web服务器启动成功");
            Serial.printf("访问地址: http://%s/device-id\n", WiFiComm.getIPAddress().c_str());
          }
        }
      } else {
        Serial.println("WiFi连接失败");
        BLEComm.sendData("WiFi连接失败");
        delay(1000);
      }

      BLEComm.stop();
    }

    if (receivedData == "turn_off") {
      Serial.println("收到关闭蓝牙命令");
      BLEComm.stop();
      delay(1000);
      Serial.println("程序结束");
      ESP.restart();
    }
  }

  static bool oldConnStatus = false;
  bool currConnStatus = BLEComm.isConnected();
  if (currConnStatus != oldConnStatus) {
    oldConnStatus = currConnStatus;
    Serial.println(currConnStatus ? "蓝牙已连接" : "蓝牙已断开");
  }
#endif // WIFI_CONNECT_MODE

  // 通用的WiFi状态检查（适用于所有模式）
  if (WiFiComm.isConnected() && WebComm.isRunning()) {
    WebComm.handleClient();
  }

  delay(100);
}
