//default constants that represent undefined values
const uint8_t BYTENOTDEF = 255;
const float FLOATNOTDEF = -123456789.9;


#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}



String FloatToString(float f, uint8_t b = 2){
    float fpowerDec = f;
    int32_t fpowerInt = (int32_t) f;
    uint8_t power = 0;
    while(power<b){ fpowerDec*=10; fpowerInt*=10; power++; }
    String s = String((int32_t) f);
    s += ".";
    s += String( ((int32_t) fpowerDec) - fpowerInt );

    return s;
}