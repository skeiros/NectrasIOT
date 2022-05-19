/*********
  BIODRONE SENSOR 2019
  Author: Daniel Sequeiros
  http://www.biodrone.com.ar
  Te: +5493515920003
*********/
#include "config.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
//#include "FS.h"                // SD Card ESP32
//#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
 
#include "driver/rtc_io.h"
//#include <EEPROM.h>            // read and write from flash memory
#include <WiFi.h>
#include <HTTPClient.h>



bool internet_connected = false;
String error; // GUARDA ERROR Y ESTADOS
long rssi; // GUARDA LA SEÑAL

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  120//216000 // 1 Envio x Hora.
// define the number of bytes you want to access


// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

int pictureNumber = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector}
 
  
  Serial.begin(115200);

  error="TRAP ID "+String(trapID)+" TURN ON";
  Serial.println("TRAP ID "+String(trapID)+" TURN ON");
  delay(2000);
  
  //Serial.setDebugOutput(true);
  //Serial.println();
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 3;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 3;
    config.fb_count = 1;
  }
 /* if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }*/
  
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    error+=String(", Camera init failed with error");
  }


  
  //Serial.println("Starting SD Card");
 
  camera_fb_t * fb = NULL;
   
  esp_err_t res = ESP_OK;
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  delay(60);
      

   
  if(!fb) {
    Serial.println("Camera capture failed");
    error+=String(", Camera capture failed");
  }
 
  
  


 if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected BASE 1");
    error+=String(",Internet connected BASE 1");
  }
  if(!internet_connected){
       if (init_wifi2()) { // Connected to WiFi
        internet_connected = true;
        Serial.println("Internet connected BASE 2");
        error+=String(",Internet connected BASE 2");
      }  
  }
  error+=",Singal DB:"+String(rssi);
  /* MANDAMOS LA FOTO*/
  
  esp_http_client_handle_t http_client;
  
  esp_http_client_config_t config_client = {0};
  
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client,(const char *)fb->buf, fb->len);

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg");

  esp_err_t err2 = esp_http_client_perform(http_client); // MODIFICADO POR RE DEFINICIÓN
  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

  /**/
  
  esp_camera_fb_return(fb); 


  //MANDAMOS INFORME 

   HTTPClient http;   
 
   http.begin(post_url_error);  //Specify destination for HTTP request
   http.addHeader("Content-Type", "application/x-www-form-urlencoded");             //Specify content-type header
 
   int httpResponseCode = http.POST("trapid="+String(trapID)+"&error="+error);   //Send the actual POST request
 
   if(httpResponseCode>0){
 
    String response = http.getString();                       //Get the response to the request

    Serial.println("Return code");
    Serial.println(httpResponseCode);   //Print return code
    Serial.println("Request answer");
    Serial.println(response);           //Print request answer
 
   }else{
 
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
 
   }
 
   http.end();  //Free resources



  
  
  
  Serial.println(error);
  delay(500);
  Serial.println("Going to sleep now");
  delay(500);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12,1);
  //esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR*TIME_TO_SLEEP);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 20) return false;
    connAttempts++;
  }
  rssi=WiFi.RSSI();
  return true;
}

bool init_wifi2()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid2));
  WiFi.begin(ssid2, password2);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 20) return false;
    connAttempts++;
  }
  rssi=WiFi.RSSI();
  return true;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}


void loop() {
  
}
