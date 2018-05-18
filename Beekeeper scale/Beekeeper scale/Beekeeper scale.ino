
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>							
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <HX711.h>


const int SCALE_DOUT_PIN = D0;
const int SCALE_SCK_PIN = D5;

#define ssid      "Edimax_2g"						// WiFi SSID
#define password  "magnaTpwd0713key"				// WiFi password

String innerPhone = "";                             // variable for inner phone
bool status;
float t = 0;
float p = 0;
float h = 0;
float weight = 0;
float callFactor = 23512.f;

SoftwareSerial SIM800(D7, D6);						// RX, TX
String sendATCommand(String cmd, bool waiting);
String waitResponse();
String whiteListPhones = "+380962372023, +380679832130, +380677748522";			// Withe lists of phone numbers
void sendSMS(String phone, String message);
//void parseSMS(String msg);

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
String _response = "";                             // Response from module SIM800L

void setup() {

	Serial.begin(9600);                            //UART speed with PC
	SIM800.begin(9600);                            //UART speed with SIM800L
	Serial.println("Start!");
	pinMode(pins, OUTPUT);
	digitalWrite(pins, LOW);

	sendATCommand("AT", true);                     // Send AT for auto setup module

												   // Setup commands for every startup modem 
	_response = sendATCommand("AT+CLIP=1", true);  // On automatic number detect
	_response = sendATCommand("AT+DDET=1", true);  // On DTMF

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
	scale.set_scale(callFactor); 
	scale.tare();

}

void handleRoot() {

	server.send(200, "text/html", getPage());

}

String sendATCommand(String cmd, bool waiting) {
	String _resp = "";                            // Result varrible
	Serial.println(cmd);                          // Double command in PC UART
	SIM800.println(cmd);                          // Send command to modem
	if (waiting) {                                // If response need...
		_resp = waitResponse();                     // ... wait, while response will be resived
													
		if (_resp.startsWith(cmd)) {                // Cut from responce double commands
			_resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
		}
		Serial.println(_resp);                      // Dublicate to PC UART
	}
	return _resp;                                 // Return result. Empty, if problem...
}

String waitResponse() {                         // Function wait request and give resul 
	String _resp = "";                            // Variable for result
	long _timeout = millis() + 10000;             // Timeout (10 sec) variable
	while (!SIM800.available() && millis() < _timeout) {}; // Wait for 10 sec
	if (SIM800.available()) {
		_resp = SIM800.readString();
	}
	else {                                        // If timeout...
		Serial.println("Timeout...");             // ... alert about it...
	}
	return _resp;
}                                                 // ... return result. Empty, if problem....

void sendSMS(String innerPhone, String message) {
	sendATCommand("AT+CMGS=\"" + innerPhone + "\"", true);             
	sendATCommand(message + "\r\n" + (String)((char)26), true);   
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
	if (SIM800.available()) {                   // If modem send something...
		_response = waitResponse();                 // Recive ansver from modem for analise
		_response.trim();                           // Cut withe spaces in start and of line
		Serial.println(_response);                  // Print in UART PC


		if (_response.startsWith("RING")) {         
			int phoneindex = _response.indexOf("+CLIP: \"");// If phone number detected, then phoneindex>-1

			if (phoneindex >= 0) {                    // If information was find
				phoneindex += 8;                        // Parse line...
				innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...take number
				Serial.println("Number: " + innerPhone); // Print number to UART PC
			}

			if (innerPhone.length() >= 7 && whiteListPhones.indexOf(innerPhone) >= 0) {


				sendATCommand("ATA", true);         // To answer a call

			}
			else if (innerPhone.length() >= 7 && whiteListPhones != innerPhone)
			{
				Serial.println("Phone number is not in white list");
				sendATCommand("ATH", true);
				sendSMS("+380962372023", "ATANTION!!! Number: " + innerPhone + " is out of white list");
			}
		}

		if (_response.startsWith("+DTMF:")) {
			
			String symbol = _response.substring(7, 8); //Take symbol from 7 position
			Serial.println("Key" + symbol);
						
			if (symbol == "1") {
				
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

	if (Serial.available()) {                       // Wait for command from UART PC...
		SIM800.write(Serial.read());                // ...and trancive this command to modem
	}
}