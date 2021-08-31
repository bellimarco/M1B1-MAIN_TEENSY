
//send to teensies the motorcontrol variable
void SendMotorControl(MotorControlStruct C){
    //!!!remember argument array must have length = MotorNumber

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




//move GTargets in first row of the GBuffer
//pushed by GtoActuating, popped on compatibility with TargetsExecuting array
GTarget* TargetQueue[GcodesSize];
//GTargets currently being executed by the actuating task
//pushed on compatibility by TargetQueue array, popped on target completion
GTarget* TargetsExecuting[GcodesSize];

void ExecutingTarget(GTarget* t){
    t->SetGoals(WorldCspace[WorldCspaceTail]);
    //send to GExecuting, push to TargetsExecuting, pop from TargetQueue
    if(xQueueSend(GExecuting,t,50/portTICK_PERIOD_MS)==pdTRUE){
        TargetsExecuting[t->gcode] = t;
        TargetQueue[t->gcode] = GTARGET_NOTDEF;
    }
}
//called when the task has finished a target
void ExecutedTarget(GTarget* t){
    //send to GExecuted, pop from TargetsExecuting
    if(xQueueSend(GExecuted,t,50/portTICK_PERIOD_MS)==pdTRUE){
        TargetsExecuting[t->gcode] = GTARGET_NOTDEF;

        //check compatibility of TargetQueue with TargetsExecuting
        for(byte j=0; j<GcodesSize; j++){
            if(TargetQueue[j] != GTARGET_NOTDEF){
                bool compatible = true;
                for(byte i=0; i<GcodesSize; i++){
                    if(TargetsExecuting[i]!=GTARGET_NOTDEF && !GCompatibility[j][i]){
                        compatible = false;
                        break;
                    }
                }
                if(compatible){ ExecutingTarget(TargetQueue[j]); }
            }
        }
    }
}


//currently executing block
MotionBlock* BlockExecuting = MOTIONBLOCK_NOTDEF;
//last executed blocks
const uint8_t LastBlocksLength = 6;
uint8_t LastBlocksTail = 0;
MotionBlock* LastBlocks[LastBlocksLength];
void LastBlocksPush(MotionBlock* b){
    DisposeMotionBlock(LastBlocks[LastBlocksTail]);
    LastBlocks[LastBlocksTail] = b;
    LastBlocksTail = (LastBlocksTail+1<LastBlocksLength)?LastBlocksTail+1:0;
}


void vTask_Actuating(void* arg) {
  
    LogPrintln("Actuating/ Warmup: "+String(1500)+"ms\n");
    vTaskDelay(1500/portTICK_PERIOD_MS);
    
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

        if(BlockExecuting != MOTIONBLOCK_NOTDEF){
            uint8_t s = BlockExecuting->Status(presentT, WorldCspace[WorldCspaceTail]);

            
        }

        //new codes in the first row of the GBuffer
        while(uxQueueMessagesWaiting(GtoActuating)>0){
            GTarget* Targ = GTARGET_NOTDEF;
            if(xQueueReceive(GtoActuating,Targ,10/portTICK_PERIOD_MS) == pdTRUE){
                
                TargetQueue[Targ->gcode] = Targ;

                //check compatibility with TargetsExecuting
                bool compatible = true;
                for(byte i=0; i<GcodesSize; i++){
                    if(TargetsExecuting[i]!=GTARGET_NOTDEF && !GCompatibility[Targ->gcode][i]){
                        compatible = false;
                        break;
                    }
                }
                if(compatible){ ExecutingTarget(Targ); }
            }
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
