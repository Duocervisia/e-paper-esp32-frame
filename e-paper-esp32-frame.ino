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

// #include "LittleFS.h"

Epd epd;
#define SD_CS_PIN 5 // Change this to match your SD card CS pin!

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }
SPIClass vspi(VSPI);

uint8_t colorPallete[8*3] = {
	0, 0, 0,
	255, 255, 255,
	67, 138, 28,
	100, 64, 255,
	191, 0, 0,
	255, 243, 56,
	232, 126, 0,
	194 ,164 , 244 
};

//uint8_t output_buffer[EPD_WIDTH * EPD_HEIGHT / 4];

// These read 16- and 32-bit types from the SD card file.
  // BMP data is stored little-endian, Arduino is little-endian too.
  // May need to reverse subscript order if porting elsewhere.

  uint16_t read16(fs::File &f) {
    uint16_t result;
    Serial.println("read16 1");
    ((uint8_t *)&result)[0] = f.read(); // LSB
    Serial.println("read16 2");
    ((uint8_t *)&result)[1] = f.read(); // MSB
    Serial.println("read16 3");
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

bool drawBmp(const char *filename) {
    fs::File bmpFS;
    Serial.println("Drawing bitmap file: " + String(filename));
    // Open requested file on SD card
    bmpFS =  SD.open(filename);
    Serial.println("File Opened");

    uint32_t seekOffset, headerSize, paletteSize = 0;
    int16_t row;
    uint8_t r, g, b;
    uint16_t bitDepth;
    Serial.println("Reading file");
    uint16_t magic = read16(bmpFS);
    Serial.println("Magic: " + String(magic, HEX));
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
    Serial.println("Bit Depth: " + String(bitDepth));

    if (read32(bmpFS) != 0 || (bitDepth != 24 && bitDepth != 1 && bitDepth != 4 && bitDepth != 8)) {
      Serial.println(F("BMP format not recognized."));
      bmpFS.close();
      return false;
    }

    uint32_t palette[256];
    Serial.println("Palettes:");
    if (bitDepth <= 8) // 1,4,8 bit bitmap: read color palette
    {
      read32(bmpFS); read32(bmpFS); read32(bmpFS); // size, w resolution, h resolution
      paletteSize = read32(bmpFS);
      if (paletteSize == 0) paletteSize = bitDepth * bitDepth; //if 0, size is 2^bitDepth
      bmpFS.seek(14 + headerSize); // start of color palette
      for (uint16_t i = 0; i < paletteSize; i++) {
        palette[i] = read32(bmpFS);
        Serial.println(palette[i], HEX);
      }
    }

    // draw img that is shorter than 240pix into the center
    uint16_t x = (width() - w) /2;
    uint16_t y = (height() - h) /2;

    bmpFS.seek(seekOffset);

    uint32_t lineSize = ((bitDepth * w +31) >> 5) * 4;
    uint8_t lineBuffer[lineSize];
    uint8_t nextLineBuffer[lineSize];
    bmpFS.read(nextLineBuffer, sizeof(nextLineBuffer));

    epd.SendCommand(0x10); // start data frame

    epd.EPD_7IN3F_Draw_Blank(y, width(), EPD_7IN3F_WHITE); // fill area on top of pic white
    
    // row is decremented as the BMP image is drawn bottom up
    for (row = h-1; row >= 0; row--) {
      // Serial.print("row: "+String(row));
      epd.EPD_7IN3F_Draw_Blank(1, x, EPD_7IN3F_WHITE); // fill area on the left of pic white
      bmpFS.read(lineBuffer, sizeof(lineBuffer));

      uint8_t*  bptr = lineBuffer;
      uint8_t*  bnptr = nextLineBuffer;

      
      uint8_t output = 0;
      // Convert 24 to 16 bit colors while copying to output buffer.
      for (uint16_t col = 0; col < w; col++)
      {
        if (bitDepth == 24) {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
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
        int indexColor = depalette(r, g, b);
        int errorR = r - colorPallete[indexColor*3+0];
        int errorG = g - colorPallete[indexColor*3+1];
        int errorB = b - colorPallete[indexColor*3+2];
        
        if(col < w-1){
          bptr[0] = bptr[0] + (7*errorR/16);
          bptr[1] = bptr[1] + (7*errorG/16);
          bptr[2] = bptr[2] + (7*errorB/16);
        }
        if(row > 0){
          if(col > 0){
            bnptr[-1] = bnptr[-1] + (3*errorR/16);
            bnptr[-2] = bnptr[-2] + (3*errorG/16);
            bnptr[-3] = bnptr[-3] + (3*errorB/16);
          }
          bnptr[0] = bnptr[0] + (5*errorR/16);
          bnptr[1] = bnptr[1] + (5*errorR/16);
          bnptr[2] = bnptr[2] + (5*errorR/16);

          if(col < w-1){
            bnptr[3] = bnptr[3] + (1*errorR/16);
            bnptr[4] = bnptr[4] + (1*errorR/16);
            bnptr[5] = bnptr[5] + (1*errorR/16);
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

void setup() {
    // put your setup code here, to run once:
    // LittleFS.begin();
    delay(1000);
    Serial.begin(115200);
    delay(1000);
    Serial.print("MOSI: ");
    Serial.println(MOSI);
    Serial.print("MISO: ");
    Serial.println(MISO);
    Serial.print("SCK: ");
    Serial.println(SCK);
    Serial.print("SS: ");
    Serial.println(SS);  

    // if(!SD.begin(SD_CS_PIN, hspi)){
    if(!SD.begin(SD_CS_PIN, vspi)){

      Serial.println("Card Mount Failed");
      return;
    }
    delay(1000);

    // testTXT();

    Serial.print("dfgdfgegherth");
    if (epd.Init() != 0) {
        Serial.print("eP init F");
        return;
    }else{
      Serial.print("eP init no F");
    }
    epd.Clear(EPD_7IN3F_WHITE);
    drawBmp("/goggles.bmp");

    // SDTest();

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
    // //epd.EPD_7IN3F_Show7Block();
    // delay(2000);
    // Serial.print("Done\r\n ");
    
    // epd.Sleep();
}

void loop() {
    Serial.println("in loop");
    delay(1000);

    // put your main code here, to run repeatedly:

}
void SDTest(){
  Serial.println("SD Test");
  File root = SD.open("/");  // open SD card main root
  Serial.println("SD Files:" + String(root.name())); 

  while (true) {
    File entry =  root.openNextFile();  // open file
    Serial.println(entry.name());  // print the file name

    if (!entry) {
      Serial.println("No more files");
      // no more files
      root.close();
      return;
    }
 
    uint8_t nameSize = String(entry.name()).length();  // get file name size
    String str1 = String(entry.name()).substring( nameSize - 4 );  // save the last 4 characters (file extension)
 
    if ( str1.equalsIgnoreCase(".bmp") )  // if the file has '.bmp' extension
      Serial.println(entry.name());  // print the file name
 
    entry.close();  // close the file
  }
}
void testTXT(){
  Serial.println("Test TXT");
  File file = SD.open("/test.txt");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}
