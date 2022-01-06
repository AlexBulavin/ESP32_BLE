/*
  Сервис для циклической бубликации сообщений пользователя с периодом 1 сообщение в 30 секунд.
  Предусмотрена возможностью публикации в эфир стороннего сообщения.
  Для этого необходимо через приложение nRF Connect (или подобное) с телефона подключиться к девайсу с именем, определённом в DEVICE_NAME.
  Перейти во вкладку Client.
  На характеристике ca9512c1-32e8-4ba3-9a48-2a8cda1e8d нажать стрелку вверх ^,
  В открывшемся окне написать необходимый текст (на любом языке), выбрать кодировку UTF8, нажать кнопку Write.
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "intents.h"
#include "lcd_indication.h"
#include "temperature.h"
#include "voltage.h"

BLECharacteristic *pCharacteristic;
BLECharacteristic *sCharacteristic;

#define DEVICE_MANUFACTURER "Manufactured by: Universe devices LTD" // Имя "Производителя"
#define DEVICE_NAME "ESP for everyone" // Имя устройства


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define DEVICE_SERVICE_UUID "0000180a-0000-1000-8000-00805f9b34fb"
#define SERVICE_UUID        "fb3415d3-da43-4ac8-af1e-118b4ac96d46"//Случайно сгенерированный UUID отсюда: https://www.uuidgenerator.net/https://www.uuidgenerator.net/
#define CHARACTERISTIC_UUID "ca9512c1-32e8-4ba3-9a48-2a8cda1e8dcd"//Случайно сгенерированный UUID отсюда: https://www.uuidgenerator.net/https://www.uuidgenerator.net/
#define CHARACTERISTIC_MFCR_UUID "00002a29-0000-1000-8000-00805f9b34fb"//"Manufacturer Name String"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN            2

int fading_brightness = 0;    // how bright the LED is
int connection_brightness = 100;//Для индикации подключенного пользователя включаем постоянно синий светодиод на установленную этой переменной величину. Максимум 255
int fadeAmount = 5;    // how many points to fade the LED by one step (30ms)

bool deviceConnected = false;
unsigned long advTime = 20000;//Время публикации
unsigned long onWrite_primary_timer = 0;
unsigned long onWrite_secondary_timer = 0;

unsigned long led_timer = 0;

unsigned long publishing_timer = 0;
bool user_data_step_trigger = true;//Тоггл для возможности смены строки из массива intents
int intents_counter = 0;//Счётчик для элементов массива intents
int dots_counter = 0;//Счётчик для напечатанных точек

bool onWrite_trigger = false; //Триггер на коллбэк по событию на запись сторонним пользователем в характеристику данных
const int ledPin = 2;//Выводим индикацию на внутренний синий светодиод со 2 пина

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      ledcAnalogWrite(LEDC_CHANNEL_0, connection_brightness);
      Serial.println("Client connected");
      pServer->getAdvertising()->start();
      
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected");
      ledcWrite(LEDC_CHANNEL_0, 0);
    };
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("********* Set new value for characteristic *********");
        Serial.println(value.c_str());//Преобразовали массив в строку и отправили на печать.
        Serial.println("****************************************************");
        Serial.print("And advertise for ");
        Serial.print(advTime / 1000);
        Serial.println(" seconds!");
      };
      onWrite_primary_timer = millis();//Зафиксировали время размещения публикации
      onWrite_secondary_timer = onWrite_primary_timer;//Для посекундной индикации взвели второй таймер с тем же начальным временем
      led_timer = millis();
      onWrite_trigger = true;
      pCharacteristic->setValue(value);
      pCharacteristic->notify();
      pCharacteristic->indicate();

    };
};

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay (500);
  digitalWrite(ledPin, LOW);

  sendTemperature();//Проверяем и отправляем в порт температуру процессора
  sendVoltage();//Прорверяем и отправляем в порт напряжение

  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pServer->setCallbacks(new MyServerCallbacks());

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |

                      BLECharacteristic::PROPERTY_WRITE |

                      BLECharacteristic::PROPERTY_NOTIFY |

                      BLECharacteristic::PROPERTY_INDICATE
                    );

  sCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_MFCR_UUID,
                      BLECharacteristic::PROPERTY_READ |

                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  sCharacteristic->setCallbacks(new MyCallbacks());

  sCharacteristic->setValue(DEVICE_MANUFACTURER);
  Serial.println(DEVICE_MANUFACTURER);

  pCharacteristic->setValue(intents[0]);//Берём последнее значение из массива intents[] и его публикуем как стартовое.
  Serial.println(intents[0]);

  //BLE2902 needed to notify
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  pServer->getAdvertising()->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);

  publishing_timer = millis();//Взводим таймер для того, чтобы считать время публикации и по его окончании публиковать следующий элемент из массива пользовательских интентов.
};


void loop() {
  if ((millis() - onWrite_primary_timer) > advTime) {

    if (((millis() - publishing_timer) > advTime) && user_data_step_trigger) {//Закончился адвертайзинг стороннего пользовательского сообщения, можно публиковать очередное админское сообщение
      if (deviceConnected) {
        ledcAnalogWrite(LEDC_CHANNEL_0, connection_brightness);
      };
      pCharacteristic->setValue(intents[intents_counter]);
      pCharacteristic->notify();
      pCharacteristic->indicate();

      intents_counter++;
      if (intents_counter > (sizeof(intents) / sizeof(intents[0])) - 1)  { //Вычисляем количество элементов массива:sizeof(intents) / sizeof(intents[0])
        intents_counter = 0;
      };

      onWrite_trigger = false;
      user_data_step_trigger = false;
      publishing_timer = millis();
      Serial.println(intents[intents_counter]);

    } else if (((millis() - publishing_timer) < advTime) && !user_data_step_trigger) {//В это время ещё идёт адвертайзинг записанного сторонним пользователем сообщения
      onWrite_trigger = false;
      user_data_step_trigger = true;
    };
  }

  if (onWrite_trigger && ((millis() - onWrite_primary_timer) < advTime))  {//
    if ((millis() - onWrite_secondary_timer) > 1000) {//Выводим в порт точку каждую секунду
        Serial.print("*");
        dots_counter++;
        onWrite_secondary_timer = millis();//Сбрасываем таймер к текущему значению времени
      }
    if ((millis() - led_timer) > 30) {
      // set the fading_brightness on LEDC channel 0
      ledcAnalogWrite(LEDC_CHANNEL_0, fading_brightness);

      // change the fading_brightness for next time through the loop:
      fading_brightness = fading_brightness + fadeAmount;

      // reverse the direction of the fading at the ends of the fade:
      if (fading_brightness <= 0 || fading_brightness >= 255) {
        fadeAmount = -fadeAmount;
      }
      // wait for 30 milliseconds to see the dimming effect
      led_timer = millis();
    }
  } 
  if (advTime/1000 <= dots_counter+1) {//Сравниваем количество напечатанных * с секундами адвертайзинга.
    //Когда напечатаны все точки, печатаем последнюю точку и переводим строку
    Serial.println("*");
    dots_counter = 0;
    };

};
