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


const uint8_t BLOCKSTATUS_RUNNING = 0;
const uint8_t BLOCKSTATUS_FINISHED = 1;
const uint8_t BLOCKSTATUS_RUNERROR = 2;

//params object of a motionblock
class MotionBlockParams{
    public:
    uint8_t b0 = BYTENOTDEF;
	uint8_t b1 = BYTENOTDEF;
	float f0 = FLOATNOTDEF;
	float f1 = FLOATNOTDEF;
    MotionBlockParams(){ };
    MotionBlockParams(GTargetGoals* g){ g->b0 = b0; g->b1 = b1; g->f0 = f0; g->f1 = f1; }
    MotionBlockParams(uint8_t b0_){ b0 = b0_; }
    MotionBlockParams(float f0_){ f0 = f0_; }
    MotionBlockParams(float f0_, float f1_){ f0 = f0_; f1 = f1_; }
    MotionBlockParams(uint8_t b0_, uint8_t b1_, float f0_, float f1_){ b0 = b0_; b1 = b1_; f0 = f0_; f1 = f1_; }
};
MotionBlockParams* MOTIONBLOCKPARAMS_NOTDEF = new MotionBlockParams();


class MotionBlock{
    public:
    
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
    bool BlockFinished_MOVEJOINT(Cspace* C);
    bool BlockFinished_STAND(Cspace* C);


    //status of the block given t and C, 0->running, 1->finished, 2->failed
    uint8_t Status(uint32_t t, Cspace* C){
        //update Trun with current timestamp
        Trun = (float) (t-Tstart)*10e-6;

        //TODO: add conditional to test status run error

        if(id == BLOCKID_MOVEJOINT){
            LastStatus = BlockFinished_MOVEJOINT(C)?BLOCKSTATUS_FINISHED:BLOCKSTATUS_RUNNING;
        }
        else if(id == BLOCKID_STAND){
            LastStatus = BlockFinished_STAND(C)?BLOCKSTATUS_FINISHED:BLOCKSTATUS_RUNNING;
        }
        else{
            LastStatus = BLOCKSTATUS_FINISHED;
        }

        return LastStatus;
    }

    MotionBlock(){}
    MotionBlock(uint8_t id_, uint32_t t, MotionBlockParams* p, Cspace* c){
        id = id_; Tstart = t;
        params = p; Cstart = c;
        LastStatus = BLOCKSTATUS_RUNNING;
    }
};

MotionBlock* MOTIONBLOCK_NOTDEF = new MotionBlock();

//delete motionblock object and its subobjects from memomry
void DisposeMotionBlock(MotionBlock* b){
    if(b != MOTIONBLOCK_NOTDEF){
        if(b->params != MOTIONBLOCKPARAMS_NOTDEF){ delete b->params; b->params = nullptr; }
        delete b; b = nullptr;
    }
}





//block type specific functions

//MOVEJOINT
MotorControlStruct MotionBlock::MotorController_MOVEJOINT(Cspace* C){

    return MOTORCONTROL_NOTDEF;
}
bool MotionBlock::BlockFinished_MOVEJOINT(Cspace* C){

    return false;
}
//STAND
MotorControlStruct MotionBlock::MotorController_STAND(Cspace* C){
    
    return MOTORCONTROL_NOTDEF;
}
bool MotionBlock::BlockFinished_STAND(Cspace* C){

    return false;
}


