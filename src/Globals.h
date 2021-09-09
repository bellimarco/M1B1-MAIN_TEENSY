//global variables related to the program,
//also a place to mention important notes,
//like hardcoded variables throughout the program to not lose track of'em



bool MotorDriversEnabled = false;
void MotorDriversEnable(){
    MotorDriversEnabled = true;
    #ifdef USE_MOTORS
    digitalWrite(DRIVE_EN1_pin,HIGH);
    digitalWrite(DRIVE_EN2_pin,HIGH);
    digitalWrite(DRIVE_EN3_pin,HIGH);
    digitalWrite(DRIVE_EN4_pin,HIGH);
    #endif
}
void MotorDriversDisable(){
    MotorDriversEnabled = false;
    #ifdef USE_MOTORS
    digitalWrite(DRIVE_EN1_pin,LOW);
    digitalWrite(DRIVE_EN2_pin,LOW);
    digitalWrite(DRIVE_EN3_pin,LOW);
    digitalWrite(DRIVE_EN4_pin,LOW);
    #endif
}



//battery management
//battery reading is managed by SerialComm
//ControlCode 2 prints battery charge to the serial Log port

//current battery charge from 0 to 1
float BatteryCharge = 1;
//if Battery switches to empty, motors are disabled
//  and communication with teensies is discountinued
bool BatteryEmpty = false;
//divider constant * Vref / analog resolution
const float BatteryK = 6.01511 * 3.3 /1024;
//4p LIPO battery charge map: 50% -> 14.8 Volts, 0% -> 12.8V, 100% -> 16.8V
float BatteryVoltageToCharge(float V){ return (V-12.8)/4; }

void UpdateBatteryCharge(){
    #ifdef USE_BATTERY
    BatteryCharge = BatteryVoltageToCharge(analogRead(BATTERYpin)*BatteryK);

    if(BatteryCharge < 0.05){
        if(!BatteryEmpty){
            BatteryEmpty = true;

            MotorDriversDisable();
            LogPrintln("Battery Empty, Motor Drivers Disabled");
        }
    }
    else if(BatteryCharge < 0.1){
        LogPrintln("Battery Charge Very Low");
    }else if(BatteryCharge < 0.25){
        LogPrintln("Battery Charge Low");
    }



    #else
    BatteryCharge = 1;
    #endif
}




//how many motors, a bit vague but anyways useful throughout
//if this is changed, remember considering:
//  TeensyReader configurations (receiving MotorNumberT1*2 values from teensy 1)
//  Sensitive/ENCupdate() assumes TeensyReader.val[] encoded as [angles,velocities] for total length of MotorNumberT1*2
//  teensy writing configurations (Actuating/SendMotorTarget array arguments and are 
//      assumed to be encoded in [Teensy1values,Teensy2values] for a total length of Motor#T1+Motor#T2=Motor#)
//  teensy communication as is now supports max 4 motors, for the motor modes to fit in the cntrlByte
#define MotorNumber 8
#define MotorNumberT1 4 //on teensy 1
#define MotorNumberT2 4 //on teensy 2
//motor encoded from 0 to 8:
//  on teensy 1:                               on teensy 2:
//  0           1           2       3           4           5       6       7
//  hip1right, hip2right, legright, kneeright, hip1left, hip2left, legleft, kneeleft


//motor control variable, encoded as above
struct MotorControlStruct{
    float val[MotorNumber];
    uint8_t mode[MotorNumber];
};
//when motorcontrol is not defined, just send 0 torque target
MotorControlStruct MOTORCONTROL_NOTDEF = MotorControlStruct{
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0}
};

//inter-teensy serial communication
#define checkintMask 0b11111110
#define checkintValue 0b00001010
#define  parityMask  0b00000001



#define TOFsensors 4
#define VIBRsensors 2




class Cspace{
    public:


    Cspace(){

    }
};

Cspace* CSPACE_NOTDEF = new Cspace();

void DisposeCspace(Cspace* c){
    if(c != CSPACE_NOTDEF){

        delete c; c = nullptr;
    }
}


//worldCspaces are created by the sensitive task and pushed to this array
//Cspaces that become outdated are delete by the push function
//this way any task can access the most recent Cspace at any time,
//  only thing, it must check if it didn't outdate by checking if pointer became nullptr
//  i.e. it must finish using the WorldCspace[Tail] in SenseDelay*WorldCspaceLength milliseconds
const uint8_t WorldCspaceLength = 25;
uint8_t WorldCspaceTail = 0;
Cspace* WorldCspace[WorldCspaceLength];  //array should now only contain null pointers

void WorldCspacePush(Cspace* c){
    if(WorldCspace[WorldCspaceTail] != nullptr){ DisposeCspace(WorldCspace[WorldCspaceTail]); }
    WorldCspace[WorldCspaceTail] = c;
    WorldCspaceTail = (WorldCspaceTail+1<WorldCspaceLength)?WorldCspaceTail+1:0;
}