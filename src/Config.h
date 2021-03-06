


#define Log            //serial logging of various important info

//#define USE_BATTERY   //if battery is used, else power supply is used

//#define USE_ESP       //if a esp8266 is used, if not teensy is controlled by usb serial monitor

#define USE_TEENSY1      //right body parts control
//#define USE_TEENSY2      //left body parts control

//#define USE_MOTORS    //if motor drivers can be turned on, and will be at setup

//enabled sensors
//#define USE_IMU          //inertial measurment unit
//#define USE_TOF          //time of flight sensor
//#define USE_VIBR         //vibration sensor
#define USE_ENC          //magnetic encoders, useful only if teensy 1 or 2 are also enabled





//CONDITIONALS CONFIGS

bool Log_ = true;   //controllable variable to turn on off the serial log
#ifdef Log
    #define SerialLog Serial  //serial channel to use for Log
    #define LogPrintln(x) if(Log_){SerialLog.println(x);}
    #define LogPrint(x) if(Log_){SerialLog.print(x);}
#else
    #define LogPrintln(x)
    #define LogPrint(x)
#endif

#ifdef USE_ESP
    #define SerialGcode Serial3 //serial channel to use for gcode communication
#else
    #define SerialGcode Serial
#endif

#ifdef USE_TEENSY1
    #define SerialT1 Serial4    //serial channels to use for inter-teensy communications
#else

#endif
#ifdef USE_TEENSY2
    #define SerialT2 Serial5
#else

#endif

//conditions to enable I2C comm
#ifdef USE_IMU
    #define USE_I2C
#endif

//Log subconfigs
#ifdef Log
    //#define Log_SerialCommPing      //pings the serial port periodically from the SerialComm task
    #define Log_SerialGcodeEcho     //echo messages on the SerialGcode channel
    #define Log_GcodeMonitoring     //various noteworthy gcode events in the SerialComm task
    #define Log_GcodeLifeCycle      //detailed events about the actual GTargets and MotionBlocks
    #define Log_SendMotorControl    //any motorcontrol sent to the teensies is logged
    #define Log_IMUsetup    //setup process of imu
    #define Log_ENCdata     //in the sensesitive task loop, print live encoder values
#endif


//DEBUG 
//#define TeensySlaves_SendTest    //sending in the actuating task loop testing data to the teensy





//start importing files
#include <Arduino.h>
#include <FreeRTOS_TEENSY4.h>

#include <Pinout.h>

#include <utils/General.h>
#include <utils/LinAlgebra.h>
#include <utils/PWMReader.h>

#include <JointStructure.h>
#include <Globals.h>
#include <Gcode.h>
#include <MotionBlock.h>

#ifdef USE_I2C
#include "Wire.h"
#include "I2Cdev.h"
#endif





//enabled sensors files
#ifdef USE_IMU
    #include <sensors/IMU.h>
#endif
#ifdef USE_TOF
    #include <sensors/TOF.h>
#endif
#ifdef USE_VIBR
    #include <sensors/VIBR.h>
#endif
#ifdef USE_ENC
    #include <sensors/ENC.h>
#endif





//Low level configurations for the motion architecture (for GTarget and MotionBlock)

