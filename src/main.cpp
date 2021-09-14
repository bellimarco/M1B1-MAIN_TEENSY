//import program configs and libraries
#include <Config.h>



TaskHandle_t Task_SerialComm;
TaskHandle_t Task_Sensitive;
TaskHandle_t Task_Actuating;
SemaphoreHandle_t  Task_SerialComm_Semaphore = xSemaphoreCreateCounting(3,0);
SemaphoreHandle_t  Task_Sensitive_Semaphore = xSemaphoreCreateCounting(3,0);
SemaphoreHandle_t  Task_Actuating_Semaphore = xSemaphoreCreateCounting(3,0);
//loop delays for each task
#define SerialCommDelay 100
#define SensitiveDelay 100
#define ActuatingDelay 50
//Task defining files
#include <tasks/Actuating.h>
#include <tasks/Sensitive.h>
#include <tasks/SerialComm.h>





void setup() {
    //setup pins
    Pinout();

    GCodeSetup();

    SerialGcode.begin(115200);
    
    #ifdef USE_TEENSY1
    SerialT1.begin(INTER_TEENSY_BAUDRATE);
    #endif
    #ifdef USE_TEENSY2
    SerialT2.begin(INTER_TEENSY_BAUDRATE);
    #endif

    #ifdef Log
    SerialLog.begin(115200);
    for(byte i=0; i<16; i++){ digitalWrite(LED_BUILTIN,HIGH); delay(13); digitalWrite(LED_BUILTIN,LOW); delay(25); }
    SerialLog.println("Starting Setup:\n");
    #endif

    UpdateBatteryCharge();
    LogPrintln("Setup/ Battery Charge: "+String(BatteryCharge*100,1)+"%");
    MotorDriversEnable();

    // test devices
    char e[]= {'R','E','A','D','Y'};
    byte i=0;

    #ifdef USE_ESP
    LogPrintln("Setup/ Waiting ESP...");
    while(i<5){while(SerialGcode.available()){ if(SerialGcode.read()==e[i]){ i++; }else{i=0;} } } i=0;  //listen fo ready message
    SerialGcode.println();  delay(5);//signal to device that ready message is received
    while(SerialGcode.available()){ SerialGcode.read(); } //flush serial buffer
    LogPrintln("Setup/ ESP Ready!\n");
    for(byte i=0; i<8; i++){ digitalWrite(LED_BUILTIN,HIGH); delay(13); digitalWrite(LED_BUILTIN,LOW); delay(25); }
    #endif


    #ifdef USE_TEENSY1
    LogPrintln("Setup/ Waiting Teensy1...");
    while(i<5){while(SerialT1.available()){ if(SerialT1.read()==e[i]){ i++; }else{ i=0;} } } i=0;
    SerialT1.println(); delay(5);
    while(SerialT1.available()){ SerialT1.read(); }
    LogPrintln("Setup/ Teensy1 Ready!\n");
    for(byte i=0; i<8; i++){ digitalWrite(LED_BUILTIN,HIGH); delay(13); digitalWrite(LED_BUILTIN,LOW); delay(25); }
    #endif

    #ifdef USE_TEENSY2
    LogPrintln("Setup/ Waiting Teensy2...");
    while(i<5){while(SerialT2.available()){ if(SerialT2.read()==e[i]){ i++; }else{ i=0;} } } i=0;
    SerialT2.println(); delay(5);
    while(SerialT2.available()){ SerialT2.read(); }
    LogPrintln("Setup/ Teensy2 Ready!\n");
    for(byte i=0; i<8; i++){ digitalWrite(LED_BUILTIN,HIGH); delay(13); digitalWrite(LED_BUILTIN,LOW); delay(25); }
    #endif



    LogPrintln("Setup/ Starting Sensors");
    
    #ifdef USE_I2C
    Wire.begin();
    Wire.setClock(400000);
    #endif
    #ifdef USE_IMU
    IMUsetup();
    #endif
    #ifdef USE_TOF
    TOFsetup();
    #endif
    #ifdef USE_VIBR
    VIBRsetup();
    #endif
    #ifdef USE_ENC
    ENCsetup();
    #endif



    LogPrintln("\nSetup/ Creating Tasks");

    xTaskCreate(vTask_SerialComm, "SerialComm", 10000, NULL, 5, &Task_SerialComm);
    xTaskCreate(vTask_Sensitive, "Sensitive", 10000, NULL, 5, &Task_Sensitive);
    xTaskCreate(vTask_Actuating, "Actuating", 10000, NULL, 5, &Task_Actuating);


    for(byte i=0; i<16; i++){ digitalWrite(LED_BUILTIN,HIGH); delay(13); digitalWrite(LED_BUILTIN,LOW); delay(25); }
    digitalWrite(LED_BUILTIN,HIGH);
    LogPrintln("Setup Successful, starting scheduler");

    vTaskStartScheduler();
    while(1){}
 }

// loop must never block
void loop() {}