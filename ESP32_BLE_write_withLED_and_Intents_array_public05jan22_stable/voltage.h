//Проверяем и отправляем текущее напряжение
char info_bufferV[80];//Используется для определения напряжения
volatile uint8_t  info_availableV = 0;//, output_ready = 0, websock_num = 0; //Используется для определения температуры проца
const int ADCbat = 36; // VP pin питания

void sendVoltage() {
  double reading = analogRead(ADCbat); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
  //if(reading < 1 || reading > 4095) return 0;
  reading = -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
  float batVoltage = reading * 2; //analogRead(ADCbat)/4095.0*3.3*2; //12bit ADC, 50:50 voltage divider
  sprintf(info_bufferV, "Напряжение: %.2f", batVoltage);
  info_availableV = 1;
  Serial.println(info_bufferV);
}
