
////////////////////////////////////////////////////////////////////////////////////////
/*
Name:Mohamed Hassan
Project:Vogelhaus_ESP32_S3
HSRW
Version 1.3
*/
////////////////////////////////////////////////////////////////////////////////////////


#include "esp_camera.h"
#include "FS.h"
#include "SPI.h"
#include <WiFi.h>
#include "SD.h"
#include <ESP_Mail_Client.h>
#include "camera_pins.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      21

//Mail_vogelhaus512@gmail.com
//Password_mail_Vogelhaus2024#123


/*
// Your network credentials_home
const char* ssid = "iotlab";
const char* password = "iotlab18";
*/


// Your network credentials_Lab
const char* ssid = "Vodafone-6CB4";
const char* password = "nptYqTknrqxKdY2Z";



// SMTP Server settings
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 //587
#define AUTHOR_EMAIL "vogelhaus512@gmail.com"
#define AUTHOR_PASSWORD "roykllvkkqxizpjk"// this is the password from the app passwords from google portal account

// Recipient Email
#define RECIPIENT_EMAIL "mohamedsherif225@gmail.com"

// Declare the global used SMTPSession object for SMTP transport
SMTPSession smtp;

// Declare the global used Session_Config for user defined session credentials
Session_Config config;           

// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status);

unsigned long lastCaptureTime = 0; // Last shooting time
int imageCount = 1;                // File Counter
bool camera_sign = false;          // Check camera status
bool sd_sign = false;              // Check SD status


////////////////////////////////////////////////////////////////////////////////////////////



void photo_save(const char * fileName) {
  // Take a photo
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Failed to get camera frame buffer");
    return;
  } else {
    Serial.println("Photo taken");
  }

  // Save photo to file
  writeFile(SD, fileName, fb->buf, fb->len);

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  Serial.println("Photo saved to file");

  // Check if the file was saved correctly
  File file = SD.open(fileName);
  if (file) {
    Serial.println("File saved successfully. File size: " + String(file.size()));
    file.close();
  } else {
    Serial.println("Failed to save file.");
  }
}





/////////////////////////////////////////////////////////////////////////////////////

// SD card write file
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len){
    Serial.printf("Writing file: %s\r\n", path);

    File file = LittleFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.write(data, len) == len){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}


///////////////////////////////////////////////////////////////////////////////////

void sendEmailWithAttachment(const char* filePath) {
// Set the session config
  config.server.host_name = SMTP_HOST; // for Gmail.com
  config.server.port = SMTP_PORT; // for TLS with STARTTLS or 25 (Plain/TLS with STARTTLS) or 465 (SSL)
  config.login.email = AUTHOR_EMAIL; // set to empty for no SMTP Authentication
  config.login.password = AUTHOR_PASSWORD; // set to empty for no SMTP Authentication
  
  // For client identity, assign invalid string can cause server rejection_Part of the main code in the library but commented for no use 
  config.login.user_domain = ""; 

 /*
   Set the NTP config time
   For times east of the Prime Meridian use 0-12
   For times west of the Prime Meridian add 12 to the offset.
   Ex. American/Denver GMT would be -6. 6 + 12 = 18
   See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
   */
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

    // Declare the SMTP_Message class variable to handle to message being transport
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

// Set the message headers
  message.sender.name = "Bird House";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP32-S3";
  message.addRecipient("Aly nour", "alinour93@gmail.com");
  message.addRecipient("Mohamed Hassan ", "mohamedsherif225@gmail.com");

/*
  message.addCc("email3");
  message.addBcc("email4");
*/

  //Send raw text message
  String textMsg = "This is a picture from birdhouse";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;


  /* The attachment data item */
  SMTP_Attachment att;

  /** Set the attachment info e.g. 
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
  */
  att.descr.filename = "/image.jpg";
  att.descr.mime = "image/jpg"; //binary data
  att.file.path = filePath; // this one i have changed it from image
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att);




  // Set debug option
  smtp.debug(1);

  // Set the callback function to get the sending results
  smtp.callback(smtpCallback);

  /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

}

//////////////////////////////////////////////////////////////////////////////////////////
  
  void smtpCallback(SMTP_Status status)
{
 
  Serial.println(status.info());

  if (status.success())
  {
    // See example for how to get the sending result
  }
}


/////////////////////////////////////////////////////////////////////////////////////

void deletePhoto(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);

  if (LittleFS.remove(path)) {
    Serial.println("File deleted successfully");
  } else {
    Serial.println("Failed to delete file");
  }
  }




////////////////////////////////////////////////////////////////////////////////////
  
  void enterDeepSleep(int seconds) {
    Serial.println("Entering deep sleep for " + String(seconds) + " seconds...");
    esp_deep_sleep(seconds * 1000000);
  }

/////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

  pinMode(10, OUTPUT);

#include "camera_pins.h"

ESP_MAIL_DEFAULT_FLASH_FS.begin();


#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(10, HIGH);
    delay(1000);
      digitalWrite(10, LOW);

    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

/*  Set the network reconnection option */
  MailClient.networkReconnect(true);



 // while(!Serial); // When the serial monitor is turned on, the program starts to execute

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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  camera_sign = true; // Camera initialization check passes


  Serial.println("Photos will begin in one minute, please be ready.");
  
  //photo_save("/image.jpg");



    // Initialize SD card
  if(!SD.begin(21)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  // Determine if the type of SD card is available
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  sd_sign = true; // sd initialization check passes

  Serial.println("Photos will begin in one minute, please be ready.");

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

  void loop() {


  // Pin_HIGH
  digitalWrite(0, HIGH);
  
  digitalWrite(7, HIGH);

  //Take_photo && Save_photo
  photo_save("/image.jpg");


    // Check if the photo was saved
  File file = SD.open("/image.jpg");
  if (file) {
    Serial.println("Image file exists, size: " + String(file.size()));
    file.close();
  }

  // send mail
  sendEmailWithAttachment("/image.jpg");

  //Delete_photo
  deletePhoto(SD,"/image.jpg");

  //Pin_LOW
  digitalWrite(0, LOW);


  //DeepSleep_10 Sec
  enterDeepSleep(5);

  
  }
