
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>                                   // ���������� ���������� ���������� ������ �� UART-���������
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>


	SoftwareSerial SIM800(D7, D6);                                // RX, TX
			String sendATCommand(String cmd, bool waiting);
			String waitResponse();
			String whiteListPhones = "+380962372023";       // ����� ������ ���������
			void sendSMS(String phone, String message);
			void parseSMS(String msg);

			float t;
			float p;
			float h;
			Adafruit_BME280 bme;
			int pins = D8;                                  // ���� � ������������� ������������
															//int state = 0;
			String _response = "";                          // ���������� ��� �������� ������ ������
			void setup() {
				Serial.begin(9600);                           // �������� ������ ������� � �����������
				SIM800.begin(9600);                           // �������� ������ ������� � �������
				Serial.println("Start!");
				pinMode(pins, OUTPUT);
				digitalWrite(pins, LOW);
				sendATCommand("AT", true);                    // ��������� AT ��� ��������� �������� ������ �������
				bool status;
				// ������� ��������� ������ ��� ������ �������
				_response = sendATCommand("AT+CLIP=1", true);  // �������� ���
				_response = sendATCommand("AT+DDET=1", true);  // �������� DTMF

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
				String _resp = "";                            // ���������� ��� �������� ����������
				Serial.println(cmd);                          // ��������� ������� � ������� �����
				SIM800.println(cmd);                          // ���������� ������� ������
				if (waiting) {                                // ���� ���������� ��������� ������...
					_resp = waitResponse();                     // ... ����, ����� ����� ������� �����
																// ���� Echo Mode �������� (ATE0), �� ��� 3 ������ ����� ����������������
					if (_resp.startsWith(cmd)) {                // ������� �� ������ ������������� �������
						_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
					}
					Serial.println(_resp);                      // ��������� ����� � ������� �����
				}
				return _resp;                                 // ���������� ���������. �����, ���� ��������
			}

			String waitResponse() {                         // ������� �������� ������ � �������� ����������� ����������
				String _resp = "";                            // ���������� ��� �������� ����������
				long _timeout = millis() + 10000;             // ���������� ��� ������������ �������� (10 ������)
				while (!SIM800.available() && millis() < _timeout) {}; // ���� ������ 10 ������, ���� ������ ����� ��� �������� �������, ��...
				if (SIM800.available()) {                     // ���� ����, ��� ���������...
					_resp = SIM800.readString();                // ... ��������� � ����������
				}
				else {                                        // ���� ������ �������, ��...
					Serial.println("Timeout...");               // ... ��������� �� ���� �...
				}
				return _resp;                                 // ... ���������� ���������. �����, ���� ��������
			}

			void sendSMS(String phone, String message) {
				sendATCommand("AT+CMGS=\"" + phone + "\"", true);             // ��������� � ����� ����� ���������� ���������
				sendATCommand(message + "\r\n" + (String)((char)26), true);   // ����� ������ ���������� ������� ������ � Ctrl+Z
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
				if (SIM800.available()) {                   // ���� �����, ���-�� ��������...
					_response = waitResponse();                 // �������� ����� �� ������ ��� �������
					_response.trim();                           // ������� ������ ������� � ������ � �����
					Serial.println(_response);                  // ���� ����� ������� � ������� �����


					if (_response.startsWith("RING")) {         // ���� �������� �����
						int phoneindex = _response.indexOf("+CLIP: \"");// ���� �� ���������� �� ����������� ������, ���� ��, �� phoneindex>-1
						String innerPhone = "";                   // ���������� ��� �������� ������������� ������
						if (phoneindex >= 0) {                    // ���� ���������� ���� �������
							phoneindex += 8;                        // ������ ������ � ...
							innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...�������� �����
							Serial.println("Number: " + innerPhone); // ������� ����� � ������� �����
						}

						if (innerPhone.length() >= 7 && whiteListPhones.indexOf(innerPhone) >= 0) {


							sendATCommand("ATA", true);         // ��������� ������

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
						String symbol = _response.substring(7, 8); //����������� ������ � 7 ������� ������ 1 (�� 8)
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

					if (_response.startsWith("+CMTI:")) {       // ������ ��������� �� �������� SMS
						int index = _response.lastIndexOf(",");   // ������� ��������� �������, ����� ��������
						String result = _response.substring(index + 1, _response.length()); // �������� ������
						result.trim();                            // ������� ���������� ������� � ������/�����
						_response = sendATCommand("AT+CMGR=" + result, true); // �������� ���������� SMS
						parseSMS(_response);                      // ���������� SMS �� ��������

						sendATCommand("AT+CMGDA=\"DEL ALL\"", true); // ������� ��� ���������, ����� �� �������� ������ ������
					}

				}
				if (Serial.available()) {                    // ������� ������� �� Serial...
					SIM800.write(Serial.read());                // ...� ���������� ���������� ������� ������
				}
			}