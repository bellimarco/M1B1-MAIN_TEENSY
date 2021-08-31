
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



//move GTargets in first row of the GBuffer
//pushed by GtoActuating, popped on compatibility with TargetsExecuting array
GTarget* TargetQueue[GcodesSize];
//GTargets currently being executed by the actuating task
//pushed on compatibility by TargetQueue array, popped on target completion
GTarget* TargetsExecuting[GcodesSize];
//flags if a target upon entering TargetsExecuting couldn't set its goals cause of unavailable Cspace
//this causes the actuatingtask loop to try instead
bool ToSetTargetGoals = false;

void ExecutingTarget(GTarget* t){
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
                if(compatible){
                    //if Cspace available, set goals right now, otherwise loop will retry later
                    if(xSemaphoreTake(WorldCspace_Mutex, 10/portTICK_PERIOD_MS)==pdTRUE){
                        TargetQueue[j]->SetGoals(WorldCspace);
                        xSemaphoreGive(WorldCspace_Mutex);
                    }else{ ToSetTargetGoals = true; }
                    ExecutingTarget(TargetQueue[j]);
                }
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

        //if there are targets with their goals undefined
        if(ToSetTargetGoals){
            if(xSemaphoreTake(WorldCspace_Mutex, 10/portTICK_PERIOD_MS) == pdTRUE){
                for(byte i=0; i<GcodesSize; i++){
                    if(TargetsExecuting[i] != GTARGET_NOTDEF && TargetsExecuting[i]->goals == GTARGETGOALS_NOTDEF){
                        TargetsExecuting[i]->SetGoals(WorldCspace);
                    }
                }
                ToSetTargetGoals = false;   //unflag
                xSemaphoreGive(WorldCspace_Mutex);
            }
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

                if(compatible){
                    //if Cspace available, set goals right now, otherwise loop will retry later
                    if(xSemaphoreTake(WorldCspace_Mutex, 10/portTICK_PERIOD_MS)==pdTRUE){
                        Targ->SetGoals(WorldCspace);
                        xSemaphoreGive(WorldCspace_Mutex);
                    }else{ ToSetTargetGoals = true; }
                    ExecutingTarget(Targ);
                }
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
