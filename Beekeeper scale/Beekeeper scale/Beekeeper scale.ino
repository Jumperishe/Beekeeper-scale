
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>                                   // Библиотека програмной реализации обмена по UART-протоколу
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>


	SoftwareSerial SIM800(D7, D6);                                // RX, TX
			String sendATCommand(String cmd, bool waiting);
			String waitResponse();
			String whiteListPhones = "+380962372023";       // Белый список телефонов
			void sendSMS(String phone, String message);
			void parseSMS(String msg);

			float t;
			float p;
			float h;
			Adafruit_BME280 bme;
			int pins = D8;                                  // Пины с подключенными светодиодами
															//int state = 0;
			String _response = "";                          // Переменная для хранения ответа модуля
			void setup() {
				Serial.begin(9600);                           // Скорость обмена данными с компьютером
				SIM800.begin(9600);                           // Скорость обмена данными с модемом
				Serial.println("Start!");
				pinMode(pins, OUTPUT);
				digitalWrite(pins, LOW);
				sendATCommand("AT", true);                    // Отправили AT для настройки скорости обмена данными
				bool status;
				// Команды настройки модема при каждом запуске
				_response = sendATCommand("AT+CLIP=1", true);  // Включаем АОН
				_response = sendATCommand("AT+DDET=1", true);  // Включаем DTMF

				status = bme.begin();
				if (!status) {
					Serial.println("BMP280 KO!");
					while (1);
				}
				else {
					Serial.println("BMP280 OK");
				}
			}

			String sendATCommand(String cmd, bool waiting) {
				String _resp = "";                            // Переменная для хранения результата
				Serial.println(cmd);                          // Дублируем команду в монитор порта
				SIM800.println(cmd);                          // Отправляем команду модулю
				if (waiting) {                                // Если необходимо дождаться ответа...
					_resp = waitResponse();                     // ... ждем, когда будет передан ответ
																// Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
					if (_resp.startsWith(cmd)) {                // Убираем из ответа дублирующуюся команду
						_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
					}
					Serial.println(_resp);                      // Дублируем ответ в монитор порта
				}
				return _resp;                                 // Возвращаем результат. Пусто, если проблема
			}

			String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
				String _resp = "";                            // Переменная для хранения результата
				long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
				while (!SIM800.available() && millis() < _timeout) {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
				if (SIM800.available()) {                     // Если есть, что считывать...
					_resp = SIM800.readString();                // ... считываем и запоминаем
				}
				else {                                        // Если пришел таймаут, то...
					Serial.println("Timeout...");               // ... оповещаем об этом и...
				}
				return _resp;                                 // ... возвращаем результат. Пусто, если проблема
			}

			void sendSMS(String phone, String message) {
				sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // Переходим в режим ввода текстового сообщения
				sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
			}

			void parseSMS(String msg) {
				String msgheader = "";
				String msgbody = "";
				String msgphone = "";

				msg = msg.substring(msg.indexOf("+CMGR: "));
				msgheader = msg.substring(0, msg.indexOf("\r"));

				msgbody = msg.substring(msgheader.length() + 2);
				msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));
				msgbody.trim();

				int firstIndex = msgheader.indexOf("\",\"") + 3;
				int secondIndex = msgheader.indexOf("\",\"", firstIndex);
				msgphone = msgheader.substring(firstIndex, secondIndex);

				Serial.println("Phone: " + msgphone);
				Serial.println("Message: " + msgbody);

				if (msgphone.length() > 6 && whiteListPhones.indexOf(msgphone) > -1) {
					/*if (msgbody.equals("11")) {
					digitalWrite(pins, HIGH);
					Serial.println("Pin D8 is set HIGH by SMS reqest ");
					}*/
				}
			}


			void loop() {
				if (SIM800.available()) {                   // Если модем, что-то отправил...
					_response = waitResponse();                 // Получаем ответ от модема для анализа
					_response.trim();                           // Убираем лишние пробелы в начале и конце
					Serial.println(_response);                  // Если нужно выводим в монитор порта


					if (_response.startsWith("RING")) {         // Есть входящий вызов
						int phoneindex = _response.indexOf("+CLIP: \"");// Есть ли информация об определении номера, если да, то phoneindex>-1
						String innerPhone = "";                   // Переменная для хранения определенного номера
						if (phoneindex >= 0) {                    // Если информация была найдена
							phoneindex += 8;                        // Парсим строку и ...
							innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...получаем номер
							Serial.println("Number: " + innerPhone); // Выводим номер в монитор порта
						}

						if (innerPhone.length() >= 7 && whiteListPhones.indexOf(innerPhone) >= 0) {


							sendATCommand("ATA", true);         // Поднимаем трубку

						}
						else if (innerPhone.length() >= 7 && whiteListPhones != innerPhone)
						{
							Serial.println("Phone number is not in white list");
							sendATCommand("ATH", true);
							sendSMS(whiteListPhones, "ATANTION!!! Number: " + innerPhone + " is out of white list");
						}
					}

					if (_response.startsWith("+DTMF:")) {
						//Serial.println(">" + _response);
						String symbol = _response.substring(7, 8); //Выдергиваем символ с 7 позиции длиной 1 (по 8)
						Serial.println("Key" + symbol);

						/*switch (symbol)
						{
						case "1":

						break;
						default:
						break;
						}*/

						if (symbol == "1") {
							//digitalWrite(pins, HIGH);
							t = bme.readTemperature();
							delay(100);
							h = bme.readHumidity();
							delay(100);
							p = bme.readPressure() / 133.32239F;
							delay(100);

							String temp = String(t);

							Serial.println(t);
							Serial.println(h);
							Serial.println(p);
							sendSMS("+380962372023", "Temperature: " + temp + "C \r\n" + "Humid: " + String(h) + "% \r\n" + "Pressure: " + String(p) + "mmHg");
						}

						if (symbol == "0") {
							digitalWrite(pins, LOW);
							Serial.println("Pin D8 is set to LOW");
						}

					}

					if (_response.startsWith("+CMTI:")) {       // Пришло сообщение об отправке SMS
						int index = _response.lastIndexOf(",");   // Находим последнюю запятую, перед индексом
						String result = _response.substring(index + 1, _response.length()); // Получаем индекс
						result.trim();                            // Убираем пробельные символы в начале/конце
						_response = sendATCommand("AT+CMGR=" + result, true); // Получить содержимое SMS
						parseSMS(_response);                      // Распарсить SMS на элементы

						sendATCommand("AT+CMGDA=\"DEL ALL\"", true); // Удалить все сообщения, чтобы не забивали память модуля
					}

				}
				if (Serial.available()) {                    // Ожидаем команды по Serial...
					SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
				}
			}