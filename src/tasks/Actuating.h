
//send to teensies the motorcontrol variable
void SendMotorControl(MotorControlStruct C){
    #ifdef Log_GcodeLifeCycle
    String s = "GCycle/ MotorTarg: ";
    for(uint8_t i=0; i<MotorNumber; i++){
        s += "("+String(i)+","+(C.mode[i]?"ps":"tq")+","+FloatToString(C.val[i])+")";
    }
    LogPrintln(s);
    #endif

    byte *b;
    byte datasendBytes[MotorNumber][4];
    byte cntrlByte = 0b00000000;
    byte parity = 0;
    byte mask;

    #ifdef USE_TEENSY1
    for(byte i=0; i<MotorNumberT1; i++){
        b = (byte *) & C.val[i];
        for(byte j=0; j<4; j++){ datasendBytes[i][j] = b[j]; }
    }
    for(byte k=0; k<MotorNumberT1; k++){
        for(byte i=0; i<4; i++){
            mask = 0b10000000;
            for(byte j=0; j<8; j++){
                if(datasendBytes[k][i] & mask){ parity++; }
                mask>>=1;
            }
        }
    }
    parity &= 0b00000001;

    mask = 0b10000000;
    for(byte k=0; k<MotorNumberT1; k++){
        if(C.mode[k]){ cntrlByte |= mask; }
        mask>>=1;

        for(byte i=0; i<4; i++){
            SerialT1.write(datasendBytes[k][i]);
        }
    }
    cntrlByte |= (checkintValue | parity);
    SerialT1.write(cntrlByte);
    #endif

    #ifdef USE_TEENSY2
    for(byte i=0; i<MotorNumberT2; i++){
        b = (byte *) & C.val[i+MotorNumberT1];
        for(byte j=0; j<4; j++){ datasendBytes[i][j] = b[j]; }
    }
    parity=0;
    for(byte k=0; k<MotorNumberT2; k++){
        for(byte i=0; i<4; i++){
            mask = 0b10000000;
            for(byte j=0; j<8; j++){
                if(datasendBytes[k][i] & mask){ parity++; }
                mask>>=1;
            }
        }
    }
    parity &= 0b00000001;

    mask = 0b10000000;
    cntrlByte = 0b00000000;
    for(byte k=0; k<MotorNumberT2; k++){
        if(C.mode[k+MotorNumberT1]){ cntrlByte |= mask; }
        mask>>=1;

        for(byte i=0; i<4; i++){
            SerialT2.write(datasendBytes[k][i]);
        }
    }
    cntrlByte |= (checkintValue | parity);
    SerialT2.write(cntrlByte);
    #endif
}



//INTER-TASK COMMUNICATION

//queue for sending to the actuating task newly arrived gtargets on the lowest row in the gbuffer
const uint8_t GtoActuatingLength = 8;
QueueHandle_t GtoActuating = xQueueCreate(GtoActuatingLength,sizeof(GTarget*));

//queue containing the GTargets that have entered the TargetsExecuting array in the actuating task
//i.e. the targets that have to be popped from the lowest row in the gbuffer from SerialComm
const uint8_t GExecutingLength = 8;
QueueHandle_t GExecuting = xQueueCreate(GExecutingLength,sizeof(GTarget*));

//queue where different tasks can store the gtargets they have executed
//the serialcomm task then reads this queue and signals the esp that
//  the given codes have been executed
const uint8_t GExecutedLength = 8;
QueueHandle_t GExecuted = xQueueCreate(GExecutedLength,sizeof(GTarget*));





//ACTUATING TASK TARGET MANAGEMENT

//move GTargets in first row of the GBuffer
//pushed by GtoActuating, popped on compatibility with TargetsExecuting array
GTarget* TargetQueue[GcodesSize];
//GTargets currently being executed by the actuating task
//pushed on compatibility by TargetQueue array, popped on target completion
GTarget* TargetsExecuting[GcodesSize];

void ExecutingTarget(GTarget* t){
    #ifdef Log_GcodeLifeCycle
    LogPrintln("Actu/ Executing Target: "+String((int)t)+", "+t->gcode_string2);
    #endif
    //send to GExecuting, push to TargetsExecuting, pop from TargetQueue
    if(xQueueSend(GExecuting,&t,50/portTICK_PERIOD_MS)==pdTRUE){
        TargetsExecuting[t->gcode] = t;
        TargetQueue[t->gcode] = GTARGET_NOTDEF;
    }
}
//called when the task has finished a target
void ExecutedTarget(GTarget* t){
    LogPrintln("Actu/ Executed Target: "+String((int)t)+", "+t->gcode_string2);
    //send to GExecuted, pop from TargetsExecuting
    if(xQueueSend(GExecuted,&t,50/portTICK_PERIOD_MS)==pdTRUE){
        TargetsExecuting[t->gcode] = GTARGET_NOTDEF;
    }
}
//check compatibility with TargetsExecuting and try to push
void TryPushToExecuting(uint32_t t, Cspace* C, GTarget* T){
    bool compatible = true;
    for(byte i=0; i<GcodesSize; i++){
        if(TargetsExecuting[i]!=GTARGET_NOTDEF && !GCompatibility[T->gcode][i]){
            compatible = false; break;
        }
    }
    if(compatible){
        #ifdef Log_GcodeLifeCycle
        LogPrintln("GCycle/ GTarget SetGoals: "+String((int)T)+", "+T->gcode_string2);
        #endif
        T->SetGoals(t, C);
        ExecutingTarget(T);
    }
}




//ACTUATING TASK BLOCK MANAGEMENT

//currently executing block
MotionBlock* BlockExecuting = MOTIONBLOCK_NOTDEF;
//last executed blocks
const uint8_t LastBlocksLength = 6;
uint8_t LastBlocksTail = 0;
MotionBlock* LastBlocks[LastBlocksLength];
void LastBlocksPush(MotionBlock* b){
    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ Pushing to lastblocks: "+String((int)b)+", "+b->block_string);
    #endif
    //blocks are not disposed until they reach the end of LastBlocks
    if(LastBlocks[LastBlocksTail] != MOTIONBLOCK_NOTDEF){
        DisposeMotionBlock(LastBlocks[LastBlocksTail]);
    }
    #ifdef Log_GcodeLifeCycle
    else{
        LogPrintln("GCycle/ MotionBlock not disposed, LastBlocks isn't full");
    }
    #endif
    LastBlocks[LastBlocksTail] = b;
    LastBlocksTail = (LastBlocksTail+1<LastBlocksLength)?LastBlocksTail+1:0;
}


//given the current TargetsExecuting, TargetQueue, LastBlocks arrays,
MotionBlock* PlanBlocks(uint32_t t, Cspace* C){
    MotionBlockParams* params = MOTIONBLOCKPARAMS_NOTDEF;
    MotionBlock* block = MOTIONBLOCK_NOTDEF;

    if(TargetsExecuting[GCODE_MOVEJOINT] != GTARGET_NOTDEF){
        #ifdef Log_GcodeLifeCycle
        LogPrintln("GCycle/ PlanBlocks MOVEJOINT started, timestamp: "+String(t));
        #endif
        //block params = target goals
        params = new MotionBlockParams(TargetsExecuting[GCODE_MOVEJOINT]->goals);
        block = new MotionBlock(BLOCKID_MOVEJOINT,t,params,C);
        #ifdef Log_GcodeLifeCycle
        LogPrintln("GCycle/ created block: "+String((int)block)+", "+block->block_string);
        #endif
        //there is only this one motionblock for MOVEJOINT target
        TargetsExecuting[GCODE_MOVEJOINT]->BlocksFinished = true;
    }

    return block;
}



//if a block return status other than running or finished,
//  next block is determined by this function
MotionBlock* PlanError(uint32_t t, Cspace* C, uint8_t status){
    MotionBlockParams* params = MOTIONBLOCKPARAMS_NOTDEF;
    MotionBlock* block = MOTIONBLOCK_NOTDEF;



    return block;
}





void vTask_Actuating(void* arg) {
  
    LogPrintln("Actuating/ Warmup: "+String(1500)+"ms\n");
    vTaskDelay(1500/portTICK_PERIOD_MS);

    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ MotionBlock not defined: "+String((int)MOTIONBLOCK_NOTDEF));
    LogPrintln("GCycle/ BlockParams not defined: "+String((int)MOTIONBLOCKPARAMS_NOTDEF));
    #endif
    
    //timemark of the previous loop
    uint32_t presentT = micros();
    uint32_t previousT = presentT;
    float elapsedT = 0;
    
    //initialise target queues
    for(byte i=0; i<GcodesSize; i++){
        TargetQueue[i] = GTARGET_NOTDEF;
        TargetsExecuting[i] = GTARGET_NOTDEF;
    }
    //initialise lastblocks
    for(byte i=0; i<LastBlocksLength; i++){ LastBlocks[i] = MOTIONBLOCK_NOTDEF; }



    //motor control variable sent to the teensies
    MotorControlStruct MotorControlTarget;
    for(byte i=0; i<MotorNumber; i++){ MotorControlTarget.val[i] = 0; MotorControlTarget.mode[i] = 0; }
    
    while (1) { 

        presentT = micros();
        elapsedT = float(presentT - previousT);
        previousT = presentT;



        //taking latest WorldCspace available, remember it has an expiring time (details at WorldCspace definition)
        Cspace* Cnow = WorldCspace[WorldCspaceTail];

        uint8_t BlockStatus = (BlockExecuting==MOTIONBLOCK_NOTDEF)?
            LastBlocks[LastBlocksTail]->LastStatus :    //if no block executing, take exit status value of last block (which shoudn't be 0)
            BlockExecuting->Status(presentT, Cnow);     //if block executing, evaluate current status value

        while(BlockStatus != BLOCKSTATUS_RUNNING){

            //if there is a block executing, and in any case it is not running
            if(BlockExecuting != MOTIONBLOCK_NOTDEF){
                //clear existing block and push it to LastBlocks
                LastBlocksPush(BlockExecuting); BlockExecuting = MOTIONBLOCK_NOTDEF;
            }

            //if there is no block executing
            if(BlockStatus == BLOCKSTATUS_FINISHED){
                //if block finished successfully start standard procedure to determine the next block

                //check if any target has finished
                //in that case ExecutedTarget will automatically shift compatible elements to TargetsExecuting
                bool executed = false;
                for(uint8_t i=0; i<GcodesSize; i++){
                    if(TargetsExecuting[i] != GTARGET_NOTDEF){ if(TargetsExecuting[i]->Finished(Cnow)){
                        ExecutedTarget(TargetsExecuting[i]);
                        executed = true;
                    }}
                }

                //if has any target has now finished
                if(executed){
                    //check compatibility of TargetQueue with TargetsExecuting
                    for(byte j=0; j<GcodesSize; j++){
                        if(TargetQueue[j] != GTARGET_NOTDEF){
                            TryPushToExecuting(presentT, Cnow, TargetQueue[j]);
                        }
                    }

                    //let SerialComm run immediatly, so that since some targets entered execution (i.e. exited row 0 of the GBuffer)
                    //  it can check for compatibility on row 0 and send over any new compatible targets
                    xSemaphoreGive(Task_SerialComm_Semaphore);
                    //CHECK IF THIS DOES INDEED WORK, CAUSE NOW ITS JUST A RACE CONDITION
                }

                //check if any new targets want to enter TargetQueue
                while(uxQueueMessagesWaiting(GtoActuating)>0){
                    GTarget* Targ;
                    if(xQueueReceive(GtoActuating,&Targ,10/portTICK_PERIOD_MS) == pdTRUE){
                        #ifdef Log_GcodeLifeCycle
                        LogPrintln("GCycle/ QueueRecv GtoActuating: "+String((int)Targ)+", "+Targ->gcode_string2);
                        #endif
                        TargetQueue[Targ->gcode] = Targ;

                        TryPushToExecuting(presentT, Cnow, Targ);
                    }
                }

                BlockExecuting = PlanBlocks(presentT, Cnow);
            }
            else{
                //if block had an error
                BlockExecuting = PlanError(presentT, Cnow, BlockStatus);
            }

            //if a no new block entered directly execution, break from while loop to continue with the task loop
            if(BlockExecuting == MOTIONBLOCK_NOTDEF){
                break;
            }
            else{
                //if nee block was actually created, evaluate its status and rerun while loop (if status != 0)
                BlockStatus = BlockExecuting->Status(presentT, Cnow);
            }
        }

        //after all this jazz
        //if existing block is runnning, send control parameters to secondary teensies
        if(BlockStatus == 0 && BlockExecuting != MOTIONBLOCK_NOTDEF){
            MotorControlTarget = BlockExecuting->BlockControl(Cnow);
            SendMotorControl(MotorControlTarget);
        }


        #ifdef TeensySlaves_SendTest
        //test sending targets
        for(byte i=0; i<MotorNumber; i++){
            MotorControlTarget[i] += 0.1*(i+1);
            MotorModeTarget[i] = !MotorModeTarget[i];
        }
        SendMotorTarget(MotorControlTarget,MotorModeTarget);
        #endif


        xSemaphoreTake(Task_Actuating_Semaphore,ActuatingDelay/portTICK_PERIOD_MS);
    }
  
}
