
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>							// ���������� ���������� ���������� ������ �� UART-���������
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <HX711.h>


const int SCALE_DOUT_PIN = D0;
const int SCALE_SCK_PIN = D5;

#define ssid      "Edimax_2g"						// WiFi SSID
#define password  "magnaTpwd0713key"				// WiFi password

String innerPhone = "";                             // ���������� ��� �������� ������������� ������
bool status;
float t = 0;
float p = 0;
float h = 0;
float weight = 0;
float callFactor = 23512.f;

SoftwareSerial SIM800(D7, D6);						// RX, TX
String sendATCommand(String cmd, bool waiting);
String waitResponse();
String whiteListPhones = "+380962372023, +380679832130, +380677748522";			// ����� ������ ���������
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
String _response = "";                             // ���������� ��� �������� ������ ������

void setup() {

	Serial.begin(9600);                            // �������� ������ ������� � �����������
	SIM800.begin(9600);                            // �������� ������ ������� � �������
	Serial.println("Start!");
	pinMode(pins, OUTPUT);
	digitalWrite(pins, LOW);

	sendATCommand("AT", true);                     // ��������� AT ��� ��������� �������� ������ �������

												   // ������� ��������� ������ ��� ������ �������
	_response = sendATCommand("AT+CLIP=1", true);  // �������� ���
	_response = sendATCommand("AT+DDET=1", true);  // �������� DTMF

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

void sendSMS(String innerPhone, String message) {
	sendATCommand("AT+CMGS=\"" + innerPhone + "\"", true);             // ��������� � ����� ����� ���������� ���������
	sendATCommand(message + "\r\n" + (String)((char)26), true);   // ����� ������ ���������� ������� ������ � Ctrl+Z
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
	if (SIM800.available()) {                   // ���� �����, ���-�� ��������...
		_response = waitResponse();                 // �������� ����� �� ������ ��� �������
		_response.trim();                           // ������� ������ ������� � ������ � �����
		Serial.println(_response);                  // ���� ����� ������� � ������� �����


		if (_response.startsWith("RING")) {         // ���� �������� �����
			int phoneindex = _response.indexOf("+CLIP: \"");// ���� �� ���������� �� ����������� ������, ���� ��, �� phoneindex>-1

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
				sendSMS("+380962372023", "ATANTION!!! Number: " + innerPhone + " is out of white list");
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

		/*if (_response.startsWith("+CMTI:")) {         // ������ ��������� �� �������� SMS
		int index = _response.lastIndexOf(",");   // ������� ��������� �������, ����� ��������
		String result = _response.substring(index + 1, _response.length()); // �������� ������
		result.trim();                            // ������� ���������� ������� � ������/�����
		_response = sendATCommand("AT+CMGR=" + result, true); // �������� ���������� SMS
		parseSMS(_response);                      // ���������� SMS �� ��������

		sendATCommand("AT+CMGDA=\"DEL ALL\"", true); // ������� ��� ���������, ����� �� �������� ������ ������
		}*/

	}
	server.handleClient();
	t = bme.readTemperature();
	h = bme.readHumidity();
	p = bme.readPressure() / 133.32239F;
	weight = scale.get_units(10);
	delay(100);

	if (Serial.available()) {                       // ������� ������� �� Serial...
		SIM800.write(Serial.read());                // ...� ���������� ���������� ������� ������
	}
}