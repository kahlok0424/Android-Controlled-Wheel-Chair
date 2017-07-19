#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
// define pin
#define MOTOR_LEFT_STEP_PIN   4
#define MOTOR_LEFT_DIR_PIN    5
#define MOTOR_RIGHT_STEP_PIN  0
#define MOTOR_RIGHT_DIR_PIN   2
#define MOTOR_LEFT_FOWARD     1
#define MOTOR_LEFT_BACKWARD   0
#define MOTOR_RIGHT_FOWARD    0
#define MOTOR_RIGHT_BACKWARD  1

const char* ssid = "abcdefg";
const char* password =  "qwer123456";

typedef struct MotorInfo MotorInfo;
struct  MotorInfo {
  int motorControlPin;
  int pinState;
  int dirPin;
  int dir;  
  int steps;
  unsigned long prevTime;
  unsigned long delay;
};

MotorInfo leftMotorInfo = {
  .motorControlPin = MOTOR_LEFT_STEP_PIN,
  .pinState = LOW ,
  .dirPin = MOTOR_LEFT_DIR_PIN,
  .dir = MOTOR_LEFT_FOWARD,
  .steps = 0,
  .prevTime = 0,
  .delay = 800,
};

MotorInfo rightMotorInfo = {
  .motorControlPin = MOTOR_RIGHT_STEP_PIN,
  .pinState = LOW ,
  .dirPin = MOTOR_RIGHT_DIR_PIN,
  .dir = MOTOR_RIGHT_FOWARD,
  .steps = 0,
  .prevTime = 0,
  .delay = 400,
};

typedef struct AngleSpeed AngleSpeed;
struct  AngleSpeed {
  int Speed;
  int Angle;
  int previousAngle;
  int previousSpeed; 
};

AngleSpeed instruction ={
  .Speed = 0.0 ,
  .Angle = 270.00 ,
  .previousAngle = 270,
  .previousSpeed = 0,
};

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_LEFT_STEP_PIN, OUTPUT); 
  pinMode(MOTOR_LEFT_DIR_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_STEP_PIN, OUTPUT); 
  pinMode(MOTOR_RIGHT_DIR_PIN, OUTPUT);

  WiFi.softAP(ssid, password,1,1);
  handling(&instruction);
  handleUturn(&leftMotorInfo , & rightMotorInfo);
  server.begin();
  IPAddress myIP=WiFi.softAPIP();
  Serial.println(myIP);
  delay(150);
}

int TimerExpired(unsigned long duration ,unsigned long previous )
{   
   unsigned long current = micros();
    if(duration != -1 && (current - previous >= duration))
    {
      return 1;
    }
    return 0;
}

void motorStep(MotorInfo *motorInfo)
{
  digitalWrite(motorInfo->dirPin , motorInfo->dir);
  
  if(TimerExpired(motorInfo->delay, motorInfo->prevTime))
   {
    if(motorInfo->pinState == LOW){
      motorInfo->pinState = HIGH;
    }
    else{
      motorInfo->pinState = LOW;
    }
    motorInfo->prevTime = micros();
    digitalWrite( motorInfo->motorControlPin , motorInfo->pinState);
    motorInfo-> steps ++ ;
   } 
}

void Uturn(MotorInfo *leftInfo , MotorInfo *rightInfo)
{
    leftInfo->dir = MOTOR_LEFT_FOWARD;
    leftInfo->delay = 400;
    rightInfo->dir = MOTOR_RIGHT_BACKWARD;
    rightInfo  ->delay = 400; 
  while( (leftInfo->steps < 4000) && (rightInfo-> steps <4000) )
  {
      motorStep(leftInfo);
      motorStep(rightInfo);
  }
}

void ForceStop(MotorInfo *leftInfo , MotorInfo *rightInfo)
{
  leftInfo-> delay = -1;
  rightInfo -> delay = -1;
}

void Calculation(AngleSpeed *MainInfo , MotorInfo *leftMotor , MotorInfo *rightMotor)
{
    float x =0.0;
    float y =0.0;

      if( MainInfo->Speed == 0 )
     {
      // ForceStop(leftMotor ,rightMotor);
      }

   if(MainInfo->previousSpeed != MainInfo->Speed){
    MainInfo->previousSpeed = MainInfo->Speed;
    leftMotor->delay = 6000 / MainInfo->Speed;
    rightMotor->delay = 6000 / MainInfo->Speed;
   }

   if(MainInfo->previousAngle != MainInfo->Angle)
   {
      MainInfo->previousAngle = MainInfo->Angle;
    if( 0 <= MainInfo->Angle && MainInfo->Angle <= 180)
      {
      leftMotor -> dir = MOTOR_LEFT_FOWARD ;
      rightMotor -> dir = MOTOR_RIGHT_FOWARD;
      x = (float) (MainInfo->Angle /180.0);
      y = (float) (1 - (MainInfo->Angle/180.0));
      printf("x = %f \n",x);
      printf("y = %f \n",y);
      leftMotor -> delay = (unsigned long)(leftMotor->delay * x);
      rightMotor -> delay = (unsigned long)( rightMotor->delay * y);
      }
    else if( 180 < MainInfo->Angle && MainInfo->Angle <= 360)
      {
      leftMotor -> dir = MOTOR_LEFT_BACKWARD;
      rightMotor -> dir = MOTOR_RIGHT_BACKWARD;
      x = (float)(MainInfo->Angle /360.0);
      y = (float)(1 - (MainInfo->Angle/360.0));
      leftMotor -> delay = (unsigned long)(y * leftMotor->delay);
      rightMotor -> delay = (unsigned long)(x * rightMotor->delay);
      }
    }
  }
   


void handling(AngleSpeed *MainInfo){
  server.on("/body", [MainInfo](){   
     if (server.hasArg("plain")== false){ //Check if body received

      server.send(200, "text/plain", "Body not received");
      return;
    }
    String message = "Body received:\n";
       message += server.arg("plain");
       message += "\n";

    server.send(200, "text/plain", message);
    Serial.println(message);

    StaticJsonBuffer<200> jsonBuffer;
    
    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));

    if(!root.success()){
      Serial.println("parseObject() failed");
      return;
    }
   MainInfo->Speed = root["offset"];
   MainInfo->Angle = root["degrees"];  
  });
}  

void handleUturn(MotorInfo *leftMotor , MotorInfo *rightMotor){
   server.on("/uturn", [=](){
     leftMotor->steps = 0;
     rightMotor->steps = 0;   
     Uturn(leftMotor ,rightMotor);
  });
  return;
}
  

// the loop function runs over and over again forever
void loop() {

    motorStep(&leftMotorInfo);
    yield();
    //Serial.println("got things to show");
    motorStep(&rightMotorInfo);
    yield();
    server.handleClient();
    yield();
    Calculation(&instruction , &leftMotorInfo ,&rightMotorInfo);
    yield();
}
