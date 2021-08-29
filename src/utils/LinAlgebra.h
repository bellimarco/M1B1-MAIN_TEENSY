


//inplace transformations on vector v
inline void Vnullify(float v[]){
    v[0] = 0; v[1] = 0; v[2] = 0;
}
inline void Vnullify(int v[]){
    v[0] = 0; v[1] = 0; v[2] = 0;
}
inline void Vadd(float v[], float a[]){
    v[0]+=a[0]; v[1]+=a[1]; v[2]+=a[2];
}
inline void Vscale(float v[], float k){
    v[0]*=k; v[1]*=k; v[2]*=k;
}


//set v to operation result between parameters
inline void Vadd(float v[], float a[], float b[]){
    v[0]=a[0]+b[0]; v[1]=a[1]+b[1]; v[2]=a[2]+b[2];
}
inline void Vscale(float v[], float k, float a[]){
    v[0]=k*a[0]; v[1]=k*a[1]; v[2]=k*a[2];
}
inline void Vscale(float v[], float k, int a[]){
    v[0]=k*a[0]; v[1]=k*a[1]; v[2]=k*a[2];
}

//create new array as result of operation between parameters





