#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include "epd7in3combined.h"
#include <Preferences.h>
#include <algorithm>
#include <vector>
#include "time_utils.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"

//This is the pin for the transistor that powers the external components
#define TRANSISTOR_PIN 26

Preferences preferences;

Epd epd;
unsigned long delta; // Variable to store the time it took to update the display for deep sleep calculations
unsigned long deltaSinceTimeObtain; // Variable to store the time it took to update the display since the time was obtained for deep sleep calculations

#define SD_CS_PIN 22

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }

SPIClass vspi(VSPI); // VSPI for SD card


#if defined(DISPLAY_TYPE_E)
  // Color pallete for dithering. These are specific to the 7in3e waveshare display.
  uint8_t colorPallete[6*3] = {
    0, 0, 0,
    255, 255, 255,
    255, 255, 0,
    255, 0, 0,
    0, 0, 255,
    0, 255, 0
  };
#elif defined(DISPLAY_TYPE_F)
  // Color pallete for dithering. These are specific to the 7in3f waveshare display.
  uint8_t colorPallete[7*3] = {
    0, 0, 0,
    255, 255, 255,
    67, 138, 28,
    100, 64, 255,
    191, 0, 0,
    255, 243, 56,
    232, 126, 0,
  };
#endif

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

float readBattery() {
  uint32_t value = 0;
  int rounds = 11;
  esp_adc_cal_characteristics_t adc_chars;

  //battery voltage divided by 2 can be measured at GPIO34, which equals ADC1_CHANNEL6
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  switch(esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars)) {
    case ESP_ADC_CAL_VAL_EFUSE_TP:
      Serial.println("Characterized using Two Point Value");
      break;
    case ESP_ADC_CAL_VAL_EFUSE_VREF:
      Serial.printf("Characterized using eFuse Vref (%d mV)\r\n", adc_chars.vref);
      break;
    default:
      Serial.printf("Characterized using Default Vref (%d mV)\r\n", 1100);
  }

  //to avoid noise, sample the pin several times and average the result
  for(int i=1; i<=rounds; i++) {
    value += adc1_get_raw(ADC1_CHANNEL_6);
  }
  value /= (uint32_t)rounds;

  //due to the voltage divider (1M+1M) values must be multiplied by 2
  //and convert mV to V
  return esp_adc_cal_raw_to_voltage(value, &adc_chars)*2.0/1000.0;
}

void setup() {
  setCpuFrequencyMhz(80); // or 40, 20, etc. (default is 240)
  Serial.begin(115200);
  delta = millis();
  
  preferences.begin("e-paper", false);

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up from deep sleep due to timer.");
  } else {
    Serial.println("Did not wake up from deep sleep.");
  }

  // Release hold on TRANSISTOR_PIN after deep sleep so we can control it again
  gpio_hold_dis((gpio_num_t)TRANSISTOR_PIN);
  // Turn on the transistor to power the external components
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, LOW); 
  delay(10);

  // Initialize the SD card
  while(!SD.begin(SD_CS_PIN, vspi)){
    Serial.println("Card Mount Failed");
    hibernate();
  }

  // Initialize Wifi and get the time
  initializeWifi();
  initializeTime();

  deltaSinceTimeObtain = millis();

  // Initialize the e-paper display
  if (epd.Init() != 0) {
    Serial.println("eP init F");
    hibernate();
  }else{
    Serial.println("eP init no F");
  }

  checkSDFiles(); //Check if the SD files have changed and update the preferences if needed

  String file = getNextFile(); // Get the next file to display

  drawBmp(file.c_str()); // Display the file

  digitalWrite(TRANSISTOR_PIN, HIGH); // Turn off external components

  preferences.end();
}

void loop() {
    hibernate();
}

void hibernate() {
    Serial.println("start sleep");

    // Ensure TRANSISTOR_PIN stays HIGH during deep sleep
    gpio_hold_en((gpio_num_t)TRANSISTOR_PIN);
    gpio_deep_sleep_hold_en();

    esp_deep_sleep(static_cast<uint64_t>(getSecondsTillNextImage(delta, deltaSinceTimeObtain))* 1e6);
    // sleep for 5 seconds debug
    // esp_deep_sleep(5* 1e6);
}

// Function to check if the SD files have changed and update the preferences if needed
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

  // Check if the info.txt content has changed, if so update the preferences
  if(preferences.getString("checker", "") != infoText){
    Serial.println("Check SD File");
    File root = SD.open("/");
    u_int16_t fileCount = 0;  
    String fileString = "";
    std::vector<String> bmpFiles;

    // Get every filename in the root directory and save the ones with '.bmp' extension in the bmpFiles vector
    while (true) {
      File entry =  root.openNextFile(); 

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

    // Create a string with all the file names separated by commas
    for (int i = 0; i < bmpFiles.size(); i++) {
      fileString += bmpFiles[i] + ",";  // add file name to fileString
    }

    // Reset preferences values
    preferences.putUInt("fileCount", bmpFiles.size());
    preferences.putUInt("imageIndex", 0);
    preferences.putString("checker", infoText);

    // Save the fileString to a txt file to parse the files later
    File file = SD.open("/fileString.txt", FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file for writing");
      return;
    }
    file.print(fileString);
    file.close();
  }
}

// Function to get the next file to display
String getNextFile(){
  // Read fileString from txt file
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

  String date;

  // If time is working, get the date from the timeinfo struct to display a image set for that day
  if(timeWorking){

    Serial.println("timeinfo.tm_hour: " + String(timeinfo.tm_hour));
    Serial.println("timeinfo.tm_min: " + String(timeinfo.tm_min));
    if (timeinfo.tm_hour < 9) {
      Serial.println("Getting the date of the previous day");
      // Get the date of the previous day
      time_t previousDay = mktime(&timeinfo) - 24 * 60 * 60;
      struct tm* previousDayInfo = localtime(&previousDay);
      date = String(previousDayInfo->tm_mday < 10 ? "0" : "") + String(previousDayInfo->tm_mday) + "." + String((previousDayInfo->tm_mon + 1) < 10 ? "0" : "") + String(previousDayInfo->tm_mon + 1);
    } else {
      date = String(timeinfo.tm_mday < 10 ? "0" : "") + String(timeinfo.tm_mday) + "." + String((timeinfo.tm_mon + 1) < 10 ? "0" : "") + String(timeinfo.tm_mon + 1);
    }
  // If time is not working, get the date from the preferences to display the next image in the list. The date is updated every time the device obtains the time.
  }else{
    date = preferences.getString("date", "01.01");
    int day = date.substring(0, 2).toInt();
    int month = date.substring(3, 5).toInt();

    //Added for leap year (Years don't matter for this project, years are not checked)
    int year = 2000;
    struct tm timeinfoTemp = {0};
    timeinfoTemp.tm_year = year - 1900;
    timeinfoTemp.tm_mon = month - 1; // tm_mon is 0-based
    timeinfoTemp.tm_mday = day;

    // Add one day
    timeinfoTemp.tm_mday += 1;

    // Normalize the time structure (this will handle month/year overflow)
    mktime(&timeinfoTemp);

    // Format the new date back to a string
    char newDate[6];
    snprintf(newDate, sizeof(newDate), "%02d.%02d", timeinfoTemp.tm_mday, timeinfoTemp.tm_mon + 1);

    // Save the new date
    date = String(newDate);
    Serial.println("New date: " + date);
  }
  Serial.println("date: " + date);
  preferences.putString("date", date);
  int start = 0;
  int end = fileString.indexOf(",", start);
  String nextFile = "";

  // Get the next file from the fileString based on the date
  while (true) {
    String currentFile = fileString.substring(start, end);
    Serial.println("currentFile: " + currentFile);
    Serial.println("currentFile.indexOf(date): " + currentFile.indexOf(date));
    if (currentFile.indexOf(date) != -1) {
      nextFile = currentFile;
      break;
    }
    start = end + 1;
    end = fileString.indexOf(",", start);
    if (end == -1) {
      break;
    }
  }

  if (nextFile != "") {
    return "/" + nextFile;
  }
  
  // If no file was found for the date, get the next file in the list based on the imageIndex
  unsigned int fileCount = preferences.getUInt("fileCount", 0);
  unsigned int imageIndex = preferences.getUInt("imageIndex", 0);

  unsigned int temp = imageIndex; 
  if(imageIndex >= fileCount - 1){
    imageIndex = 0;
  }else{
    imageIndex++;
  }
  preferences.putUInt("imageIndex", imageIndex);

  start = 0;
  end = fileString.indexOf(",", start);
  for(int i = 0; i < temp; i++){
    start = end + 1;
    end = fileString.indexOf(",", start);
  }
  nextFile = fileString.substring(start, end);
  Serial.println("nextFile: " + nextFile);

  return "/" + nextFile;
}

// Function to draw a BMP image on the e-paper display
bool drawBmp(const char *filename) {
  Serial.println("Drawing bitmap file: " + String(filename));
  fs::File bmpFS;
  bmpFS =  SD.open(filename); // Open requested file on SD card
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

  // Check if the BMP is valid
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

  // draw img that is shorter than display in the middle
  uint16_t x = (width() - w) /2;
  uint16_t y = (height() - h) /2;

  bmpFS.seek(seekOffset);

  uint32_t lineSize = ((bitDepth * w +31) >> 5) * 4;
  uint8_t lineBuffer[lineSize];
  uint8_t nextLineBuffer[lineSize];

  epd.SendCommand(0x10); // start data frame

  epd.EPD_7IN3F_Draw_Blank(y, width(), EPD_WHITE); // fill area on top of pic white
  
  // row is decremented as the BMP image is drawn bottom up
  bmpFS.read(lineBuffer, sizeof(lineBuffer));
  //reverse linBuffer with the alorithm library 
  std::reverse(lineBuffer, lineBuffer + sizeof(lineBuffer));

  float batteryVolts = readBattery();
  Serial.println("Battery voltage: " + String(batteryVolts) + "V");

  for (row = h-1; row >= 0; row--) {
    epd.EPD_7IN3F_Draw_Blank(1, x, EPD_WHITE); // fill area on the left of pic white
    
    if(row != 0){
      bmpFS.read(nextLineBuffer, sizeof(nextLineBuffer));
      std::reverse(nextLineBuffer, nextLineBuffer + sizeof(nextLineBuffer));
    }
    uint8_t*  bptr = lineBuffer;
    uint8_t*  bnptr = nextLineBuffer;
    
    uint8_t output = 0;

    for (uint16_t col = 0; col < w; col++)
    {
      // Get r g b values for the next pixel
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

      // Floyd-Steinberg dithering is used to dither the image
      uint8_t color;
      int indexColor;
      int errorR;
      int errorG;
      int errorB;
    
      indexColor = depalette(r, g, b); // Get the index of the color in the colorPallete
      errorR = r - colorPallete[indexColor*3+0];
      errorG = g - colorPallete[indexColor*3+1];
      errorB = b - colorPallete[indexColor*3+2];
      
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

      // Set the color based on the indexColor
      switch (indexColor){
        #if defined(DISPLAY_TYPE_E)
          case 0:
            color = EPD_7IN3E_BLACK;
            break;
          case 1:
            color = EPD_7IN3E_WHITE;
            break;
          case 2:
            color = EPD_7IN3E_YELLOW;
            break;
          case 3:
            color = EPD_7IN3E_RED;
            break;
          case 4:
            color = EPD_7IN3E_BLUE;
            break;
          case 5:
            color = EPD_7IN3E_GREEN;
            break;
        #elif defined(DISPLAY_TYPE_F)
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
        #endif
      }

      if (batteryVolts <= 3.3 && col <= 50 && row >= h-50){
        color = EPD_RED;
        if (batteryVolts < 3.1) {
          Serial.println("Battery critically low, hibernating...");

          //switch off everything that might consume power
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
          esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
          //esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);

          //disable all wakeup sources
          esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

          digitalWrite(2, LOW);
          esp_deep_sleep_start();

          Serial.println("This should never get printed");
          return false;
        }
      }

      // Vodoo magic i don't understand
      uint32_t buf_location = (row*(width()/2)+col/2);
      if (col & 0x01) {
        output |= color;
        epd.SendData(output);
      } else {
        output = color << 4;
      }
    }

    epd.EPD_7IN3F_Draw_Blank(1, x, EPD_WHITE); // fill area on the right of pic white
    memcpy(lineBuffer, nextLineBuffer, sizeof(lineBuffer));
  }

  epd.EPD_7IN3F_Draw_Blank(y, width(), EPD_WHITE); // fill area below the pic white

  bmpFS.close(); // Close the file
  epd.TurnOnDisplay(); // Turn on the display
  epd.Sleep(); // Put the display to sleep
  return true;
}

// Function to depalette the image
int depalette( uint8_t r, uint8_t g, uint8_t b ){
	int p;
	int mindiff = 100000000;
	int bestc = 0;

  // Find the color in the colorPallete that is closest to the r g b values
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