//vl53l1x setup for the sensitive task

#define TOF_minD 0.12           //upper/lower limits set arbitrarily
#define TOF_maxD 0.8
#define TOF_trasfM 9.533021e-05 //coefficients found with Colab
#define TOF_trasfQ 0.002841
#define TOF_avrgdepth 5         //educated guess
#define TOF_noiseCap 300

//convert pulse high time to distance
inline float TOFtoDistance(float t){
    return max(TOF_minD, min(TOF_maxD, (t * TOF_trasfM + TOF_trasfQ)));
}


//pwm reader (samples average depth, hightime change per pulse limit)
PWMReader TOF1 = PWMReader(TOF_avrgdepth,TOF_noiseCap);
PWMReader TOF2 = PWMReader(TOF_avrgdepth,TOF_noiseCap);
PWMReader TOF3 = PWMReader(TOF_avrgdepth,TOF_noiseCap);
PWMReader TOF4 = PWMReader(TOF_avrgdepth,TOF_noiseCap);

void TOF1_ISR(){
    if(digitalRead(TOF1pin)){ TOF1.On(); }
    else{ TOF1.Off(); }
}
void TOF2_ISR(){
    if(digitalRead(TOF2pin)){ TOF2.On(); }
    else{ TOF2.Off(); }
}
void TOF3_ISR(){
    if(digitalRead(TOF3pin)){ TOF3.On(); }
    else{ TOF3.Off(); }
}
void TOF4_ISR(){
    if(digitalRead(TOF4pin)){ TOF4.On(); }
    else{ TOF4.Off(); }
}




void TOFsetup(){

    attachInterrupt(digitalPinToInterrupt(TOF1pin),TOF1_ISR,CHANGE);
    attachInterrupt(digitalPinToInterrupt(TOF2pin),TOF2_ISR,CHANGE);
    attachInterrupt(digitalPinToInterrupt(TOF3pin),TOF3_ISR,CHANGE);
    attachInterrupt(digitalPinToInterrupt(TOF4pin),TOF4_ISR,CHANGE);
}

void TOFupdate(float TOF_D[]){
    //update global variables
    TOF_D[0] = TOFtoDistance(TOF1.HighTime());
    TOF_D[1] = TOFtoDistance(TOF2.HighTime());
    TOF_D[2] = TOFtoDistance(TOF3.HighTime());
    TOF_D[3] = TOFtoDistance(TOF4.HighTime());
}