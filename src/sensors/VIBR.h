//vibration sensor setup for the sensitive task

//!!! still need to calibrate the sensor, to find suitable constants:
#define VIBR_Ttrigger 2000
#define VIBR_avrgdepth 3
#define VIBR_noiseCap 1000

//pwm reader (samples average depth, hightime change per pulse limit)
PWMReader VIBR1 = PWMReader(VIBR_avrgdepth,VIBR_noiseCap);
PWMReader VIBR2 = PWMReader(VIBR_avrgdepth,VIBR_noiseCap);



void VIBR1_ISR(){
    if(digitalRead(VIBR1pin)){ VIBR1.On(); }
    else{ VIBR1.Off(); }
}
void VIBR2_ISR(){
    if(digitalRead(VIBR2pin)){ VIBR2.On(); }
    else{ VIBR2.Off(); }
}

void VIBRsetup(){
    attachInterrupt(digitalPinToInterrupt(VIBR1pin),VIBR1_ISR,CHANGE);
    attachInterrupt(digitalPinToInterrupt(VIBR2pin),VIBR2_ISR,CHANGE);
}

void VIBRupdate(float VIBR_T[], bool VIBR_trigg[]){
    VIBR_T[0] = VIBR1.HighTime();
    VIBR_trigg[0] = VIBR_T[0] > VIBR_Ttrigger;

    VIBR_T[1] = VIBR2.HighTime();
    VIBR_trigg[1] = VIBR_T[1] > VIBR_Ttrigger;

}