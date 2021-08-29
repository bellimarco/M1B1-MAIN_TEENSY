//the Gcodes are defined by two letters at the start of the code,
//  and an optional number of float parameters after the letters,
//  paramaters are often referced by #1,#2,#3, and are separated by '_'
//example: Wi_1_-56.2, CC_90

#define GcodeMaxLength 20

//define the array of all Gcodes supported
#define GcodesSize 9

const static String Gcodes[GcodesSize] = {
    "CC",   //CC_#1 -> general purpose:
            //#=0 -> stop movement execution, #=1-> resume movement execution
    "ST",   //ST_#1 -> (#1=1/0) STAND(maintain vertical idle position)/SIT
    "Wi",   //WALKinfite_#1_#2 -> walk at speed #2 in direction #1(0=forward,1=sideways)
    "Ws",   //WALKstep_#1_#2 -> walk an amount of #2 in direction #1
    "Ri",   //RUNinfinite_#1 -> run forward at speed #1
    "Ti",   //TURNinfinite_#1 -> turn CW at speed #1
    "Ts",   //TURNstep_#1 -> turn CW an amount of #1
    "Js",   //JUMPstep_#1 -> jump an amount of #1
    "MV"    //MOVE_#1_#2 -> move joint #1 in some way, low level code for testing
};







//the compatibility matrix
//  [i][j]=true -> gcode i and gcode j can be tried to be executed simultaneously
//  [i][j]=[j][i] -> the matrix is symmetric
//  naturally, a code is never compatible with itself
const static bool GCompatibility[GcodesSize][GcodesSize] = {
  {  0,  1,  1,  1,  1,  1,  1,  1,  1}, //CC
  {  1,  0,  0,  0,  0,  0,  0,  0,  0}, //ST
  {  1,  0,  0,  0,  0,  1,  1,  1,  0}, //Wi
  {  1,  0,  0,  0,  0,  0,  0,  0,  0}, //Ws
  {  1,  0,  0,  0,  0,  1,  1,  1,  0}, //Ri
  {  1,  0,  1,  0,  1,  0,  0,  0,  0}, //Ti
  {  1,  0,  1,  0,  1,  0,  0,  0,  0}, //Ts
  {  1,  0,  1,  0,  1,  0,  0,  0,  0}, //Js
  {  1,  0,  0,  0,  0,  0,  0,  0,  0}  //MV
};
//  CC  ST  Wi  Ws  Ri  Ti   Ts  Js  MV






//Gcode buffer management
#define GBufferSize 6  //max number of rows in the buffer
bool GBufferFull = false;
//the Gcode buffer matrix
//  any incoming gcode from the esp8266 is always placed at the lowest compatible row
//    in this matrix, i.e. the lowest row without any other incompatible code below it
//  the processes that execute gcodes look for codes they can execute
//    in the lowest row of this buffer, if there is a code it is gonna be copied from that process
//    for whatever it needs to do and popped from this buffer
//  using head-tail system to shift the arrays
String GBuffer[GcodesSize][GBufferSize];


//queue where different tasks can store the gcodes they have executed
//the serialcomm task then reads this queue and signals the esp that
//  the given codes have been executed
//before sending a string make sure its a newly created char array,
//  cause it seems the freertos queue sends strings by reference
#define GExecutedLength 8
QueueHandle_t GExecuted = xQueueCreate(GExecutedLength,GcodeMaxLength);



void GCodeSetup(){
  for(byte i=0; i<GcodesSize; i++){
    for(byte j=0; j<GBufferSize; j++){
      GBuffer[i][j] = "/";
    }
  }
}





#define GcodeParse_NULL 123456789
//parse parameter p for gcode g
float GcodeParse(String g, byte p){
    if(g.length()>3){
        //starting from [0], find index of next '_', for p times
        byte n=0;
        for(byte i=0; i<p+1; i++){
            n = g.indexOf("_",n)+1;
            if(n==0){ break; }
        }
        //if indeed found a '_' after p steps, find last index of content befor next '_' or end
        byte m=n;
        if(n>2){
            while(m<g.length() && g[m]!='_'){ m++; }
            return (g.substring(n,m)).toFloat();
        }else{
            return GcodeParse_NULL;
        }
    }else{
        return GcodeParse_NULL;
    }
}
//same as above, but with a float pointer copy the parsed parameter (if successfull)
void GcodeParse(float * f, String g, byte p){
    if(g.length()>3){
        //starting from [0], find index of next '_', for p times
        byte n=0;
        for(byte i=0; i<p+1; i++){
            n = g.indexOf("_",n)+1;
            if(n==0){ break; }
        }
        //if indeed found a '_' after p steps, find last index of content befor next '_' or end
        byte m=n;
        if(n>2){
            while(m<g.length() && g[m]!='_'){ m++; }

            *f = (g.substring(n,m)).toFloat();  //copy the value to the passed pointer
        }
    }
}
void GcodeParse(bool * f, String g, byte p){
    if(g.length()>3){
        //starting from [0], find index of next '_', for p times
        byte n=0;
        for(byte i=0; i<p+1; i++){
            n = g.indexOf("_",n)+1;
            if(n==0){ break; }
        }
        //if indeed found a '_' after p steps, find last index of content befor next '_' or end
        byte m=n;
        if(n>2){
            while(m<g.length() && g[m]!='_'){ m++; }

            *f = (g.substring(n,m)).toInt()?true:false;  //copy the value to the passed pointer
        }
    }
}