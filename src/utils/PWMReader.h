
//object used to keep track of pwm pulses,
//  to calculate on demand the HighTime of the pulse
//basically, an async version of the arduino pulseIn function

class PWMReader{
  public:
  //caps dt change between pulses to +- noiseCap
  float noiseCap = 0;

  byte avrgdepth = 0;
  byte avrgdepthsum = 0;

  unsigned long t1 = 0;
  byte arrayHead = 0;
  const byte arraysize = 12; //fixed array size, since it can't be set in the constructor
  float dt[12];

  PWMReader(byte n, float noise){
    avrgdepth = n;
    noiseCap = noise;
    for(byte i=1; i<=avrgdepth; i++){
      avrgdepthsum += i;
    }
    for(byte i=0; i<arraysize; i++){
      dt[i] = 0;
    }
  }

  void On(){
    t1 = micros();
  }
  void Off(){
    byte a = arrayHead;
    arrayHead = (arrayHead+1<arraysize)?arrayHead+1:0;
    dt[arrayHead] = max( min( (float)(micros()-t1) ,dt[a]+noiseCap) ,dt[a]-noiseCap);
  }
  float HighTime(){
    float t = 0;
    byte n=0;
    for(byte i=0; i<avrgdepth; i++){
      n = (arrayHead-i+1>0)?arrayHead-i:arraysize+arrayHead-i;
      t += dt[n]*(avrgdepth-i);
    }
    t/=avrgdepthsum;
    return t;
  }
};