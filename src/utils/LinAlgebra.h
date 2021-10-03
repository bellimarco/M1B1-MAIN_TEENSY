
//constants
const float I3[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
const float I4[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
const float v0_4[4] = {0,0,0,1};
const float v0_3[3] = {0,0,0};
const float vx[3] = {1,0,0}; float vy[3] = {0,1,0}; float vz[3] = {0,0,1};


//inplace transformations on vector v
inline void Vnullify(float v[]){
    v[0] = 0; v[1] = 0; v[2] = 0;
}
inline void Vnullify(int v[]){
    v[0] = 0; v[1] = 0; v[2] = 0;
}
inline void Voppose(float v[]){
    v[0]=-v[0]; v[1]=-v[1]; v[2]=-v[2];
}
inline void Vadd(float v[], float a[]){
    v[0]+=a[0]; v[1]+=a[1]; v[2]+=a[2];
}
inline void Vsub(float v[], float a[]){
    v[0]-=a[0]; v[1]-=a[1]; v[2]-=a[2];
}
inline void Vscale(float v[], float k){
    v[0]*=k; v[1]*=k; v[2]*=k;
}


//set v to operation result between parameters
inline void Vcopy(float v[], float a[]){
    v[0] = a[0]; v[1] = a[1]; v[2] = a[2];
}
inline void Vadd(float v[], float a[], float b[]){
    v[0]=a[0]+b[0]; v[1]=a[1]+b[1]; v[2]=a[2]+b[2];
}
inline void Vsub(float v[], float a[], float b[]){
    v[0]=a[0]-b[0]; v[1]=a[1]-b[1]; v[2]=a[2]-b[2];
}
inline void Vscale(float v[], float k, float a[]){
    v[0]=k*a[0]; v[1]=k*a[1]; v[2]=k*a[2];
}
inline void Vcross(float v[], float a[], float b[]){
    v[0] = a[1]*b[2]-a[2]*b[1];
    v[1] = a[2]*b[0]-a[0]*b[2];
    v[2] = a[0]*b[1]-a[1]*b[0];
}
inline void Vtransf(float v[], float a[], float m[][]){
    v[0] = a[0]*m[0][0] + a[1]*m[1][0] + a[2]*m[2][0];
    v[1] = a[0]*m[0][1] + a[1]*m[1][1] + a[2]*m[2][1];
    v[2] = a[0]*m[0][2] + a[1]*m[1][2] + a[2]*m[2][2];
}



//vector properties
float Vmag(float v[]){
    return sqrt(sq(v[0])+sq(v[1])+sq(v[2]));
}
float Vquadr(float v[]){
    return sq(v[0])+sq(v[1])+sq(v[2]);
}




//set m to result of operation
