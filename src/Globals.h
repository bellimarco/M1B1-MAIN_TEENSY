//global variables related to the program,
//also a place to mention important notes,
//like hardcoded variables throughout the program to not lose track of'em

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
MotorControlStruct MOTORCONTROL_NOTDEF = MotorControlStruct{
    {FLOATNOTDEF,FLOATNOTDEF,FLOATNOTDEF,FLOATNOTDEF},
    {BYTENOTDEF,BYTENOTDEF,BYTENOTDEF,BYTENOTDEF}
};

//inter-teensy serial communication
#define checkintMask 0b11111110
#define checkintValue 0b00001010
#define  parityMask  0b00000001



#define TOFsensors 4
#define VIBRsensors 2



SemaphoreHandle_t  WorldCspace_Mutex = xSemaphoreCreateBinary();

class Cspace{
    public:


    Cspace(){

    }
};

Cspace* CSPACE_NOTDEF = new Cspace();

Cspace* WorldCspace = new Cspace();




