//Проверяем и отправляем в порт температуру процессора.
//Судя по сообщениям в Инете эта функция не работает и выдаёт обрыв = 128F = 53.33C
//Внутренний датчик температуры (вернее доступ к нему) залочен ESP.

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();
char info_buffer[80];//Используется для определения температуры проца
volatile uint8_t  info_available = 0;//, output_ready = 0, websock_num = 0; //Используется для определения температуры проца

void sendTemperature() {
  sprintf(info_buffer, "Температура процессора: %.3f", temperatureRead());//Записывает в info_buffer значение temperatureRead() в формате "Text %f", где %f = float
  info_available = 1;
  Serial.println(info_buffer);
  // Convert raw temperature in F to Celsius degrees
  Serial.print("temprature_sens_read() - 32) / 1.8 = ");
  Serial.print((temprature_sens_read() - 32) / 1.8);
  Serial.println(" C");

};
