

//when a gtarget reaches row 0 of the gbuffer, it is sent to this function
void GSendToTask(GTarget* Targ){
    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ SendToTask: "+String((int)Targ)+", "+Targ->gcode_string2);
    #endif

    uint8_t g = Targ->gcode;
    //sort the code
    if(
        g == GCODE_MOVEJOINT ||
        g == GCODE_STAND ||
        g == GCODE_SIT ||
        g == GCODE_WALKFORW ||
        g == GCODE_WALKFORW_INF ||
        g == GCODE_TURNYAW ||
        g == GCODE_TURNYAW_INF ||
        g == GCODE_RUN_INF ||
        g == GCODE_JUMP
    ){
        //send move codes to actuating task
        xQueueSend(GtoActuating,&Targ,500/portTICK_PERIOD_MS);
    }
    else if(g == GCODE_CONTROL){
        //control codes are executed here directly
        //send to GExecuting, to pop it off the buffer
        xQueueSend(GExecuting,&Targ,500/portTICK_PERIOD_MS);
        //sort control code
        uint8_t c = Targ->params->b0;
        if(c != BYTENOTDEF){
            if(c==0){ LogPrintln("exec/===Turning off log==="); Log_ = false; }
            else if(c==1){ Log_ = true; LogPrintln("exec/===Turned on log==="); }
            else if(c==2){
                LogPrintln("exec/ Battery Charge: "+String(BatteryCharge*100,1)+"%");
            }
            else{
                LogPrintln("exec/===Control Code not defined===");
            }
        }else{
            LogPrintln("exec/====Control test code=====");
        }

        //after execution directly sent to GExecuted queue
        xQueueSend(GExecuted,&Targ,500/portTICK_PERIOD_MS);
    }
    else{
        //unhandled or undefined gcode, execute immediatly
        LogPrintln("GSentToTask/ unhandled gode: "+Targ->gcode_string);
        xQueueSend(GExecuted,&Targ,500/portTICK_PERIOD_MS);
    }
}


void vTask_SerialComm(void* arg) {

    LogPrintln("SerialComm/ Warmup: "+String(500)+"ms");
    vTaskDelay(500/portTICK_PERIOD_MS);

    #ifdef Log_GcodeLifeCycle
    LogPrintln("GCycle/ GTarget not defined: "+String((int)GTARGET_NOTDEF));
    LogPrintln("GCycle/ GTargetParams not defined: "+String((int)GTARGETPARAMS_NOTDEF));
    LogPrintln("GCycle/ GTargetGoals not defined: "+String((int)GTARGETGOALS_NOTDEF));
    #endif

    uint32_t presentT = micros();

    char c;
    String serialIn = "";

    //if has sent to the esp the signal that the GBuffer is full
    bool sentBufferFullFlag = false;


    while (1) {

        presentT = micros();

        #ifdef Log_SerialCommPing
        if(presentT>Ping_t){
            Ping_t = presentT + PingT;
            LogPrintln("\tPing. timestamp: "+String(presentT)+", memory: "+String(freeMemory()));
        }
        #endif

        //listen for incoming messages
        while(SerialGcode.available()){
            c = SerialGcode.read();
            if(c!='\n'){ serialIn += c; }
            else if(serialIn[0] != '#'){
                //new gcode received
                digitalWrite(LED_BUILTIN,LOW);

                #ifdef Log_GcodeMonitoring
                LogPrintln("SerialComm/ new gcode: "+serialIn);
                #endif
                GTarget* Targ = new GTarget(serialIn);
                #ifdef Log_GcodeLifeCycle
                LogPrintln("GCycle/ created target: "+String((int)Targ)+", "+Targ->gcode_string2);
                #endif

                //sort new target into the GBuffer
                if(Targ->gcode != GCODE_NOTDEF){
                    //start from the highest row, and search the last compatible row
                    byte row = GBufferSize;
                    for(byte j=GBufferSize-1; j>=0; j--){
                        bool compatible = true;
                        for(byte i=0; i<GcodesSize; i++){
                            if(GBuffer[i][j]!=GTARGET_NOTDEF && !GCompatibility[i][Targ->gcode]){
                                compatible = false; break;
                            }
                        }
                        if(compatible){ row = j; }else{ break; }
                    }
                    //if the gbuffer has just two rows left, the GBuffer is flagged full
                    if(row>=GBufferSize-GBufferFull_rowslimit){
                        GBufferFull = true;
                        if(!sentBufferFullFlag){
                            //signal the esp that the GBuffer is full, and it cant send any more gcodes
                            sentBufferFullFlag = true;
                            SerialGcode.println("full"); SerialGcode.println("full");
                            #ifdef Log_GcodeMonitoring
                            LogPrintln("SerialComm/ GBuffer full flag to esp");
                            #endif
                        }
                    }

                    //if code fits into the buffer
                    if(row<GBufferSize){
                        GBuffer[Targ->gcode][row] = Targ;

                        #if defined (Log_GcodeMonitoring) || defined (Log_GcodeLifeCycle)
                        PrintGBuffer();
                        #endif

                        //if the code went to the first row, send it directly to its task
                        if(row==0){  GSendToTask(Targ); }

                        //relay the gcode to the esp for confirmation
                        SerialGcode.println("c/"+serialIn);
                    }else{
                        #if defined (Log_GcodeMonitoring) || defined (Log_GcodeLifeCycle)
                        PrintGBuffer();
                        #endif
                        #ifdef Log_GcodeMonitoring
                        LogPrintln("SerialComm/ GBuffer Full, couldn't push new gcode");
                        #endif
                        DisposeGTarget(Targ);
                    }
                }

                serialIn = "";
                digitalWrite(LED_BUILTIN,HIGH);
            }else{
                //echo message from Gcode device, #+xxxx
                #ifdef Log_SerialGcodeEcho
                LogPrintln("SerialGcode echo/"+serialIn.substring(1));
                #endif
                serialIn = "";
            }
        }
        
        //signal the esp that it can continue to send messages
        if(sentBufferFullFlag && !GBufferFull){
            sentBufferFullFlag = false;
            SerialGcode.println("empty"); SerialGcode.println("empty");
            #ifdef Log_GcodeMonitoring
            LogPrintln("SerialComm/ GBuffer empty flag to esp");
            #endif
        }


        //pop from the GBuffer targets that are executing
        while(uxQueueMessagesWaiting(GExecuting)>0){

            GTarget* Targ;

            if(xQueueReceive(GExecuting,&Targ,10/portTICK_PERIOD_MS) == pdTRUE){
                #ifdef Log_GcodeLifeCycle
                LogPrintln("GCycle/ QueueRecv GExecuting: "+String((int)Targ)+", "+Targ->gcode_string2);
                #endif

                if(Targ != GTARGET_NOTDEF){
                    if(GBuffer[Targ->gcode][0] == Targ){
                        //if target is indeed in the first row of the buffer
                        //erase the code from the first row
                        GBuffer[Targ->gcode][0] = GTARGET_NOTDEF;

                        //check starting from the second row, if codes can be shifted downwards
                        for(uint8_t j=1; j<GBufferSize; j++){
                            //check if any codes in this row, are compatible with the row below
                            for(uint8_t i=0; i<GcodesSize; i++){
                                if(GBuffer[i][j]!=GTARGET_NOTDEF){
                                    
                                    bool canshift=true;
                                    for(uint8_t i2=0; i2<GcodesSize; i2++){
                                        if(GBuffer[i2][j-1]!=GTARGET_NOTDEF && !GCompatibility[i2][i]){
                                            canshift = false; break;
                                        }
                                    }
                                    //if is compatible with row below, shift the code
                                    if(canshift){
                                        GBuffer[i][j-1] = GBuffer[i][j];
                                        GBuffer[i][j]=GTARGET_NOTDEF;
                                        //if it has been shifted from the 2nd to the 1st row send it
                                        if(j==1){  GSendToTask(GBuffer[i][j-1]); }
                                    }
                                }
                            }
                        }
                        //if GBuffer full flag is true, check again if the buffer has space now
                        if(GBufferFull){
                            GBufferFull = false;
                            for(uint8_t i=0; i<GcodesSize; i++){
                                if(GBuffer[i][GBufferSize-GBufferFull_rowslimit] != GTARGET_NOTDEF){
                                    GBufferFull = true;
                                    break;
                                }
                            }
                        }

                        #ifdef Log_GcodeMonitoring
                        LogPrintln("SerialComm/ Executing: "+String((int)Targ)+", "+Targ->gcode_string2);
                        #endif
                    }
                }else{
                    //if the target received in the queue isn't at its place in the first row

                }

                #ifdef Log_GcodeLifeCycle
                PrintGBuffer();
                #endif
            }
        }

        while(uxQueueMessagesWaiting(GExecuted)>0){
            GTarget* Targ;
            if(xQueueReceive(GExecuted,&Targ,10/portTICK_PERIOD_MS) == pdTRUE){
                #ifdef Log_GcodeLifeCycle
                LogPrintln("GCycle/ QueueRecv GExecuted: "+String((int)Targ)+", "+Targ->gcode_string2);
                #endif

                #ifdef Log_GcodeMonitoring
                LogPrintln("SerialComm/ Executed: "+String((int)Targ)+", "+Targ->gcode_string2);
                #endif
                
                //relay to the esp the executed code
                SerialGcode.print("e/");
                SerialGcode.println(Targ->gcode_string);

                DisposeGTarget(Targ);
            }
        }


    UpdateBatteryCharge();

        xSemaphoreTake(Task_SerialComm_Semaphore,SerialCommDelay/portTICK_PERIOD_MS);
    }
}

