
//block type id
const uint8_t BLOCKID_NOTDEF = 0;
const uint8_t BLOCKID_MOVEJOINT = 1;
const uint8_t BLOCKID_STAND = 2;
const uint8_t BLOCKID_WALKFORW_START_R = 3;
const uint8_t BLOCKID_WALKFORW_GOING_R = 4;
const uint8_t BLOCKID_WALKFORW_END_R = 5;
const uint8_t BLOCKID_WALKFORW_START_L = 6;
const uint8_t BLOCKID_WALKFORW_GOING_L = 7;
const uint8_t BLOCKID_WALKFORW_END_L = 8;

const uint8_t BlocksSize = 9;
const String BLOCK_DICT[BlocksSize] = {
    "NOTDEF",
    "MOVEJOINT",
    "STAND",
    "WALKFORW_START_R",
    "WALKFORW_GOING_R",
    "WALKFORW_END_R",
    "WALKFORW_START_L",
    "WALKFORW_GOING_L",
    "WALKFORW_END_L"
};

const uint8_t BLOCKSTATUS_RUNNING = 0;
const uint8_t BLOCKSTATUS_FINISHED = 1;
const uint8_t BLOCKSTATUS_ERROR = 2;      //general block error
const uint8_t BLOCKSTATUS_LOSTSTAND = 3;  //lost standing equilibrium status


//params object of a motionblock
class MotionBlockParams{
    public:
    uint8_t b0 = BYTENOTDEF;
	uint8_t b1 = BYTENOTDEF;
	float f0 = FLOATNOTDEF;
	float f1 = FLOATNOTDEF;
    MotionBlockParams(){ };
    MotionBlockParams(GTargetGoals* g){ b0 = g->b0; b1 = g->b1; f0 = g->f0; f1 = g->f1; }
    MotionBlockParams(uint8_t b0_){ b0 = b0_; }
    MotionBlockParams(float f0_){ f0 = f0_; }
    MotionBlockParams(float f0_, float f1_){ f0 = f0_; f1 = f1_; }
    MotionBlockParams(uint8_t b0_, uint8_t b1_, float f0_, float f1_){ b0 = b0_; b1 = b1_; f0 = f0_; f1 = f1_; }
};
MotionBlockParams* MOTIONBLOCKPARAMS_NOTDEF = new MotionBlockParams();


class MotionBlock{
    public:
    String block_string = "...";    //for debug
    
    uint8_t id = BLOCKID_NOTDEF;
    MotionBlockParams* params = MOTIONBLOCKPARAMS_NOTDEF;

    uint32_t Tstart = 0;    //micros timestamp given at constructor
    float Trun = 0;        //seconds from Tstart, updated manually by Status method
    
    Cspace* Cstart = CSPACE_NOTDEF;

    uint8_t LastStatus = BLOCKSTATUS_FINISHED; //last status evaluated, by default set to Finished

    //Motor controllers for each block type
    //given the current Cspace and Trun, compute MotorControl object to employ
    MotorControlStruct MotorController_MOVEJOINT(Cspace* C);
    MotorControlStruct MotorController_STAND(Cspace* C);

    //the motion control in response to current Cspace difference at current Trun
    MotorControlStruct BlockControl(Cspace* C){
        if(id == BLOCKID_MOVEJOINT){
            return MotorController_MOVEJOINT(C);
        }
        else if(id == BLOCKID_STAND){
            return MotorController_STAND(C);
        }
        else{
            return MOTORCONTROL_NOTDEF;
        }
    }


    //block finished functions for each block type
    bool Finished_MOVEJOINT(Cspace* C);
    bool Finished_STAND(Cspace* C);


    //general error functions, implemented in Status function for particular block types
    bool Error_LostStand(Cspace* C);

    //status of the block given t and C, 0->running, 1->finished, 2->failed
    uint8_t Status(uint32_t t, Cspace* C){
        //update Trun with current timestamp
        Trun = ((float)(t-Tstart))*1e-6;

        //TODO: add conditional to test status run error

        if(id == BLOCKID_MOVEJOINT){
            //no error for movejoint
            LastStatus = Finished_MOVEJOINT(C)?BLOCKSTATUS_FINISHED:BLOCKSTATUS_RUNNING;
        }
        else if(id == BLOCKID_STAND){
            LastStatus = Error_LostStand(C)?BLOCKSTATUS_LOSTSTAND:
                        (Finished_STAND(C)?BLOCKSTATUS_FINISHED:BLOCKSTATUS_RUNNING);
        }
        else{
            LastStatus = BLOCKSTATUS_ERROR;
        }

        return LastStatus;
    }

    MotionBlock(){}
    MotionBlock(uint8_t id_, uint32_t t, MotionBlockParams* p, Cspace* c){
        id = id_; Tstart = t;
        params = p; Cstart = c;
        LastStatus = BLOCKSTATUS_RUNNING;

        block_string = BLOCK_DICT[id];
        block_string += (params->b0 != BYTENOTDEF)?", "+String(params->b0):", /";
        block_string += (params->b1 != BYTENOTDEF)?", "+String(params->b1):", /";
        block_string += (params->f0 != FLOATNOTDEF)?", "+FloatToString(params->f0):", /";
        block_string += (params->f1 != FLOATNOTDEF)?", "+FloatToString(params->f1):", /";
    }
};

MotionBlock* MOTIONBLOCK_NOTDEF = new MotionBlock();

//delete motionblock object and its subobjects from memomry
void DisposeMotionBlock(MotionBlock* b){
    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ Disposing block: "+String((int)b)+", "+b->block_string);
    #endif

    if(b != MOTIONBLOCK_NOTDEF){
        if(b->params != MOTIONBLOCKPARAMS_NOTDEF){ delete b->params; b->params = nullptr; }
        delete b; b = nullptr;
    }
}






//block type specific functions

//MOVEJOINT
MotorControlStruct MotionBlock::MotorController_MOVEJOINT(Cspace* C){
    MotorControlStruct cntrl = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};
    cntrl.mode[params->b0] = params->b1;
    cntrl.val[params->b0] = params->f1;
    return cntrl;
}
bool MotionBlock::Finished_MOVEJOINT(Cspace* C){
    //running time of block greater than exit time set in params
    return Trun > params->f0;
}
//STAND
MotorControlStruct MotionBlock::MotorController_STAND(Cspace* C){

    return MOTORCONTROL_NOTDEF;
}
bool MotionBlock::Finished_STAND(Cspace* C){

    return false;
}



//Block Error Management

//when Cspace is in such a configuration that the STAND controller can't get
//  the robot upright by itself
bool MotionBlock::Error_LostStand(Cspace* C){

    return false;
}