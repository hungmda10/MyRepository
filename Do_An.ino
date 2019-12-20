#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>      
#include <SoftwareSerial.h> 
#include <SimpleTimer.h>
#define DHTTYPE DHT11   // DHT 11
#define ssid "Hung"
#define password "hunglan1"
// Thông tin về MQTT Broker
#define mqtt_server "192.168.20.8"
#define mqtt_user "root"    
#define mqtt_pwd "root"

#define LIGHT1_ON 10
#define LIGHT2_ON 11
#define AUTO 8
#define MANUAL 9
#define LIGHT1_OFF 12
#define LIGHT2_OFF 13

char lamp1 = 0;
char ledState = 0;
char lamp2 = 0;
SoftwareSerial mySerial(D6, D5);

const uint16_t mqtt_port = 1883; //Port của MQTT

WiFiClient espClient;
PubSubClient client(espClient);
const int DHTPin3 = D3;       //Đọc dữ liệu từ DHT11 ở chân D1 trên mạch esp8266
const int DHTPin8 = D4;
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

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Thiet Lap cam bien DHT.
DHT dht1(DHTPin3, DHTTYPE);
DHT dht2(DHTPin8, DHTTYPE);

/*Blynk*/
SimpleTimer timer;
char auth[] = "46yxK2hp1zGFYC0ITMGkIVOW6ORyyO5V";    //AuthToken copy ở Blynk Project aNprg497GFPmN2QmD6kXA5EHc4RNTn7_
int t1;
int h1;
int recieve = 0;
uint32_t buff[16];
int i = 0;
uint32_t request = 0;
char button_auto =0;
char button_manual =0;
int t2 = 0;
char light1_on = 0;
char light2_on = 0;
int h2 = 0;
uint32_t lux1 = 0;
uint32_t lux2 = 0;
int gas = 0;
WidgetLCD lcd(V7);
/*Blynk*/
void setup() {
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port); 
  dht2.begin();
  Serial.begin(9600);
  Blynk.begin(auth, ssid, password);//,IPAddress(192,168,1,163),8080
  timer.setInterval(2000, sendUptime);
  mySerial.begin(9600);
}

void sendUptime()
{
  Blynk.virtualWrite(V6, t1);
  Blynk.virtualWrite(V5, h1);
  Blynk.virtualWrite(V7, gas);
}
BLYNK_WRITE(V0)
{
  button_auto= param.asInt();
}


BLYNK_WRITE(V2)
{
  light1_on= param.asInt();
}

BLYNK_WRITE(V3)
{
  light2_on= param.asInt();
}
// Hàm kết nối wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
// Hàm reconnect thực hiện kết nối lại khi mất kết nối với MQTT Broker
void reconnect() {
  // Chờ tới khi kết nối
  while (!client.connected()) {
    Serial.print("Dang ket noi den MQTT...");
    // Thực hiện kết nối với mqtt user và pass
    if (client.connect("ESP8266Client",mqtt_user, mqtt_pwd)) {
      Serial.println("ket noi");
      // Khi kết nối sẽ publish thông báo
      client.subscribe("Topic 1");
      client.subscribe("Topic 2");
    } else {
      Serial.print("Ket noi that bai, rc=");
      Serial.print(client.state());
      Serial.println("Ket noi lai trong 5s");
      // Đợi 1s
      delay(1000);
    }
  }
}

void loop() {

    //Serial.println("Da gui");
    if(request == 2000)
    {
    mySerial.write('S');
    request = 0;
    


      //mySerial.write(10);
    if(button_auto == 0)
    {
      //Serial.println("auto");
      mySerial.write(AUTO);
    }
    else
    {
      if(light1_on == 0)
      {
        //Serial.println("LED1_on");
        mySerial.write(LIGHT1_ON);
      }
      else
      {
        //Serial.println("LED1_off");
        mySerial.write(LIGHT1_OFF);
      }
      //delay(100);
      if(light2_on == 0)
      {
        //Serial.println("LED2_on");
        mySerial.write(LIGHT2_ON);
      }
      else
      {
        //Serial.println("LED2_off");
        mySerial.write(LIGHT2_OFF);
      }
    
    }
    }
    
    if((mySerial.available() > 0)) //Nếu có tín hiệu gửi đến Serial
  {
    ledState = mySerial.read(); //Đọc giá trị gửi
    //Serial.write("Truoc khi nhan");
    
    if((ledState == 'B')&&(recieve == 0))
    {
      recieve = 1;
    }
    else if(recieve == 1)
    {
      buff[i] = ledState;
      //Serial.write(buff[i]);
      i++;
    }
    else
    {
      
    }
    if((recieve == 1)&&(ledState == 'E'))
    {
      
      t1 = buff[1];
      h1 = buff[3];
      gas = buff[5];
      recieve = 0;
      i = 0;
        if (!client.connected()) {
    reconnect();
  }
  if(client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Pub du lieu moi
  if (now - lastMeasure > 1000) {
    lastMeasure = now;
    getAndSendData();
}
    }
    else
    {
      
    }
  }

  request++;
  Blynk.run();
  timer.run();
}

void getAndSendData()
{ 
  
  char tempString[8];
  sprintf(tempString, "%d",t1);
  Serial.print("  Temperature: ");
  Serial.print(t1);
  client.publish("home/sensors/temperature", tempString);
  
  char humString[8];
  sprintf(humString, "%d", h1);
  Serial.print("  Humidity: ");
  Serial.println(h1);
  client.publish("home/sensors/humidity", humString);
  
  char gasString[8];
  sprintf(gasString, "%d",gas);
  Serial.print("  Gas: ");
  Serial.print(gas);
  client.publish("home/sensors/Gas", gasString);
  
}
