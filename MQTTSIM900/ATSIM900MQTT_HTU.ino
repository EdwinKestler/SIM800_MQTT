#include <mqtt.h>
#include <SoftwareSerial.h>

SoftwareSerial GPRS(2, 3); //RX, TX

//but is more 'smooth'
#define NUMSAMPLES 5

int samplesT [NUMSAMPLES];
int samplesH [NUMSAMPLES];
int HTU_Temp=0;
int HTU_Hum=0;
String gprsStr = "";
int index = 0;
byte data1;
char atCommand[50];
byte mqttMessage[127];
int mqttMessageLength = 0;


//bolean flags
boolean smsReady = false;
boolean smsSent = false;
boolean gprsReady = false;
boolean mqttSent = false;

void setup(){
  Serial.begin (9600); //debug
  GPRS.begin(9600); //GPRS
  
  Serial.println("HTU21D-F test");
   
}

void loop (){
  delay (15000); //Time to get GPRS Satable
  
  get_HTU();
  
  Serial.println("Checking if GPRS is READy?");
  GPRS.println("AT");
  delay(1000);
  
  gprsReady= isGPRSReady();
  if(gprsReady == true){
    Serial.println("GPRS READY!");
    String json= buildJson();
    char jsonStr[300];
    json.toCharArray(jsonStr,300);
    Serial.println(json);
    
    //*arguments in function are:
    //clientID, IP, Port, Topic, Message
    
    sendMQTTMessage("NodeHTU", "flatbox.io", "1883", "prueba/", jsonStr);
  }
  delay (15000);
}

boolean isGPRSReady(){
 GPRS.println("AT");
 GPRS.println("AT");
 GPRS.println("AT+CGATT?");
 index = 0;
 while (GPRS.available()){
    data1 = (char)GPRS.read();
    Serial.write(data1);
    gprsStr[index++] = data1;
    }
 Serial.println("Check OK");
 Serial.print("gprs str = ");
 Serial.println(data1);
 if (data1 > -1){
  Serial.println("GPRS OK");
  return true;
 }
 else {
  Serial.println("GPRS NOT OK");
  return false;
 }
}

void sendMQTTMessage(char* clientId, char* brokerUrl, char* brokerPort, char* topic, char* message){
 GPRS.println("AT"); // Sends AT command to wake up cell phone
 Serial.println("send AT to wake up GPRS");
 delay(1000); // Wait a second
// digitalWrite(13, HIGH);
 GPRS.println("AT+CSTT=\"broadband.tigo.gt\",\"\",\"\""); // Puts phone into GPRS mode
 Serial.println("AT+CSTT=\"broadband.tigo.gt\",\"\",\"\"");
 delay(2000); // Wait a second
 GPRS.println("AT+CIICR");
 Serial.println("AT+CIICR");
 delay(2000);
 GPRS.println("AT+CIFSR");
 Serial.println("AT+CIFSR");
 delay(2000);
 strcpy(atCommand, "AT+CIPSTART=\"TCP\",\"");
 strcat(atCommand, brokerUrl);
 strcat(atCommand, "\",\"");
 strcat(atCommand, brokerPort);
 strcat(atCommand, "\"");
 GPRS.println(atCommand);
 Serial.println(atCommand);
 // Serial.println("AT+CIPSTART=\"TCP\",\"mqttdashboard.com\",\"1883\"");
 delay(2000);
 GPRS.println("AT+CIPSEND");
 Serial.println("AT+CIPSEND");
 delay(2000);
 mqttMessageLength = 16 + strlen(clientId);
 Serial.println(mqttMessageLength);
 mqtt_connect_message(mqttMessage, clientId);
 for (int j = 0; j < mqttMessageLength; j++) {
 GPRS.write(mqttMessage[j]); // Message contents
 Serial.write(mqttMessage[j]); // Message contents
 Serial.println("");
 }
 GPRS.write(byte(26)); // (signals end of message)
 Serial.println("Sent");
 delay(10000);
 GPRS.println("AT+CIPSEND");
 Serial.println("AT+CIPSEND");
 delay(2000);
 mqttMessageLength = 4 + strlen(topic) + strlen(message);
 Serial.println(mqttMessageLength);
 mqtt_publish_message(mqttMessage, topic, message);
 for (int k = 0; k < mqttMessageLength; k++) {
 GPRS.write(mqttMessage[k]);
 Serial.write((byte)mqttMessage[k]);
 }
 GPRS.write(byte(26)); // (signals end of message)
 Serial.println("-------------Sent-------------"); // Message contents
 delay(5000);
 GPRS.println("AT+CIPCLOSE");
 Serial.println("AT+CIPCLOSE");
 delay(2000);
}

void get_HTU(){
   uint8_t i;
  float averageT;
  float averageH;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samplesT[i] = 25;
   samplesH[i] = 45;
   delay(100);
  }
 
  // average all the samples out
  averageT = 0;
  averageH = 0;
  
  for (i=0; i< NUMSAMPLES; i++) {
     averageT += samplesT[i];
     averageH += samplesH[i];
  }
  averageT /= NUMSAMPLES;
  averageH /= NUMSAMPLES;
  
  Serial.print("Average reading "); 
  Serial.println(averageT);
  Serial.println(averageT);
  
  HTU_Temp= averageT;
  HTU_Hum=  averageH;
  
  Serial.print("Temperature "); 
  Serial.print(HTU_Temp);
  Serial.println(" *C");

  Serial.print("Humidity "); 
  Serial.print(HTU_Hum);
  Serial.println(" %");
  
}

String buildJson() {
  String data = "{";
  data+="\n";
  data+= "\"d\": {";
  data+="\n";
  data+="\"ID\": \"HTU_Node_01\",";
  data+="\n";
  data+="\"HTU_T\": ";
  data+=(int)HTU_Temp;
  data+="\n";
  data+="\"HTU_H\": ";
  data+=(int)HTU_Hum;
  data+="\n";
  data+="}";
  data+="\n";
  data+="}";
  return data;
}
