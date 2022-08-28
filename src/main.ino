#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
//#include <ESPAsyncWebServer.h>
#include "epd5in65f.h"
#include <FS.h>
//#include <LITTLEFS.h>
#include <SPIFFS.h>
#include <WebServer.h>


WebServer server(80);
Epd epd;

// SOFT-AP Credentials
#ifdef ESP32
const String AP_SSID = String(String("ePaper-") + String((uint32_t)ESP.getEfuseMac(), HEX));
#endif
#ifdef ESP8266
const String AP_SSID = String(String("ePaper-") + String(ESP.getChipId(), HEX));
#endif
const String AP_PASS = "epd-pass";


void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID.c_str(),AP_PASS.c_str());
    WiFi.begin();
    // WiFi.begin("MySSID", "myPassword"); // Only required on first connect, credentials are saved permanently
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    SPIFFS.begin();
    setupServer();
    pinMode(LED_BUILTIN,OUTPUT);
    epd.Init();
}

void loop() {
    unsigned long ts = millis();
    server.handleClient();
    digitalWrite(LED_BUILTIN, (ts>>8) & 1);
}


void showImage(String filename) {
    Serial.printf("Showing image: ");
    Serial.println(filename);
    if(!SPIFFS.exists(filename)) {
        Serial.println("File does not exist!");
        return;
    }
    File f = SPIFFS.open(filename);
    
    // See https://www.pohlig.de/Unterricht/Inf2004/Kap27/27.3_Das_bmp_Format.htm

    struct bmpHeader
    {
        uint16_t magic;
        uint32_t fileSize;
        uint32_t pixelOffset;
        uint32_t headerSize;
        uint32_t width;
        int32_t height; // negative height: lines are top to bottom
        uint16_t layers;
        uint16_t bitsPerPixel;
    } header;

    uint8_t buf[64];
    f.read(buf, 64 );
    Serial.printf("Byte 0x%02x 0x%02x 0x%02x \n", buf[0], buf[1], buf[2]);
    header.magic = buf[0]<<8 | buf[1];
    header.fileSize = buf[5]<<24 | buf[4]<<16 | buf[3]<<8 | buf[2];
    header.pixelOffset = buf[13]<<24 | buf[12]<<16 | buf[11]<<8 | buf[10];
    header.headerSize = buf[17]<<24 | buf[16]<<16 | buf[15]<<8 | buf[14];
    header.width = buf[21]<<24 | buf[20]<<16 | buf[19]<<8 | buf[18];
    header.height = buf[25]<<24 | buf[24]<<16 | buf[23]<<8 | buf[22];
    header.layers = buf[27]<<8 | buf[26];
    header.bitsPerPixel = buf[29]<<8 | buf[28];
    Serial.printf("Magic 0x%04x Offs %u %u [ %u x %d ] \n", header.magic, header.pixelOffset, header.bitsPerPixel, header.width, header.height);
    Serial.printf("Layers %u Bits %u \n", header.layers, header.bitsPerPixel);
    if(header.magic != 0x424d) {
        Serial.println("Not a BitMap file");
        return;
    }

    bool linesTopToBottom = false;
    if(header.height < 0) {
        linesTopToBottom = true;
        header.height = abs(header.height);
    }

    f.seek(header.pixelOffset);

    uint32_t imageBytes = f.size() - header.pixelOffset;
    Serial.printf("ImageBytes %u \n",imageBytes);
    //imageBytes = 32;
    uint8_t c[1];

    //epd.Reset();
    //delay(200);
    //epd.Init();
    //delay(200);

    epd.SendCommand(0x61);//Set Resolution setting
    epd.SendData(0x02);
    epd.SendData(0x58);
    epd.SendData(0x01);
    epd.SendData(0xC0);
    epd.SendCommand(0x10);

    if(linesTopToBottom) {
        while (imageBytes-- > 0) {
            f.read(c,1);
            epd.SendData(c[0]);
        }
    } else {
        for(int16_t y = header.height-1; y >=0; y--) {
            f.seek(header.pixelOffset + y * (header.width/2));
            for(int16_t x = 0; x < header.width/2; x++) {
                f.read(c,1);
                epd.SendData(c[0]);
            }
        }
    }

    epd.SendCommand(0x04);//0x04
    epd.EPD_5IN65F_BusyHigh();
    epd.SendCommand(0x12);//0x12
    epd.EPD_5IN65F_BusyHigh();
    epd.SendCommand(0x02);  //0x02
    epd.EPD_5IN65F_BusyLow();
    delay(200);
    //epd.Sleep();
	
}



String getContentType(String filename) {
	if (server.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".json")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool handleFileRead(String path) {
	Serial.println("handleFileRead: " + path);
	if (path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
		if (SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void handleFileUpload() {
    static File fsUploadFile;
	if (server.uri() != "/edit") {
		server.send(406,"text/plain","Not Acceptable");
		return;
	}
	HTTPUpload& upload = server.upload();
	if (upload.status == UPLOAD_FILE_START) {
		String filename = upload.filename;
		if (!filename.startsWith("/")) filename = "/" + filename;
		fsUploadFile = SPIFFS.open(filename, "w");
		if(!fsUploadFile) {
			server.send(507,"text/plain","Failed to open file");
		}
	}
	if (upload.status == UPLOAD_FILE_WRITE) {
		if (fsUploadFile) {
			uint32_t size = fsUploadFile.write(upload.buf, upload.currentSize);
		} else {
			server.send(500,"text/plain","No open file");
		}
	}
	if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile) {
			fsUploadFile.close();
			server.send(200,"text/plain","File created");
		} else {
			server.send(500,"text/plain","No open file");
		}
	}
}

void handleFileDelete() {
	if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	if (path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if (!SPIFFS.exists(path))
		return server.send(404, "text/plain", "FileNotFound");
	SPIFFS.remove(path);
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate() {
	if (server.args() == 0)
		return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	if (path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if (SPIFFS.exists(path))
		return server.send(500, "text/plain", "FILE EXISTS");
	File file = SPIFFS.open(path, "w");
	if (file)
		file.close();
	else
		return server.send(500, "text/plain", "CREATE FAILED");
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if (!server.hasArg("dir")) { server.send(500, "text/plain", "BAD ARGS"); return; }

	String path = server.arg("dir");
	File dir = SPIFFS.open(path);
    if(!dir.isDirectory()) { server.send(500, "text/plain", "NOT A DIRECTORY"); return; }
	path = String();

	String output = "[";
	while (File entry = dir.openNextFile()) {
		if (output != "[") output += ',';
		output += "{\"type\":\"";
		output += entry.isDirectory() ? "dir" : "file";
		output += "\",\"name\":\"";
		output += String(entry.name());
		output += "\"}";
		entry.close();
	}
	output += "]";
	server.send(200, "text/json", output);
}

void handleFileInfo() {
	if (!server.hasArg("dir")) { server.send(500, "text/plain", "BAD ARGS"); return; }

	String path = server.arg("dir");
	File dir = SPIFFS.open(path);
    if(!dir.isDirectory()) { server.send(500, "text/plain", "NOT A DIRECTORY"); return; }
	path = String();

	String output = "{ \"files\" : [";
	while (File entry = dir.openNextFile()) {
		if (output != "{ \"files\" : [") output += ',';
		output += "{\"type\":\"";
		output += entry.isDirectory() ? "dir" : "file";
		output += "\",\"name\":\"";
		output += String(entry.name());
		output += "\"}";
		entry.close();
	}
	output += "], \"total\": ";
    output += String(SPIFFS.totalBytes());
    output += ", \"used\": " ;
    output += String(SPIFFS.usedBytes());
    output += "}";
	server.send(200, "text/json", output);
}


void setupServer() {
	server.on("/list", HTTP_GET, handleFileList);
    server.on("/info", HTTP_GET, handleFileInfo);
	//load editor
	server.on("/edit", HTTP_GET, []() {
		if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
	});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, []() { server.send(500, "text/plain", "Upload not handled by callback"); }, handleFileUpload);


	server.on("/reboot", HTTP_ANY, [](){
		server.send(204, "text/plain", "");
		ESP.restart();
	});

    server.on("/show", HTTP_ANY, [](){
        if (!server.hasArg("file")) { server.send(500, "text/plain", "BAD ARGS"); return; }
        Serial.println("/show");
		server.send(200, "text/plain", "OK");
        showImage(server.arg("file"));
	});


	server.onNotFound([]() {
		if(!handleFileRead(server.uri())) 
            server.send(404, "text/plain", "FileNotFound");
    });

	server.begin();
}