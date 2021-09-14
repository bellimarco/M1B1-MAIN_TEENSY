


void vTask_Sensitive(void* arg) {

    LogPrintln("Sensitive/ Warmup: "+String(1000)+"ms");
    vTaskDelay(1000/portTICK_PERIOD_MS);

    
    //timemark of the previous loop
    uint32_t previousT = micros();
    uint32_t presentT = previousT;
    float elapsedT = 0;
    

    //latest sensory data

    // IMU, values in world reference frame
    float IMU_Q[4] = {1,0,0,0};   //quaternion values
    float IMU_G[3] = {0,0,0};      //gravity vector
    float IMU_A[3] = {0,0,0};      //smoothed worldframe acceleration

    // TOF, distance values in metres
    float TOF_D[TOFsensors];
    for(byte i=0; i<TOFsensors; i++){ TOF_D[i] = 0; }

    // VIBR, pwm high time
    float VIBR_T[VIBRsensors];
    bool VIBR_trigg[VIBRsensors];
    for(byte i=0; i<VIBRsensors; i++){ VIBR_T[i] = 0; VIBR_trigg[i] = false; }

    //Encoders, joint angle and speed value in radians, not homed
    float ENC_RAD[MotorNumber];
    float ENC_RADS[MotorNumber];
    for(byte i=0; i<MotorNumber; i++){ ENC_RAD[i] = 0; ENC_RADS[i] = 0; }

    while (1) {

        
        presentT = micros();
        elapsedT = float(presentT - previousT);
        previousT = presentT;
        

        #ifdef USE_IMU
        IMUupdate(IMU_Q,IMU_G,IMU_A);
        LogPrint(IMU_Q[0]); LogPrint("\t");
        LogPrint(IMU_Q[1]); LogPrint("\t");
        LogPrint(IMU_Q[2]); LogPrint("\t");
        LogPrint(IMU_Q[3]); LogPrint("\n");
        #endif

        #ifdef USE_TOF
        TOFupdate(TOF_D);
        #endif

        #ifdef USE_VIBR
        VIBRupdate(VIBR_T,VIBR_trigg);
        #endif

        #ifdef USE_ENC
        ENCupdate(ENC_RAD,ENC_RADS);

        #ifdef Log_ENCdata
        LogPrint("ENC: (");
        LogPrint(ENC_RAD[0]); LogPrint("  "); LogPrint(ENC_RAD[1]); LogPrint(" ");
        LogPrint(ENC_RAD[2]); LogPrint("  "); LogPrint(ENC_RAD[3]); LogPrint(" ) , ( ");
        LogPrint(ENC_RADS[0]); LogPrint("  "); LogPrint(ENC_RADS[1]); LogPrint("  ");
        LogPrint(ENC_RADS[2]); LogPrint("  "); LogPrint(ENC_RADS[3]); LogPrint(" )\n");
        #endif
        #endif


        xSemaphoreTake(Task_Sensitive_Semaphore,SensitiveDelay/portTICK_PERIOD_MS);
    }

    
}