
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>							// Библиотека програмной реализации обмена по UART-протоколу
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <HX711.h>


const int SCALE_DOUT_PIN = D0;
const int SCALE_SCK_PIN = D5;

#define ssid      "Edimax_2g"						// WiFi SSID
#define password  "magnaTpwd0713key"				// WiFi password

String innerPhone = "";                             // Переменная для хранения определенного номера
bool status;
float t = 0;
float p = 0;
float h = 0;
float weight = 0;
float callFactor = 23512.f;

SoftwareSerial SIM800(D7, D6);						// RX, TX
String sendATCommand(String cmd, bool waiting);
String waitResponse();
String whiteListPhones = "+380962372023, +380679832130, +380677748522";			// Белый список телефонов
void sendSMS(String phone, String message);
void parseSMS(String msg);

Adafruit_BME280 bme;
ESP8266WebServer server(80);
HX711 scale(SCALE_DOUT_PIN, SCALE_SCK_PIN);
String getPage() {
	String page = "<html lang='en'><head><meta http-equiv='refresh' content='120' name='viewport' content='width=device-width, initial-scale=1'/>";
	page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
	page += "<title>ESP8266 </title></head><body>";
	page += "<div class='container-fluid'>";
	page += "<div class='row'>";
	page += "<div class='col-md-12'>";
	page += "<h1>Beekeeper csale</h1>";
	page += "<ul class='nav nav-pills'>";
	page += "</li><li>";
	page += "<a href='#'> <span class='badge pull-right'>";
	page += t;
	page += "</span> Temperature</a>";
	page += "</li><li>";
	page += "<a href='#'> <span class='badge pull-right'>";
	page += h;
	page += "</span> Humidity</a>";
	page += "</li><li>";
	page += "<a href='#'> <span class='badge pull-right'>";
	page += p;
	page += "</span> Pressure</a></li>";
	page += "</li><li>";
	page += "<a href='#'> <span class='badge pull-right'>";
	page += weight;
	page += "</span>Weight</a></li>";
	page += "</ul>";
	page += "<table class='table'>";
	page += "<thead><tr><th>Sensor</th><th>Measure</th><th>Value</th></tr></thead>";
	page += "<tbody>";
	page += "<tr><td>BME280</td><td>Temperature</td><td>";
	page += t;
	page += "&deg;C</td><td>";
	page += "-</td></tr>";
	page += "<tr class='active'><td>BME280</td><td>Humidity</td><td>";
	page += h;
	page += "%</td><td>";
	page += "</td></tr>";
	page += "<tr><td>BME280</td><td>Pressure</td><td>";
	page += p;
	page += "mmHg</td><td>";
	page += "-</td></tr>";
	page += "<tr><td>Load Cell</td><td>Kg</td><td>";
	page += weight;
	page += "Kg</td><td>";
	page += "-</td></tr>";
	page += "</tbody></table>";
	page += "</div></div></div>";
	page += "</body></html>";
	return page;
}

int pins = D8;
String _response = "";                             // Переменная для хранения ответа модуля

void setup() {

	Serial.begin(9600);                            // Скорость обмена данными с компьютером
	SIM800.begin(9600);                            // Скорость обмена данными с модемом
	Serial.println("Start!");
	pinMode(pins, OUTPUT);
	digitalWrite(pins, LOW);

	sendATCommand("AT", true);                     // Отправили AT для настройки скорости обмена данными

												   // Команды настройки модема при каждом запуске
	_response = sendATCommand("AT+CLIP=1", true);  // Включаем АОН
	_response = sendATCommand("AT+DDET=1", true);  // Включаем DTMF

												   /********************************************************Wi-Fi and Server inicialisation****************/
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500); Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to "); Serial.println(ssid);
	Serial.print("IP address: "); Serial.println(WiFi.localIP());

	server.on("/", handleRoot);

	server.begin();
	Serial.println("HTTP server started");
	/*****************************************************************************************************/

	status = bme.begin();
	if (!status) {
		Serial.println("BMP280 KO!");
		while (1);
	}
	else {
		Serial.println("BMP280 OK");
	}

	scale.read();
	scale.set_gain(128);
	scale.set_scale(callFactor); // <- set here calibration factor!!!
	scale.tare();

}

void handleRoot() {

	server.send(200, "text/html", getPage());

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

void sendSMS(String innerPhone, String message) {
	sendATCommand("AT+CMGS=\"" + innerPhone + "\"", true);             // Переходим в режим ввода текстового сообщения
	sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
}

/*void parseSMS(String msg) {
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
}*/


void loop() {
	if (SIM800.available()) {                   // Если модем, что-то отправил...
		_response = waitResponse();                 // Получаем ответ от модема для анализа
		_response.trim();                           // Убираем лишние пробелы в начале и конце
		Serial.println(_response);                  // Если нужно выводим в монитор порта


		if (_response.startsWith("RING")) {         // Есть входящий вызов
			int phoneindex = _response.indexOf("+CLIP: \"");// Есть ли информация об определении номера, если да, то phoneindex>-1

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
				sendSMS("+380962372023", "ATANTION!!! Number: " + innerPhone + " is out of white list");
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
				sendSMS(innerPhone, "Temperature: " + temp + "C \r\n" + "Humid: " + String(h) + "% \r\n" + "Pressure: " + String(p) + "mmHg");
			}

			if (symbol == "0") {
				scale.tare();
			}

			if (symbol == "2") {
				weight = scale.get_units(10);
				Serial.println(String(weight, 2));
			}

		}

		/*if (_response.startsWith("+CMTI:")) {         // Пришло сообщение об отправке SMS
		int index = _response.lastIndexOf(",");   // Находим последнюю запятую, перед индексом
		String result = _response.substring(index + 1, _response.length()); // Получаем индекс
		result.trim();                            // Убираем пробельные символы в начале/конце
		_response = sendATCommand("AT+CMGR=" + result, true); // Получить содержимое SMS
		parseSMS(_response);                      // Распарсить SMS на элементы

		sendATCommand("AT+CMGDA=\"DEL ALL\"", true); // Удалить все сообщения, чтобы не забивали память модуля
		}*/

	}
	server.handleClient();
	t = bme.readTemperature();
	h = bme.readHumidity();
	p = bme.readPressure() / 133.32239F;
	weight = scale.get_units(10);
	delay(100);

	if (Serial.available()) {                       // Ожидаем команды по Serial...
		SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
	}
}