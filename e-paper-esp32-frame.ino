/**
    @filename   :   EPD_7in3f.ino
    @brief      :   EPD_7in3 e-paper F display demo
    @author     :   Waveshare

    Copyright (C) Waveshare     10 21 2022

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documnetation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to  whom the Software is
   furished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include "imagedata.h"
#include "epd7in3f.h"
#include <Preferences.h>
#include <algorithm>
#include <vector>

Preferences preferences;

// #include "LittleFS.h"

Epd epd;
unsigned long delta;
#define SD_CS_PIN 22 // Change this to match your SD card CS pin!

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }
SPIClass vspi(VSPI);

uint8_t colorPallete[7*3] = {
	0, 0, 0,
	255, 255, 255,
	67, 138, 28,
	100, 64, 255,
	191, 0, 0,
	255, 243, 56,
	232, 126, 0,
};

uint8_t newColorPallete[7*3] = {
0, 0, 0,
	255, 255, 255,
	67, 170, 28,
	36, 36, 185,
	165, 0, 0,
	255, 243, 56,
	232, 126, 0,
  };

//uint8_t output_buffer[EPD_WIDTH * EPD_HEIGHT / 4];

// These read 16- and 32-bit types from the SD card file.
  // BMP data is stored little-endian, Arduino is little-endian too.
  // May need to reverse subscript order if porting elsewhere.

  uint16_t read16(fs::File &f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
  }

  uint32_t read32(fs::File &f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
  }

void setup() {
    delta = millis();

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);
    
    delay(1000);
    Serial.begin(115200);
    preferences.begin("e-paper", false);
    delay(1000);
    // unsigned int counter = preferences.getUInt("imageIndex", 0);
      
    // // Increase counter by 1
    // counter++;

    // // Print the counter to Serial Monitor
    // Serial.printf("Current counter value: %u\n", counter);

    // // Store the counter to the Preferences
    // preferences.putUInt("imageIndex", counter);



    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
      Serial.println("Woke up from deep sleep due to timer.");
    } else {
      Serial.println("Did not wake up from deep sleep.");
    }
    delay(1000);
    // SD.begin(SD_CS_PIN, vspi);
    while(!SD.begin(SD_CS_PIN, vspi)){
      Serial.println("Card Mount Failed");
      ESP.restart();
    }
    delay(1000);

    // testTXT();

    if (epd.Init() != 0) {
        Serial.println("eP init F");
        return;
    }else{
      Serial.println("eP init no F");
    }
    // epd.Clear(EPD_7IN3F_WHITE);
    //drawBmp("/bild.bmp");
    // drawBmp("/duett.bmp");
    // drawBmp("/bunt.bmp");
    // drawBmp("/lor_party.bmp");

    checkSDFiles();
    String file = getNextFile();
    drawBmp(file.c_str());

    //Serial.print("eP Clr\r\n ");
    // epd.Clear(EPD_7IN3F_WHITE);
    // epd.Clear(EPD_7IN3F_CLEAN);


    // Serial.print("Show pic\r\n ");
    // epd.EPD_7IN3F_Display(gImage_7in3f);
    // epd.EPD_7IN3F_Display_part(Image7color, 0, 0, 800, 480);
    // drawBmp("/bild.bmp");
    // //epd.EPD_7IN3F_Display_part(output_buffer, 0, 120, 800, 240);
    // delay(5000);
    // //Serial.print("draw 7 color block\r\n ");
    // epd.EPD_7IN3F_Show7Block();
    // delay(2000);
    // Serial.print("Done\r\n ");
    
    // epd.Sleep();
}

void loop() {
    hibernate();
    // put your main code here, to run repeatedly:
}
void hibernate() {
    Serial.println("start sleep");

    unsigned long totalRuntime = millis() - delta;

    //Deepsleep for one minut minus totalRuntime
    esp_deep_sleep((24 * 60 + 10) * 60e6 - totalRuntime * 1000);
    // esp_deep_sleep(60e6 - totalRuntime * 1000);

    Serial.println("end sleep");

}
void checkSDFiles(){
  
  Serial.println("Checking info.txt File");
  File infoFile = SD.open("/info.txt");  // Try to open info.txt

  if (!infoFile) {
    Serial.println("info.txt not found");
    return;  // Exit if file not found
  }

  String infoText = "";
  while (infoFile.available()) {
    infoText += (char)infoFile.read();  // Read file content into a String
  }
  infoFile.close();  // Close the file after reading

  Serial.println("Content of info.txt: " + infoText);

  if(preferences.getString("checker", "") != infoText){
    Serial.println("Check SD File");
    File root = SD.open("/");  // open SD card main root
    u_int16_t fileCount = 0;  
    String fileString = "";
    std::vector<String> bmpFiles;

    while (true) {
      File entry =  root.openNextFile();  // open file

      if (!entry) {
        Serial.println("No more files");
        // no more files
        root.close();
        break;
      }
  
      uint8_t nameSize = String(entry.name()).length();  // get file name size
      String str1 = String(entry.name()).substring( nameSize - 4 );  // save the last 4 characters (file extension)
  
      if ( str1.equalsIgnoreCase(".bmp") ) {  // if the file has '.bmp' extension
        bmpFiles.push_back(entry.name());
        Serial.println(String(entry.name()));  // print the file name
      }
  
      entry.close();  // close the file
    }
    std::sort(bmpFiles.begin(), bmpFiles.end());

    for (int i = 0; i < bmpFiles.size(); i++) {
      fileString += bmpFiles[i] + ",";  // add file name to fileString
    }
    preferences.putUInt("fileCount", bmpFiles.size());
    preferences.putUInt("imageIndex", 0);
    preferences.putString("checker", infoText);

    //save the fileString to a txt file
    File file = SD.open("/fileString.txt", FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file for writing");
      return;
    }
    file.print(fileString);
    file.close();
  }
}
String getNextFile(){
  unsigned int fileCount = preferences.getUInt("fileCount", 0);
  unsigned int imageIndex = preferences.getUInt("imageIndex", 0);

  unsigned int temp = imageIndex; 
  if(imageIndex >= fileCount - 1){
    imageIndex = 0;
  }else{
    imageIndex++;
  }
  preferences.putUInt("imageIndex", imageIndex);

  //read fileString from txt file
  File file = SD.open("/fileString.txt");
  if(!file){
    Serial.println("Failed to open file for reading");
    return "";
  }
  String fileString = "";
  while(file.available()){
    fileString += (char)file.read();
  }
  file.close();
  // Serial.println("fileString: " + fileString);

  //get the next file from the fileString based on imageIndex
  int start = 0;
  int end = fileString.indexOf(",", start);
  for(int i = 0; i < temp; i++){
    start = end + 1;
    end = fileString.indexOf(",", start);
  }
  String nextFile = fileString.substring(start, end);
  Serial.println("nextFile: " + nextFile);

  return "/" + nextFile;
}

bool drawBmp(const char *filename) {
    fs::File bmpFS;
    Serial.println("Drawing bitmap file: " + String(filename));
    // Open requested file on SD card
    bmpFS =  SD.open(filename);
    uint32_t seekOffset, headerSize, paletteSize = 0;
    int16_t row;
    uint8_t r, g, b;
    uint16_t bitDepth;
    uint16_t magic = read16(bmpFS);
    if (magic != ('B' | ('M' << 8))) { // File not found or not a BMP
      Serial.println(F("BMP not found!"));
      bmpFS.close();
      return false;
    }

    read32(bmpFS); // filesize in bytes
    read32(bmpFS); // reserved
    seekOffset = read32(bmpFS); // start of bitmap
    headerSize = read32(bmpFS); // header size
    uint32_t w = read32(bmpFS); // width
    uint32_t h = read32(bmpFS); // height
    read16(bmpFS); // color planes (must be 1)
    bitDepth = read16(bmpFS);

    if (read32(bmpFS) != 0 || (bitDepth != 24 && bitDepth != 1 && bitDepth != 4 && bitDepth != 8)) {
      Serial.println(F("BMP format not recognized."));
      bmpFS.close();
      return false;
    }

    uint32_t palette[256];
    if (bitDepth <= 8) // 1,4,8 bit bitmap: read color palette
    {
      read32(bmpFS); read32(bmpFS); read32(bmpFS); // size, w resolution, h resolution
      paletteSize = read32(bmpFS);
      if (paletteSize == 0) paletteSize = bitDepth * bitDepth; //if 0, size is 2^bitDepth
      bmpFS.seek(14 + headerSize); // start of color palette
      for (uint16_t i = 0; i < paletteSize; i++) {
        palette[i] = read32(bmpFS);
      }
    }

    // draw img that is shorter than 240pix into the center
    uint16_t x = (width() - w) /2;
    uint16_t y = (height() - h) /2;

    bmpFS.seek(seekOffset);

    uint32_t lineSize = ((bitDepth * w +31) >> 5) * 4;
    uint8_t lineBuffer[lineSize];
    uint8_t nextLineBuffer[lineSize];
    // bmpFS.read(nextLineBuffer, sizeof(nextLineBuffer));

    epd.SendCommand(0x10); // start data frame

    epd.EPD_7IN3F_Draw_Blank(y, width(), EPD_7IN3F_WHITE); // fill area on top of pic white
    
    // uint8_t colorArray[2][10][3];

    //  for (row = 1; row >= 0; row--) {
    //     bmpFS.read(lineBuffer, sizeof(lineBuffer));
    //     uint8_t*  bptr = lineBuffer;

    //     for (uint16_t col = 0; col < 10; col++) {
    //       b = *bptr++;
    //       g = *bptr++;
    //       r = *bptr++;

    //       colorArray[row][col][0] = r;
    //       colorArray[row][col][1] = g;
    //       colorArray[row][col][2] = b;
    //     }
    // }
    // Serial.println("Color Array erstellt");

    // row is decremented as the BMP image is drawn bottom up
    bmpFS.read(lineBuffer, sizeof(lineBuffer));
    //reverse linBuffer with the alorithm library 
    std::reverse(lineBuffer, lineBuffer + sizeof(lineBuffer));

    for (row = h-1; row >= 0; row--) {
      // Serial.print("row: "+String(row));
      epd.EPD_7IN3F_Draw_Blank(1, x, EPD_7IN3F_WHITE); // fill area on the left of pic white
      if(row != 0){
        bmpFS.read(nextLineBuffer, sizeof(nextLineBuffer));
        std::reverse(nextLineBuffer, nextLineBuffer + sizeof(nextLineBuffer));
      }
      uint8_t*  bptr = lineBuffer;
      uint8_t*  bnptr = nextLineBuffer;

      
      uint8_t output = 0;
      // Convert 24 to 16 bit colors while copying to output buffer.
      for (uint16_t col = 0; col < w; col++)
      {
        if (bitDepth == 24) {
          r = *bptr++;
          g = *bptr++;
          b = *bptr++;
          bnptr += 3;
        } else {
          uint32_t c = 0;
          if (bitDepth == 8) {
            c = palette[*bptr++];
          }
          else if (bitDepth == 4) {
            c = palette[(*bptr >> ((col & 0x01)?0:4)) & 0x0F];
            if (col & 0x01) bptr++;
          }
          else { // bitDepth == 1
            c = palette[(*bptr >> (7 - (col & 0x07))) & 0x01];
            if ((col & 0x07) == 0x07) bptr++;
          }
          b = c; g = c >> 8; r = c >> 16;
        }
        uint8_t color;
        int indexColor;
        int errorR;
        int errorG;
        int errorB;

        // if(col > w/2){
        //   indexColor = depaletteNew(r, g, b);
        //   errorR = r - newColorPallete[indexColor*3+0];
        //   errorG = g - newColorPallete[indexColor*3+1];
        //   errorB = b - newColorPallete[indexColor*3+2];
        // }else{
          indexColor = depalette(r, g, b);
          errorR = r - colorPallete[indexColor*3+0];
          errorG = g - colorPallete[indexColor*3+1];
          errorB = b - colorPallete[indexColor*3+2];
        // }

      
        
      if(col < w-1){
          bptr[0] = constrain(bptr[0] + (7*errorR/16), 0, 255);
          bptr[1] = constrain(bptr[1] + (7*errorG/16), 0, 255);
          bptr[2] = constrain(bptr[2] + (7*errorB/16), 0, 255);
      }

      if(row > 0){
          if(col > 0){
              bnptr[-4] = constrain(bnptr[-4] + (3*errorR/16), 0, 255);
              bnptr[-5] = constrain(bnptr[-5] + (3*errorG/16), 0, 255);
              bnptr[-6] = constrain(bnptr[-6] + (3*errorB/16), 0, 255);
          }
          bnptr[-1] = constrain(bnptr[-1] + (5*errorR/16), 0, 255);
          bnptr[-2] = constrain(bnptr[-2] + (5*errorG/16), 0, 255);
          bnptr[-3] = constrain(bnptr[-3] + (5*errorB/16), 0, 255);

          if(col < w-1){
              bnptr[0] = constrain(bnptr[0] + (1*errorR/16), 0, 255);
              bnptr[1] = constrain(bnptr[1] + (1*errorG/16), 0, 255);
              bnptr[2] = constrain(bnptr[2] + (1*errorB/16), 0, 255);
          }
      }

        switch (indexColor)
        {
        case 0:
          color = EPD_7IN3F_BLACK;
          break;
        case 1:
          color = EPD_7IN3F_WHITE;
          break;
        case 2:
          color = EPD_7IN3F_GREEN;
          break;
        case 3:
          color = EPD_7IN3F_BLUE;
          break;
        case 4:
          color = EPD_7IN3F_RED;
          break;
        case 5:
          color = EPD_7IN3F_YELLOW;
          break;
        case 6:
          color = EPD_7IN3F_ORANGE;
          break;
        case 7:
          color = EPD_7IN3F_WHITE;
          break;
        
       
        }
        // uint8_t color = EPD_7IN3F_WHITE;
        // if (r <= 128 && g <= 128 && b <= 128) {
        //   color = EPD_7IN3F_BLACK;
        // } else if (r > 200 && g > 200 && b > 200) {
        //   color = EPD_7IN3F_WHITE;
        // } else if (b > 128) {
        //   color = EPD_7IN3F_BLUE;
        // } else if (g > 128 && r <= 128) {
        //   color = EPD_7IN3F_GREEN;
        // } else {
        //   color = (g > 140) ? EPD_7IN3F_YELLOW : (g > 64) ? EPD_7IN3F_ORANGE : EPD_7IN3F_RED;
        // }
        uint32_t buf_location = (row*(width()/2)+col/2);
        if (col & 0x01) {
          output |= color;
          epd.SendData(output);
        } else {
          output = color << 4;
        }
      }
      epd.EPD_7IN3F_Draw_Blank(1, x, EPD_7IN3F_WHITE); // fill area on the right of pic white
      memcpy(lineBuffer, nextLineBuffer, sizeof(lineBuffer));

    }

    epd.EPD_7IN3F_Draw_Blank(y, width(), EPD_7IN3F_WHITE); // fill area below the pic white

    bmpFS.close();
    epd.TurnOnDisplay();
    return true;
  }

int depalette( uint8_t r, uint8_t g, uint8_t b ){
	int p;
	int mindiff = 100000000;
	int bestc = 0;
	for( p = 0; p < sizeof(colorPallete)/3; p++ )
	{
		int diffr = ((int)r) - ((int)colorPallete[p*3+0]);
		int diffg = ((int)g) - ((int)colorPallete[p*3+1]);
		int diffb = ((int)b) - ((int)colorPallete[p*3+2]);
		int diff = (diffr*diffr) + (diffg*diffg) + (diffb * diffb);
		if( diff < mindiff )
		{
			mindiff = diff;
			bestc = p;
		}
	}
	return bestc;
}
int depaletteNew( uint8_t r, uint8_t g, uint8_t b ){
	int p;
	int mindiff = 100000000;
	int bestc = 0;
	for( p = 0; p < sizeof(newColorPallete)/3; p++ )
	{
		int diffr = ((int)r) - ((int)newColorPallete[p*3+0]);
		int diffg = ((int)g) - ((int)newColorPallete[p*3+1]);
		int diffb = ((int)b) - ((int)newColorPallete[p*3+2]);
		int diff = (diffr*diffr) + (diffg*diffg) + (diffb * diffb);
		if( diff < mindiff )
		{
			mindiff = diff;
			bestc = p;
		}
	}
	return bestc;
}
