#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <TimerOne.h>
#include <EEPROM.h>
#include <math.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <RFID.h>
#include <Servo.h>

Servo myservo;
DHT dht(24, DHT11);
DS3231 rtc(20, 21);
LiquidCrystal_I2C lcd(0x23,16,2);
LiquidCrystal_I2C in_lcd(0x27,16,2);
Time real_time;


#define MODE_CHANGE 1
#define MODE_ADD 2
#define MODE_RUN 3
#define MODE_CLEAR 4

#define MODE_FIRST_PASS 6
#define MODE_SECOND_PASS 7

#define MODE_FIRST_FINGER 8
#define MODE_SECOND_FINGER 9
#define MODE_ID_FINGER_ADD 5
#define MODE_ID_FINGER_CLEAR 17

#define LIGHT1_ON 10
#define LIGHT2_ON 11
#define AUTO 8
#define LIGHT1_OFF 12
#define LIGHT2_OFF 13

#define OK 15
#define NOT_OK 16


#define SS_PIN 53

#define RST_PIN 49

RFID rfid(SS_PIN, RST_PIN);
unsigned char reading_card[5];
char key = 0;
char mode = MODE_RUN;
int count_key = 0;
char buffer_pass[4];
char id_array[4]={0};
int id_count = 0;
int first_pass = 0;
int second_pass = 0;
int id = 0;
int buzzer = 22;
uint32_t gas = 0;
char p = FINGERPRINT_PACKETRECIEVEERR;
const byte rows = 4; //số hàng
const byte columns = 4; //số cột
char lamp1 = 12;
char lamp2 = 18;
char keys[rows][columns] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
  };

byte rowPins[rows] = {31, 33, 35, 37};
byte columnPins[columns] = {23, 25, 27, 29};
int humidity = 0;
int temperature = 0;
int card_pass[5] = {240, 228, 151, 165, 38};
char control_light = 0;
char state_light1 = 0;
char state_light2 = 0;

Keypad keypad = Keypad(makeKeymap(keys), rowPins, columnPins, rows, columns);
SoftwareSerial mySerial(10, 9);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

byte degree[8] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

void setup()  
{
  Timer1.initialize(100000); 
  Timer1.attachInterrupt(get_pass);
  finger.begin(57600);
  Serial.begin(9600);
  Serial3.begin(9600);

  Wire.begin();
  dht.begin();
  rtc.begin(); 
  SPI.begin();
  rfid.init();
  Serial.println("a");
  lcd.begin();  
  in_lcd.begin();
  in_lcd.createChar(1, degree);
  while(p != FINGERPRINT_OK)
  {
   p = finger.getTemplateCount();
   delay(50);
   lcd.setCursor(0,0);
   lcd.print("INIT Fingersensor");
  }
  Serial.println(finger.templateCount);
  in_lcd.clear();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INIT success");
  lcd.clear();
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  myservo.attach(11);
  myservo.write(175);
  delay(100);
  myservo.detach();
  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  digitalWrite(lamp1, HIGH);
  digitalWrite(lamp2, HIGH);
}

void loop()                  
{
  RFID_Reader();
  process_key();
  process_mode();
  DHT_read();
  Tranfer_data();
  light_fun();
}

void light_fun(void)
{
    if(control_light == 1)
    {
        if(state_light1 == 0)
        {
            digitalWrite(lamp1, LOW);
        }
        else
        {
            digitalWrite(lamp1, HIGH);
        }
        
        if(state_light2 == 0)
        {
            digitalWrite(lamp2, LOW);
        }
        else
        {
            digitalWrite(lamp2, HIGH);
        }
    }
}

void Tranfer_data(void)
{
  char Esp_data = 0;
  char send_data = 0;
  while(Serial3.available() > 0)
  {
    Esp_data = Serial3.read();
    if(Esp_data == 'S')
    {
      Esp_data = 0;
      send_data = 1;
    }
     
    switch(Esp_data)
    {
     
      case LIGHT1_ON:
      control_light = 1;
      state_light1 = 1;
      break;
      case LIGHT2_ON:
      control_light = 1;
      state_light2 = 1;
      break;
      case LIGHT1_OFF:
      control_light = 1;
      state_light1 = 0;
      break;
      case LIGHT2_OFF:
      control_light = 1;
      state_light2 = 0;
      break;
      case AUTO:
      control_light = 0;
      break;
      default :
      break;
    }
  }
  
  if(send_data == 1)
  {
    Serial3.write("B");
    Serial3.write("T");
    Serial3.write(temperature);
    Serial3.write("H");
    Serial3.write(humidity);
    Serial3.write("G");
    Serial3.write(gas);
    Serial3.write("E");
    send_data = 0;
  }
}

void RFID_Reader(void)
{
  int i, j = 0;
  char correct = 0;
  if (rfid.isCard())
  {
    if (rfid.readCardSerial())
    {
      for (i = 0; i < 5; i++)
      {
        reading_card[i] = rfid.serNum[i];
        if(reading_card[i] != card_pass[i])
        {
          correct = 1;
        }
      }
      if(correct == 1)
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Access Denied");
        delay(1000);
        lcd.clear();
      }
      else
      {
        if(control_light == 0)
        {
          digitalWrite(lamp2, LOW);
        }
        lcd.clear();
        lcd.setCursor(5,0);
        lcd.print("Access");
        myservo.attach(11);
        myservo.write(90);
        delay(3000);
        myservo.write(175);
        delay(100);
        myservo.detach();
        lcd.clear();
      }
      
    }
    rfid.halt();
  }

  
}

void DHT_read(void)
{
  int i = 0;
  uint32_t buf_gas =0;
  humidity = (int)dht.readHumidity();
  temperature = (int)dht.readTemperature();
  gas = 0;
  for(i = 0; i < 10;i++)
  {
  buf_gas = analogRead(A2);
  gas = gas + buf_gas;
  }
  gas = gas/10;
  gas = map(gas, 0, 1023, 1,100);
  in_lcd.setCursor(0,1);
  in_lcd.print("Khi Gas: ");
  in_lcd.setCursor(9,1);
  if(gas < 0)
  {
    gas = 0;
  }
  if(gas < 10)
  {
    in_lcd.print("0");
    in_lcd.print(gas);
  }
  else
  {
    in_lcd.print(gas);
  }
  
  in_lcd.print("%");

  in_lcd.setCursor(0,0);
  in_lcd.print("Tem:");
  in_lcd.print(temperature);
  in_lcd.write(1);
  in_lcd.print("C");

  in_lcd.setCursor(9,0);
  in_lcd.print("Hum:");
  in_lcd.print(humidity);
  in_lcd.print("% ");  

  if(gas > 30)
  {
    digitalWrite(buzzer, HIGH);
    delay(3000);
    digitalWrite(buzzer, LOW);
  }

}
void process_key(void)
{
    char value_0 = 0;
    char value_1 = 0;
    char value_2 = 0;
    int i = 0;
    switch(mode)
    {
        case MODE_ID_FINGER_ADD:
            lcd.setCursor(0,0);
            lcd.print("Them Van Tay");
            lcd.setCursor(0,1);
            lcd.print("ID:");
            if((key < 58)&&(key > 47))
            {
                id_array[id_count] = key - 48;
                id_count++;
                lcd.setCursor((6+id_count),1);
                lcd.print(key);
                key = 0;
            }
            else if(key == 'D')
            {
                switch(id_count)
                {
                    case 1:
                    id = id_array[0];
                    break;
                    case 2:
                    id = 10*(id_array[0]) + id_array[1];
                    break;
                    case 3:
                    id = 100*(id_array[0]) + 10*(id_array[1]) + id_array[2];
                    break;
                    case 4:
                    id = 1000*(id_array[0]) + 100*(id_array[1]) + 10*(id_array[2]) + id_array[3];
                    break;
                    default:
                    break;
                }
                mode = MODE_FIRST_FINGER;
            }
            else
            {
                
            }
        break;
        case MODE_FIRST_FINGER:
            lcd.setCursor(0,1);
            lcd.print("Dat ngon tay");
            p = finger.getImage();
            if(p == FINGERPRINT_OK)
            {
                p = finger.image2Tz(1);
                if(p == FINGERPRINT_OK)
                {
                    digitalWrite(buzzer, HIGH);
                    delay(100);
                    digitalWrite(buzzer, LOW);
                    lcd.setCursor(0,1);
                    lcd.print("Dat lai ngon tay");
                    delay(2000);
                    mode = MODE_SECOND_FINGER;
                }
            }
        break;
        case MODE_SECOND_FINGER:
            p = finger.getImage();
            if(p == FINGERPRINT_OK)
            {
                p = finger.image2Tz(2);
                if(p == FINGERPRINT_OK)
                {
                    p = finger.createModel();
                    if(p == FINGERPRINT_OK)
                    {
                        p = finger.storeModel(id);
                        if(p == FINGERPRINT_OK)
                        {
                            lcd.setCursor(0,1);
                            lcd.print("Them Thanh Cong");
                            delay(2000);
                            lcd.clear();
                            mode = MODE_RUN;
                        }
                    }
                    else if(p == FINGERPRINT_ENROLLMISMATCH)
                    {
                        lcd.setCursor(0,1);
                        lcd.print("Khong Khop");
                        delay(2000);
                        lcd.clear();
                        mode = MODE_RUN;
                    }
                    else
                    {
                      
                    }
                    id = 0;
                }
            }
        break;
        case MODE_RUN:
            lcd.setCursor(5,0);
            lcd.print("WELCOME");
            p = finger.getImage();
            if(p == FINGERPRINT_OK)
            {
                p = finger.image2Tz();
                if(p == FINGERPRINT_OK)
                {
                    p = finger.fingerFastSearch();
                    lcd.clear();
                    if(p == FINGERPRINT_OK)
                    {
                        if(control_light == 0)
                        {
                            digitalWrite(lamp2, LOW);
                        }
                        lcd.print("Success");
                        myservo.attach(11);
                        myservo.write(90);
                        delay(3000);
                        myservo.write(175);
                        delay(100);
                        myservo.detach();
                    }
                    else
                    {
                        lcd.print("Not found");
                    }
                    
                    delay(2000);
                    lcd.clear();
                }
            }
        break;
        case MODE_ID_FINGER_CLEAR:
            lcd.setCursor(0,0);
            lcd.print("Xoa Van Tay");
            lcd.setCursor(0,1);
            lcd.print("ID:");
            if((key < 58)&&(key > 47))
            {
                id_array[id_count] = key - 48;
                id_count++;
                lcd.setCursor((6+id_count),1);
                lcd.print(key);
                key = 0;
            }
            else if(key == 'D')
            {
                switch(id_count)
                {
                    case 1:
                    id = id_array[0];
                    break;
                    case 2:
                    id = 10*(id_array[0]) + id_array[1];
                    break;
                    case 3:
                    id = 100*(id_array[0]) + 10*(id_array[1]) + id_array[2];
                    break;
                    case 4:
                    id = 1000*(id_array[0]) + 100*(id_array[1]) + 10*(id_array[2]) + id_array[3];
                    break;
                    default:
                    break;
                }
                p = finger.deleteModel(id);
                if(p == FINGERPRINT_OK)
                {
                    lcd.setCursor(0,1);
                    lcd.print("Clear");
                    lcd.print(id);
                    lcd.print("success");
                    delay(2000);
                    lcd.clear();
                }
                else if(p == FINGERPRINT_BADLOCATION)
                {
                    lcd.setCursor(0,1);
                    lcd.print("id");
                    lcd.print(id);
                    lcd.print("invalid");
                    delay(2000);
                    lcd.clear();
                }
                else
                {
                    
                }
                key = 0;
                id = 0;
                mode = MODE_RUN;
            }
            else if(key == 'A')
            {
                p = finger.emptyDatabase();
                if(p == FINGERPRINT_OK)
                {
                    lcd.setCursor(0,1);
                    lcd.print("Clear all");
                    delay(2000);
                    lcd.clear();
                }
                key = 0;
                mode = MODE_RUN;
            }
            else
            {
                
            }
        break;
        case MODE_FIRST_PASS:
            /*LCD print*/
            lcd.setCursor(0,0);
            lcd.print("Mat Khau Moi");
            if((key < 58)&&(key > 47))
            {
                if(count_key < 4)
                {
                    buffer_pass[count_key] =  key - 48;
                    count_key++;
                    lcd.setCursor((count_key + 6),1);
                    lcd.print("*");
                    key = 0;
                }
                
                if(count_key == 4)
                {
                    first_pass = 1000*(buffer_pass[0]) + 100*(buffer_pass[1]) + 10*(buffer_pass[2]) + buffer_pass[3];
                    count_key = 0;
                    mode = MODE_SECOND_PASS;
                    delay(1000);
                    lcd.clear();
                }
            }
        break;
        case MODE_SECOND_PASS:
            lcd.setCursor(0,0);
            lcd.print("Nhap Lai");
            if((key < 58)&&(key > 47))
            {
                if(count_key < 4)
                {
                    buffer_pass[count_key] =  key - 48;
                    count_key++;
                    lcd.setCursor((count_key + 6),1);
                    lcd.print("*");
                    key = 0;
                }
                
                if(count_key == 4)
                {
                    second_pass = 1000*(buffer_pass[0]) + 100*(buffer_pass[1]) + 10*(buffer_pass[2]) + buffer_pass[3];
                    delay(1000);
                    lcd.clear();
                    lcd.setCursor(0,0);
                    if(second_pass == first_pass)
                    {
                        value_0 = second_pass/1000;
                        second_pass = second_pass - value_0*1000;
                        value_1 = second_pass/100;
                        second_pass = second_pass - value_1*100;
                        value_2 = second_pass/10;
                        second_pass = second_pass - value_2*10;
                        
                        EEPROM.write(0, value_0);
                        EEPROM.write(1, value_1);
                        EEPROM.write(2, value_2);
                        EEPROM.write(3, second_pass);
                        lcd.print("Thanh Cong");
                        delay(2000);
                        lcd.clear();
                    }
                    else
                    {
                        lcd.print("Khong Khop");
                        delay(2000);
                        lcd.clear();
                    }
                    count_key = 0;
                    mode = MODE_RUN;
                }
            }
        break;
        default:
        break;
    }
}

void process_mode(void)
{
  char status_pass = 0;
    switch(mode)
    {
        case MODE_CHANGE:
            lcd.setCursor(0,0);
            lcd.print("Thay Doi Mat Khau");
            lcd.setCursor(0,1);
            lcd.print("Nhap:");
            break;
        case MODE_ADD:
            lcd.setCursor(0,0);
            lcd.print("Them van tay");
            lcd.setCursor(0,1);
            lcd.print("Nhap:");
            break;
        case MODE_CLEAR:
            lcd.setCursor(0,0);
            lcd.print("Xoa van tay");
            lcd.setCursor(0,1);
            lcd.print("Nhap:");
            break;
        default:
            break;
    }
    status_pass = check_password();
    if(status_pass == OK)
    {
        switch(mode)
        {
            case MODE_CHANGE:
                mode = MODE_FIRST_PASS;
                break;
            case MODE_ADD:
                mode = MODE_ID_FINGER_ADD;
                break;
            case MODE_CLEAR:
                mode = MODE_ID_FINGER_CLEAR;
                break;
            default:
                break;
        }
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Mat Khau chinh xac");
        if(mode == MODE_RUN)
        {
          if(control_light == 0)
          {
            digitalWrite(lamp2, LOW);
          }
          myservo.attach(11);
          myservo.write(90);
          delay(3000);
          myservo.write(175);
          delay(100);
          myservo.detach();
        }
        delay(2000);
        lcd.clear();
    }
    else if(status_pass == NOT_OK)
    {
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("ko chinh xac");
        delay(2000);
        lcd.clear();
        mode = MODE_RUN;
    }
    else
    {
    
    }
}

char check_password(void)
{
    char value_0 = 0;
    char value_1 = 0;
    char value_2 = 0;
    char value_3 = 0;
    int pass  = 0;
    int pass_convert = 0;
    char status_pass = 0;
  
  if((key < 58)&&(key > 47))
  {
      if(count_key < 4)
      {
          buffer_pass[count_key] =  key - 48;
          count_key++;
          lcd.setCursor((count_key + 6),1);
          lcd.print("*");
          key = 0;
      }
      
      if(count_key == 4)
      {
        pass_convert = 1000*(buffer_pass[0]) + 100*(buffer_pass[1]) + 10*(buffer_pass[2]) + buffer_pass[3];
        count_key = 0;
        
        value_0 = EEPROM.read(0);
        value_1 = EEPROM.read(1);
        value_2 = EEPROM.read(2);
        value_3 = EEPROM.read(3);
        pass = 1000*value_0 + 100*value_1 + 10*value_2 + value_3;
        if(pass_convert == pass)
        {
            status_pass = OK;
        }
        else
        {
            status_pass = NOT_OK;
        }
      }
  }
  return status_pass;
}
void get_pass(void)
{
  char temp = keypad.getKey();
  if ((int)keypad.getState() ==  PRESSED) {
    if (temp != 0) {
      key = temp;
      Serial.println(key);
    }
  }
  /* set up password*/   
  if((key == '*')&&(mode == MODE_RUN))
  {
      mode = MODE_CHANGE;
  }
  /* Add finger*/
  else if((key == '#')&&(mode == MODE_RUN))
  {
      mode = MODE_ADD;
  }
  /* Clear finger*/
  else if((key == 'C')&&(mode == MODE_RUN))
  {
      mode = MODE_CLEAR;
  }
  else
  {
  }
  temp = 0;
}
