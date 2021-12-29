/****************************************************************************/
/*  ESP32 TTS library example for voicetext.jp By  Schmurtz                 */
/*                                                                          */
/*                                                                          */
/*  Released under GPL v2                                                   */
/*  Credits :                                                               */
/***************************************************************************

  ______    _____   _____    ____    ___  
 |  ____|  / ____| |  __ \  |___ \  |__ \ 
 | |__    | (___   | |__) |   __) |    ) |
 |  __|    \___ \  |  ___/   |__ <    / / 
 | |____   ____) | | |       ___) |  / /_ 
 |______| |_____/  |_|      |____/  |____|
 
+-----------------+-----------------+--------------+--------------------+
|   audio pins    |  External DAC   | Internal DAC |       No DAC       |
+-----------------+-----------------+--------------+--------------------+
| DAC - LCK(LRC)  | GPIO25          | NA           | NA                 |
| DAC - CK(BCLK)  | GPIO26          | NA           | NA                 |
| DAC - I2So(DIN) | GPIO22          | NA           | NA                 |
| speaker L       | NA (on the DAC) | GPIO25       | GPIO22 (L&R mixed) |
| speaker R       | NA (on the DAC) | GPIO26       | GPIO22 (L&R mixed) |
+-----------------+-----------------+--------------+--------------------+

  ______    _____   _____     ___    ___      __      __  
 |  ____|  / ____| |  __ \   / _ \  |__ \    / /     / /  
 | |__    | (___   | |__) | | (_) |    ) |  / /_    / /_  
 |  __|    \___ \  |  ___/   > _ <    / /  | '_ \  | '_ \ 
 | |____   ____) | | |      | (_) |  / /_  | (_) | | (_) |
 |______| |_____/  |_|       \___/  |____|  \___/   \___/ 


+-----------------+-----------------+------------------------+
|   audio pins    |  External DAC   |         No DAC         |
+-----------------+-----------------+------------------------+
| DAC - LCK(LRC)  | GPIO2 (D4)      | NA                     |
| DAC - CK(BCLK)  | GPIO15 (D8)     | NA                     |
| DAC - I2So(DIN) | GPIO3 (RX)      | NA                     |
| speaker L       | NA (on the DAC) | GPIO3 (RX) (L&R mixed) |
| speaker R       | NA (on the DAC) | GPIO3 (RX) (L&R mixed) |
+-----------------+-----------------+------------------------+
To Upload to an ESP8266 module or board:

  - Set CPU Frequency to 160MHz ( Arduino IDE: Tools > CPU Frequency )
  - Set IwIP to V2 Higher Bandwidth ( Arduino IDE: Tools > IwIP Variant )
  - Press "Upload"
*/


#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceVoiceTextStream.h"


// Please select one of these options
//#define USE_NO_DAC           // uncomment to use no DAC, using software-simulated delta-sigma DAC
//#define USE_EXTERNAL_DAC     // uncomment to use external I2S DAC 
#define USE_INTERNAL_DAC     // uncomment to use the internal DAC of the ESP32 (not available on ESP8266)

#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif


#ifdef USE_NO_DAC
    #include "AudioOutputI2SNoDAC.h"
#else
    #include "AudioOutputI2S.h"
#endif


#ifdef USE_NO_DAC
    AudioOutputI2SNoDAC     *out = NULL;
#else
    AudioOutputI2S          *out = NULL;
#endif


const char *SSID = "YOUR_WIFI_SSID";
const char *PASSWORD = "YOUR_WIFI_PASSWORD";



char *text1 = "こんにちは、世界！";
char *tts_parms1 ="&emotion_level=2&emotion=happiness&format=mp3&speaker=hikari&volume=200&speed=120&pitch=130";
char *tts_parms2 ="&emotion_level=2&emotion=happiness&format=mp3&speaker=takeru&volume=200&speed=100&pitch=130";
char *tts_parms3 ="&emotion_level=4&emotion=anger&format=mp3&speaker=bear&volume=200&speed=120&pitch=100";


AudioGeneratorMP3 *mp3;
AudioFileSourceVoiceTextStream *file;
AudioFileSourceBuffer *buff;

const int preallocateBufferSize = 40*1024;
uint8_t *preallocateBuffer = NULL;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.)
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}



void VoiceText_tts(char *text,char *tts_parms) {
    file = new AudioFileSourceVoiceTextStream( text, tts_parms);
   // file->setApiKey(ApiKey);
    buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
    delay(100);
    mp3->begin(buff, out);

}

void setup()
{
  Serial.begin(115200);
  preallocateBuffer = (uint8_t*)ps_malloc(preallocateBufferSize);
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nConnected");
  
  audioLogger = &Serial;

#ifdef USE_INTERNAL_DAC
    out = new AudioOutputI2S(0, 1);  //use the internal DAC : channel 1 (gpio25) , channel 2 (gpio26) 
    Serial.println(F("Using the internal DAC of the ESP32 : \r\n speaker L -> GPIO25 \r\n speaker R -> GPIO26"));
#elif USE_EXTERNAL_DAC
    out = new AudioOutputI2S();
    #ifdef ESP32
      Serial.println(F("Using I2S output on ESP32 : please connect your DAC to pins : ")));
      Serial.println(F("LCK(LRC) -> GPIO25  \r\n BCK(BCLK) -> GPIO26 \r\n I2So(DIN) -> GPIO22"));
    #else    // we are on ESP8266
      Serial.println(F("Using I2S output on ESP8266 : please connect your DAC to pins : "));
      Serial.println(F("LCK(LRC) -> GPIO2 (D4) \r\n BCK(BCLK) -> GPIO15 (D8) \r\n I2So(DIN) -> GPIO3 (RX)"));
    #endif    // end of #ifdef ESP32
#else  // we don't use DAC, using software-simulated delta-sigma DAC
    out = new AudioOutputI2SNoDAC();
    #ifdef ESP32
      Serial.println(F("Using No DAC output on ESP32, audio output on pins \r\n speaker L & R -> GPIO22"));
    #else
      Serial.println(F("Using No DAC output on ESP8266, audio output on pins \r\n speaker L & R -> GPIO3 (RX)"));
    #endif
    Serial.println(F("Don't try and drive the speaker pins can't give enough current to drive even a headphone well and you may end up damaging your device"));
#endif


  out->SetGain(0.8);

  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");

}


void loop()
{
  static int lastms = 0;

  if (mp3->isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
     }
    if (!mp3->loop()) {
            mp3->stop();
            // out->setLevel(0);
            delete file;
            delete buff;
            Serial.println("mp3 stop");
    }
  }


    if (millis()-lastms > 5000) {   //wait 5 seconds before restart playing
      VoiceText_tts(text1, tts_parms1);
      Serial.println("mp3 begin");
  //    VoiceText_tts(text1, tts_parms2);
  //    VoiceText_tts(text1, tts_parms3);
  }
}
