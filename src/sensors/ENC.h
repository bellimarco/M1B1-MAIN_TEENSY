
//object to manage serial read on a teensy communication channel,
//designed specifically for my arbitrary protocol (4float values+1control byte)
class TeensyReader{
    public:
    HardwareSerial *port;

    const unsigned long TimeoutT = 50;   //millis
    unsigned long Timeout = 0;        //msgStarted  timestamp

    bool msgStarted = false;
    byte msgBytesReceived = 0;           //bytes already received, 0 to 33
    byte cntrlByte = 0;
    //data bytes received, grouped by 4 for later float conversion
    const byte Values = 8;
    byte dataBytes[8][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    float val[8] = {0,0,0,0,0,0,0,0}; //last received values, updated when new data has been processed

    TeensyReader(HardwareSerial *p){
        port = p;
    }

    void run(){
        while((*port).available()){
            if(!msgStarted){
                msgStarted = true;
                msgBytesReceived = 0;
                Timeout = millis() + TimeoutT;
            }
            if(millis()<Timeout){
                msgBytesReceived++;
                cntrlByte = (*port).read();

                if(msgBytesReceived < Values*4 +1){
                    //group byte at right place in dataBytes array
                    dataBytes[(msgBytesReceived-1)/4][(msgBytesReceived-1)%4] = cntrlByte;
                }else{
                    //check last byte
                    byte parity=0;
                    byte mask;
                    for(byte k=0; k<Values; k++){
                        for(byte i=0; i<4; i++){
                            //for every byte in the Valuesx4 byte array
                            mask = 0b10000000;
                            for(byte j=0; j<8; j++){
                                if(dataBytes[k][i] & mask){ parity++; }
                                mask>>=1;
                            }
                        }
                    }

                    //if everything went well copy data to val array
                    if( ((cntrlByte & checkintMask) == checkintValue) && 
                        (parity & 0b00000001) == (cntrlByte & parityMask)){
                        //cast 4bytearrayreference as floatpointer, then
                        //  dereference that pointer to obtain a float variable
                        for(byte i=0; i<Values; i++){
                            val[i] = * ((float *) &dataBytes[i]);
                        }
                    }

                    //end message
                    msgStarted = false;
                }
            }else{
                //timed out
                msgStarted = false;
            }
        }
    }
};

#ifdef USE_TEENSY1
TeensyReader TeensyReader1 = TeensyReader(&SerialT1);
#endif
#ifdef USE_TEENSY2
TeensyReader TeensyReader2 = TeensyReader(&SerialT2);
#endif

void ENCsetup(){


}

void ENCupdate(float ENC_RAD[], float ENC_RADS[]){

    #ifdef USE_TEENSY1
    TeensyReader1.run();
    for(byte i=0; i<MotorNumberT1; i++){
        ENC_RAD[i] = TeensyReader1.val[i];
        ENC_RADS[i] = TeensyReader1.val[i+MotorNumberT1];
    }
    #endif

    #ifdef USE_TEENSY2
    TeensyReader2.run();
    for(byte i=0; i<MotorNumberT2; i++){
        ENC_RAD[i+MotorNumberT1] = TeensyReader2.val[i];
        ENC_RADS[i+MotorNumberT1] = TeensyReader2.val[i+MotorNumberT2];
    }
    #endif
}