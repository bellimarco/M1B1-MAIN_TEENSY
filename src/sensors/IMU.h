//mpu6050 setup for the sensitive task


#include "MPU6050_6Axis_MotionApps20.h"

const float int12ToAccel = 9.81/4096;

//the library uses theese datatypes to compute
Quaternion q;           // [w, x, y, z]     quaternion container
VectorInt16 aa;         // [x, y, z]        accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]        gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]        world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]        gravity vector
float ypr[3];           // [yaw,pitch,roll] yaw/pitch/roll container and gravity vector

//the raw acceleration signal is very noisy, so i do a quick arithmetic average
//final acceleration smoothing, through decreasing weighted average
//some stability is gained obviously at the cost of reaction time
const byte MPU_accelAvrgDepth = 5;
const byte MPU_accelAvrgDepthSum = 15;
int MPU_accelAvrg[MPU_accelAvrgDepth][3];
byte MPU_accelAvrgHead = 0;
const float accelAvrgToFloat = int12ToAccel/(float)(MPU_accelAvrgDepthSum);

//MPU6050 library stuff:

//before using any dmp method, always check dmpReady==true
//functions to update the orientation/acceleration vars
//get latest packet:   mpu.dmpGetCurrentFIFOPacket(fifoBuffer)
//  returns 0 if no packets available
//get quaternions:     mpu.dmpGetQuaternion(&q, fifoBuffer);
//get euler angles:    mpu.dmpGetEuler(euler, &q);
//get gravity vector:  mpu.dmpGetGravity(&gravity, &q);
//get ypr:             mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
//get measured accel:  mpu.dmpGetAccel(&aa, fifoBuffer);
//get real accel:      mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
//get world accel:     mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

// class default I2C address is 0x68
MPU6050 mpu;
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer


//setup mpu, calibrate, enable dmp
void IMUsetup(){

  //acceleration average setup
  for(byte i=0; i<MPU_accelAvrgDepth; i++){ Vnullify(MPU_accelAvrg[i]); }
  
  #ifdef Log_IMUsetup
  Serial.print("IMUsetup/ mpu initialising...");
  #endif
  mpu.initialize();
  #ifdef Log_IMUsetup
  Serial.println("\tdone");
  #endif

  #ifdef Log_IMUsetup
  Serial.print("IMUsetup/ mpu test connection...");
  #endif
  bool flag = mpu.testConnection();
  #ifdef Log_IMUsetup
  Serial.println("\tdone: "+String(flag?"success":"fail"));
  #endif
  

  #ifdef Log_IMUsetup
  Serial.print("IMUsetup/ dmp initialising...");
  #endif
  // load and configure the DMP
  devStatus = mpu.dmpInitialize();
  #ifdef Log_IMUsetup
  Serial.println("devStatus: "+String(devStatus));
  #endif

  //last gathered optimal offsets
  mpu.setXAccelOffset(-4651);
  mpu.setYAccelOffset(-2422);
  mpu.setZAccelOffset(889);
  mpu.setXGyroOffset(214);
  mpu.setYGyroOffset(9);
  mpu.setZGyroOffset(-17);

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    // turn on the DMP, now that it's ready
    #ifdef Log_IMUsetup
    Serial.print("IMUsetup/ enabling DMP...");
    #endif
    mpu.setDMPEnabled(true);
    #ifdef Log_IMUsetup
    Serial.println("\tdone");
    #endif

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    #ifdef Log_IMUsetup
    Serial.print("IMUsetup/ DMP fail. ");
    if(devStatus == 1){ Serial.println("initial memory load failed"); }
    else if(devStatus == 2){ Serial.println("DMP configuration updates failed"); }
    else{ Serial.println("unknown fail"); }
    #else
    #ifdef Log
    Serial.println("IMUsetup/ failed initialising dmp");
    #endif
    #endif
  }

}


void IMUupdate(float IMU_Q[], float IMU_G[], float IMU_A[]){
  if(dmpReady && mpu.dmpGetCurrentFIFOPacket(fifoBuffer)){
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

    //update global variables
    IMU_Q[0] = q.w; IMU_Q[1] = q.x; IMU_Q[2] = q.y; IMU_Q[3] = q.z;
    IMU_G[0] = gravity.x; IMU_G[1] = gravity.y; IMU_G[2] = gravity.z;

    MPU_accelAvrgHead = (MPU_accelAvrgHead+1<MPU_accelAvrgDepth)?MPU_accelAvrgHead+1:0;
    MPU_accelAvrg[MPU_accelAvrgHead][0] = aaWorld.x;
    MPU_accelAvrg[MPU_accelAvrgHead][1] = aaWorld.y;
    MPU_accelAvrg[MPU_accelAvrgHead][2] = aaWorld.z;
    // MPU_accelAvrg[MPU_accelAvrgHead][0] = aaReal.x;
    // MPU_accelAvrg[MPU_accelAvrgHead][1] = aaReal.y;
    // MPU_accelAvrg[MPU_accelAvrgHead][2] = aaReal.z;

    Vnullify(IMU_A);
    byte n=0;
    for(byte i=0; i<MPU_accelAvrgDepth; i++){
      n = (MPU_accelAvrgHead-i+1>0)?MPU_accelAvrgHead-i:MPU_accelAvrgDepth+MPU_accelAvrgHead-i;
      Vscale(IMU_A,(float)(MPU_accelAvrgDepth-i),MPU_accelAvrg[n]);
    }
    Vscale(IMU_A,accelAvrgToFloat);
  }
}