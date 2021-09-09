//the Gcodes are defined by a byte number at the start of the code,
//  and an optional number of float parameters after the letters,
//  paramaters are referenced by #0,#1,#2, and are separated by '_'
//example: "4_1_-56.2" means: gcode 4 with params #0 = 1 and #1 = -56.2


//parse parameter p for gcode g
float GcodeParseFloat(String g, byte p){
    //starting from [0], find index of next '_', for p times
    byte n=0;
    for(byte i=0; i<p+1; i++){
        n = g.indexOf("_",n)+1;
        if(n==0){ return FLOATNOTDEF; }
    }
    //if indeed found a '_' after p steps, find last index of content befor next '_' or end
    byte m=n;
    while(m<g.length()){ if(g[m]!='_'){ m++; }else{ break; }}
    if(m>n){ return (g.substring(n,m)).toFloat(); }
    else{ return FLOATNOTDEF; }
}
byte GcodeParseByte(String g, byte p){
    //starting from [0], find index of next '_', for p times
    byte n=0;
    for(byte i=0; i<p+1; i++){
        n = g.indexOf("_",n)+1;
        if(n==0){ return BYTENOTDEF; }
    }
    //if indeed found a '_' after p steps, find last index of content befor next '_' or end
    byte m=n;
    while(m<g.length()){ if(g[m]!='_'){ m++; }else{ break; }}
    if(m>n){ return (byte)(g.substring(n,m)).toInt(); }
    else{ return BYTENOTDEF; }
}



//max string length of a gcode
const uint8_t GcodeMaxLength = 20;
//how many gcodes currently supported
const uint8_t GcodesSize = 10;


//the byte representing the specific code refers to the following dictionary:
//gcode name to byte
const uint8_t GCODE_CONTROL = 0;
const uint8_t GCODE_MOVEJOINT = 1;
const uint8_t GCODE_STAND = 2;
const uint8_t GCODE_SIT = 3;
const uint8_t GCODE_WALKFORW = 4;
const uint8_t GCODE_WALKFORW_INF = 5;
const uint8_t GCODE_TURNYAW = 6;
const uint8_t GCODE_TURNYAW_INF = 7;
const uint8_t GCODE_RUN_INF = 8;
const uint8_t GCODE_JUMP = 9;
const uint8_t GCODE_NOTDEF = 10;
//gcode byte to name
const String GCODE_DICT[GcodesSize+1] = {
	"CONTROL",
	"MOVEJOINT",
	"STAND",
	"SIT",
	"WALKFORW",
	"WALKFORW_INF",
	"TURNYAW",
	"TURNYAW_INF",
	"RUN_INF",
	"JUMP",
	
	"NOTDEF"
};






//GTarget objects definitions

//object containing parameters for any gtarget
class GTargetParams{
    public:
	uint8_t b0 = BYTENOTDEF;
	uint8_t b1 = BYTENOTDEF;
	float f0 = FLOATNOTDEF;
	float f1 = FLOATNOTDEF;
    GTargetParams(){ };
    GTargetParams(uint8_t b0_){ b0 = b0_; }
    GTargetParams(float f0_){ f0 = f0_; }
    GTargetParams(float f0_, float f1_){ f0 = f0_; f1 = f1_; }
    GTargetParams(uint8_t b0_, uint8_t b1_, float f0_, float f1_){ b0 = b0_; b1 = b1_; f0 = f0_; f1 = f1_; }
};
//undefined params object
GTargetParams* GTARGETPARAMS_NOTDEF = new GTargetParams();

//target cspace goal parameters, set when a target enters execution in the actuating task
class GTargetGoals{
    public:
    uint8_t b0 = BYTENOTDEF;
	uint8_t b1 = BYTENOTDEF;
	float f0 = FLOATNOTDEF;
	float f1 = FLOATNOTDEF;
    GTargetGoals(){ }
    GTargetGoals(GTargetParams* p){ b0 = p->b0; b1 = p->b1; f0 = p->f0; f1 = p->f1; }
    GTargetGoals(uint8_t b0_){ b0 = b0_; }
    GTargetGoals(float f0_){ f0 = f0_; }
    GTargetGoals(float f0_, float f1_){ f0 = f0_; f1 = f1_; }
    GTargetGoals(uint8_t b0_, uint8_t b1_, float f0_, float f1_){ b0 = b0_; b1 = b1_; f0 = f0_; f1 = f1_; }
};
//undefined goals object
GTargetGoals* GTARGETGOALS_NOTDEF = new GTargetGoals();


//higher level object derived from gcode string
class GTarget{
    public:
	String gcode_string = GCODE_DICT[GCODE_NOTDEF];
    String gcode_string2 = "...";   //for debug
	uint8_t gcode = GCODE_NOTDEF;
	
	GTargetParams* params = GTARGETPARAMS_NOTDEF;
	GTargetGoals* goals = GTARGETGOALS_NOTDEF;
	
    //if at next call of Finished (i.e. after execution of the current block) the target is finished
	bool BlocksFinished = false;
	
	GTarget(String s){
		gcode_string = s;
		
		//parse code
        uint8_t m = 0;
        while(m<s.length()){ if(s[m]!='_'){ m++; }else{ break; }}
        if(m>0){
            int i = s.substring(0,m).toInt();
            if(i>=0 && i<GcodesSize){ gcode = i; }
        }else{ gcode = GCODE_NOTDEF; }

        //create params object
        if(gcode == GCODE_CONTROL){
            //b0 - control
            params = new GTargetParams(GcodeParseByte(gcode_string,0));
        }
        else if(gcode == GCODE_MOVEJOINT){
            //b0 - joint, b1 - cntrlMode, f0 - time, f1 - cntrlTarget
            params = new GTargetParams(GcodeParseByte(gcode_string,0),GcodeParseByte(gcode_string,1),GcodeParseFloat(gcode_string,2),GcodeParseFloat(gcode_string,3));
        }
        else if(gcode == GCODE_STAND){
            //b0 - INF, f0 - height, f1 - lambda
            params = new GTargetParams(GcodeParseByte(gcode_string,0), BYTENOTDEF, GcodeParseFloat(gcode_string,1),GcodeParseFloat(gcode_string,2));
        }
        else{ //GCODE_NOTDEF
            params = GTARGETPARAMS_NOTDEF;
        }

        #ifdef Log_GcodeLifeCycle
        gcode_string2 = GCODE_DICT[gcode];
        gcode_string2 += ", "+(params->b0 != BYTENOTDEF)?String(params->b0):"/";
        gcode_string2 += ", "+(params->b1 != BYTENOTDEF)?String(params->b1):"/";
        gcode_string2 += ", "+(params->f0 != BYTENOTDEF)?String(params->f0,3):"/";
        gcode_string2 += ", "+(params->f1 != BYTENOTDEF)?String(params->f1,3):"/";
        LogPrintln("GCycle/ created target: "+gcode_string2);
        #endif
	}
	
    //based on target params, timestamp and given cspace, set goals object
	void SetGoals(uint32_t t, Cspace* C){
        #ifdef Log_GcodeLifeCycle
        LogPrintln("GCycle/ GTarget Set Goals: "+gcode_string2);
        #endif

        if(gcode == GCODE_MOVEJOINT){
            goals = new GTargetGoals(params);
        }
        else if(gcode == GCODE_STAND){
            goals = new GTargetGoals(params);
        }
        else{ //GCODE_NOTDEF or GCODE_CONTROL
            goals = GTARGETGOALS_NOTDEF;
        }
	}


    bool Finished_MOVEJOINT(Cspace* C);
    bool Finished_STAND(Cspace* C);

	bool Finished(Cspace* C){
		if(!BlocksFinished){
			if(gcode == GCODE_MOVEJOINT){
                return Finished_MOVEJOINT(C);
            }
            else if(gcode == GCODE_STAND){
                return Finished_STAND(C);
            }
            else{
                return true;
            }
		}
        else{
			return true;
		}
	}
};
GTarget* GTARGET_NOTDEF = new GTarget(String(GCODE_NOTDEF));

//delete gtarget object and its subobjects from memomry
void DisposeGTarget(GTarget* t){
    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ Disposing target: "+String((int)t));
    #endif

    if(t != GTARGET_NOTDEF){
        if(t->params != GTARGETPARAMS_NOTDEF){ delete t->params; t->params = nullptr; }
        if(t->goals != GTARGETGOALS_NOTDEF){ delete t->goals; t->goals = nullptr; }
        delete t; t = nullptr;
    }
}




//the compatibility matrix
//  [i][j]=true -> gcode i and gcode j can be tried to be executed simultaneously
//  [i][j]=[j][i] -> the matrix is symmetric
//  naturally, a code is never compatible with itself
const static uint8_t GCompatibility[GcodesSize][GcodesSize] = {
  {  0,  1,  1,  1,  1,  1,  1,  1,  1,  1}, //Control
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0}, //MoveJoint
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0}, //Stand
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0}, //Sit
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,	 0}, //WF
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,	 0}, //WF_I
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,	 0}, //TY
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,	 0}, //TY_I
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,	 0}, //Run
  {  1,  0,  0,  0,  0,  0,  0,  0,  0,  0}  //Jump
};
//	 C	  MV  ST  SIT WF  WFI TY TYI	R	J




//Gcode buffer management
const uint8_t GBufferSize = 8;  //max number of rows in the buffer
//minimum number of empty rows at the top of the buffer for it to be considered not full
const uint8_t GBufferFull_rowslimit = 2;
//flag variable, set true on new gcode receive, set false by GExecutingQueue
bool GBufferFull = false;
//the Gcode buffer matrix
//  any incoming gcode from the esp8266 is always placed at the lowest compatible row
//    in this matrix, i.e. the lowest row without any other incompatible code below it
//  the processes that execute gcodes look for codes they can execute
//    in the lowest row of this buffer, if there is a code it is gonna be copied from that process
//    for whatever it needs to do and popped from this buffer
//  using head-tail system to shift the arrays
GTarget* GBuffer[GcodesSize][GBufferSize];



void GCodeSetup(){
    for(byte i=0; i<GcodesSize; i++){
        for(byte j=0; j<GBufferSize; j++){
            GBuffer[i][j] = GTARGET_NOTDEF;
        }
    }
}

void PrintGBuffer(){
    String f = "";
    for(byte j=GBufferSize; j>0; j--){
    for(byte i=0; i<GcodesSize; i++){
        f+="  "+GBuffer[i][j-1]->gcode_string;
    }
    f+="\n";
    }
    LogPrintln(f);
}





//move gcode specific functions

//MOVEJOINT
bool GTarget::Finished_MOVEJOINT(Cspace* C){
    //finished when block generated by planblocks is finished
    return false;
}

//STAND
bool GTarget::Finished_STAND(Cspace* C){
    
    return false;
}


