#define GET_SI8 LM_B(MEM(pc+1)); pc+=2;
#define GET_SI16 LM_W(MEM(pc)); pc+=2;
#define GET_SI32 LM_L(MEM(pc)); pc+=4;

