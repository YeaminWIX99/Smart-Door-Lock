/*
connections 
1.fingerprint
red=vcc(5V) black=gnd green=17 yellow=18
2.lipo + goes to motor positive (red) and voltage divider and (-) goes to ground
3. voltage divider middle point goes to A0 
positive to lipo positive
and other goes to gnd
4. Servo +(red) goes to lipo +
and - (chocolate) goes to gnd
pwm (orange) goes to Digital 9

5. bluetooth connected to serial 2
TX and RX are cross connected to RX and TX of arduino

6. ESP connected to serial 3
TX and RX are cross connected to RX and TX of arduino


*/









//Fingerprint libraries
#include <Wire.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display


//int getFingerprintIDez();



#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//servo library
#include <Servo.h>
Servo miServo;


/////////////////////////////////////Input and outputs////////////////////////////////////////////////
//int scan_pin = 13;    //Pin for the scan push button
//int add_id_pin = 12;  //Pin for the add new ID push button
int servo_pin = 9;
int push_pin = 7;
char message[100];
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////Editable variables//////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
int main_user_ID = 1;  //Change this value if you want a different main user
int door_open_degrees = 180;
int door_close_degrees = 0;
String file_name_to_save = "blackBox.txt";
//////////////////////////////////////////////////////////////////////////////////////////////////////

//Other used variables in the code. Do not change!
bool scanning = false;
int counter = 0;
int id_ad_counter = 0;
bool id_ad = false;
//uint8_t num = 1;
bool id_selected = false;
uint8_t id;
bool first_read = false;
bool main_user = false;
bool add_new_id = false;
bool door_locked = true;


//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////
#define REMOTEXY_MODE__HARDSERIAL

#include <RemoteXY.h>
#define REMOTEXY_SERIAL Serial2
#define REMOTEXY_SERIAL_SPEED 9600


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 71 bytes
  { 255,13,0,12,0,64,0,16,31,1,2,0,6,33,22,11,0,24,31,31,
  79,78,0,79,70,70,0,67,4,11,56,45,6,31,24,11,2,0,36,33,
  22,11,0,24,31,31,79,78,0,79,70,70,0,7,36,11,65,45,5,31,
  24,2,11,66,1,29,10,7,16,134,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t switch_1; // =1 if switch ON and =0 if OFF 
  uint8_t switch_3; // =1 if switch ON and =0 if OFF 
  char edit_1[11];  // string UTF8 end zero  

    // output variables
  char text_1[11];  // string UTF8 end zero 
  int8_t level_1; // =0..100 level position 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////


/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6PxrbPTRq"
#define BLYNK_TEMPLATE_NAME "eee416"
#define BLYNK_AUTH_TOKEN "U1r00fOIpY38sJGlD0SFOiqyKDTCtLMn"

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "connected without internet";
char pass[] = "economics";
int pinValue;
int flag = -1;  //flag=1 door close and flag=-1 door opened
// Hardware Serial on Mega, Leonardo, Micro...
#define EspSerial Serial3


// Your ESP8266 baud rate:
#define ESP8266_BAUD 115200

ESP8266 wifi(&EspSerial);
int v = 10;
// This function will be called every time Slider Widget
// in Blynk app writes values to the Virtual Pin V1
int num;


void setup() {

  RemoteXY_Init();
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  //Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass);

  Serial.begin(9600);

  pinMode(push_pin, INPUT_PULLUP);
  //Define pins as outputs or inputs
  //  pinMode(scan_pin, INPUT);
  // pinMode(add_id_pin, INPUT);

  // pinMode(close_door,INPUT);
  miServo.attach(servo_pin);
  servo_initalize();


  //fingerprint sensor
  finger.begin(57600);
  finger.getParameters();  // set the data rate for the sensor serial port
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  lcd.init();  // initialize the lcd
  lcd.backlight();
  lcd.clear();
}

void loop()

{
  RemoteXY_Handler();
  //Blynk.run();
  //RemoteXY.led_1 = (flag == -1) * 1 + (flag == 1) * 2;
  printVolts();
  //Serial.println("\n");
  //Serial.println(digitalRead(push_pin));

  if ((digitalRead(push_pin) == 0)) {
    strcpy(message, "scanning");
    //Serial.println("funck mahin\n");

    Lcdprint(message);
    int id1 = (getFingerprintIDez());
    if (id1 != -1 && id1 > 0) {
      open_door();
    }
  }
  // may need to chage this portion


  else if (RemoteXY.switch_1 == 1 && digitalRead(push_pin) == 1) {
    sprintf(RemoteXY.text_1, "ServoMOTOR");
    strcpy(message, "Control by app");
    Lcdprint(message);
    open_door();
    RemoteXY.switch_1 = 0;
    delay(100);
  }


  else if ((RemoteXY.switch_3 == 1)&& digitalRead(push_pin) == 1) {

    sprintf(RemoteXY.text_1, "Enter Id");
    strcpy(message, "Enter Id from app");
    Lcdprint(message);
    num = atoi(RemoteXY.edit_1);
    //Serial.print("aaaaaa\n");
    if (strcmp(RemoteXY.text_1, "Enter Id") == 0 && num != 0)

    {
      Serial.print("entered\n");
      //int t=millis();
      // while(millis()-t<40000);
      add_new_finger();
      //RemoteXY.switch_3 = 0;

    }
  }
}


uint8_t readnumber(void) {
  return num;
}



int add_new_finger() {
  id = readnumber();
  if (id == 0) {  // ID #0 not allowed, try again!
    return;
  }

  return getFingerprintEnroll();
}



//scanning for add new finger print
  uint8_t getFingerprintEnroll() {
    unsigned long time = millis();
    int p = -1;
    // Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
    while (p != FINGERPRINT_OK && (millis() - time < 10000)) {
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
          Lcdprint("Image taken");
          break;
        case FINGERPRINT_NOFINGER:
          Serial.println(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
    }

    // OK success!

    p = finger.image2Tz(1);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        Lcdprint("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }
Serial.println("Remove finger");
Lcdprint("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }
    Serial.print("ID ");
    Serial.println(id);
    p = -1;
    Serial.println("Place same finger again");
    Lcdprint("Place same finger again");
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          Serial.println("Image taken");
           Lcdprint("Image taken");
          break;
        case FINGERPRINT_NOFINGER:
          Serial.print(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
    }

    // OK success!

    p = finger.image2Tz(2);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        Lcdprint("Image converted");
        
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
    }

    // OK converted!
    Serial.print("Creating model for #");  
    Lcdprint("Creating model for #");
    Serial.println(id);
    lcd.setCursor(0, 0);
  lcd.print(id);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      Serial.println("Prints matched!");
      Lcdprint("Prints matched");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      Serial.println("Fingerprints did not match");
      return p;
    } else {
      Serial.println("Unknown error");
      return p;
    }

    Serial.print("ID "); 
    Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
      Serial.println("Stored!");
      Lcdprint("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Could not store in that location");
      return p;
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash");
      return p;
    } else {
      Serial.println("Unknown error");
      return p;
    }
  }



//end of code for adding new finger print
void open_door() {
  miServo.write(70);
  delay(700 * 2);
  miServo.write(90);
  delay(500);
  if (flag == -1) {
    Blynk.logEvent("door_open");
  }
  flag = -flag;
}

void printVolts() {
  int sensorValue = analogRead(A0);                  //read the A0 pin value
  float voltage = map(sensorValue, 0, 1023, 0, 12);  //convert the value to a true voltage.
  //Serial.print(voltage);
  RemoteXY.level_1 = map(sensorValue, 0, 1023, 0, 100);
  if (voltage < 6.50)  //set the voltage considered low battery here
  {
    Blynk.logEvent("battery_low");
  }
}

void servo_initalize() {
  miServo.write(0);
  delay(50);
  miServo.write(90);
}


// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  // found a match!
  Serial.print("Found ID #");
  Lcdprint("Found ID #");
  //lcd.setCursor(0, 0);
  //lcd.print("Found ID #");
  Serial.print(finger.fingerID);
  lcd.setCursor(0, 0);
  lcd.print(finger.fingerID);
  //lcd.setCursor(0, 1);
  //lcd.print(finger.fingerID);
  //lcd.clear();
  Serial.print(" with confidence of ");
  Lcdprint(" with confidence of ");
  //lcd.setCursor(0, 0);
  //lcd.print(" with confidence of ");
  Serial.println(finger.confidence);
  lcd.setCursor(0, 0);
  lcd.print(finger.fingerID);
  //lcd.setCursor(0, 1);
  //lcd.print(finger.confidence);
  //lcd.clear();
  //Serial.print("main not second");
  return finger.fingerID;
}

void Lcdprint(char a[]) {
  Serial.print(a);
  lcd.setCursor(0, 0);
  lcd.print(a);
  delay(500);
  lcd.clear();
}