
const uint8_t MOTIONBLOCK_NOTDEF = 0;
const uint8_t MOTIONBLOCK_STAND = 1;



class MotionBlockParams{
    public:
    uint8_t b0 = BYTENOTDEF;
	uint8_t b1 = BYTENOTDEF;
	float f0 = FLOATNOTDEF;
	float f1 = FLOATNOTDEF;
    MotionBlockParams(){ };
    MotionBlockParams(uint8_t b0_){ b0 = b0_; }
    MotionBlockParams(float f0_){ f0 = f0_; }
    MotionBlockParams(float f0_, float f1_){ f0 = f0_; f1 = f1_; }
    MotionBlockParams(uint8_t b0_, uint8_t b1_, float f0_, float f1_){ b0 = b0_; b1 = b1_; f0 = f0_; f1 = f1_; }
};
MotionBlockParams MOTIONBLOCKPARAMS_NOTDEF = MotionBlockParams();

class MotionBlock{
    public:
    
    uint8_t id = MOTIONBLOCK_NOTDEF;
    float T = FLOATNOTDEF;
    MotionBlockParams params = MOTIONBLOCKPARAMS_NOTDEF;
    
    Cspace Cstart = CSPACE_NOTDEF;
    Cspace Ctarget = CSPACE_NOTDEF;

    MotionBlock(uint8_t id_, float T_, MotionBlockParams p, Cspace c){

    }
};