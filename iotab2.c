#define IOTAB2_LO 0xfffa01
#define IOTAB2_HI 0xfffc3f
B LOAD_B_fffa01(void);
B LOAD_B_fffa03(void);
B LOAD_B_fffa05(void);
B LOAD_B_fffa07(void);
B LOAD_B_fffa09(void);
B LOAD_B_fffa0b(void);
B LOAD_B_fffa0d(void);
B LOAD_B_fffa0f(void);
B LOAD_B_fffa11(void);
B LOAD_B_fffa13(void);
B LOAD_B_fffa15(void);
B LOAD_B_fffa17(void);
B LOAD_B_fffa19(void);
B LOAD_B_fffa1b(void);
B LOAD_B_fffa1d(void);
B LOAD_B_fffa1f(void);
B LOAD_B_fffa21(void);
B LOAD_B_fffa23(void);
B LOAD_B_fffa25(void);
B LOAD_B_fffa27(void);
B LOAD_B_fffa29(void);
B LOAD_B_fffa2b(void);
B LOAD_B_fffa2d(void);
B LOAD_B_fffa2f(void);
B LOAD_B_fffa31(void);
B LOAD_B_fffc00(void);
B LOAD_B_fffc02(void);
B LOAD_B_fffc04(void);
B LOAD_B_fffc06(void);
B LOAD_B_fffc20(void);
B LOAD_B_fffc21(void);
B LOAD_B_fffc22(void);
B LOAD_B_fffc23(void);
B LOAD_B_fffc24(void);
B LOAD_B_fffc25(void);
B LOAD_B_fffc26(void);
B LOAD_B_fffc27(void);
B LOAD_B_fffc28(void);
B LOAD_B_fffc29(void);
B LOAD_B_fffc2a(void);
B LOAD_B_fffc2b(void);
B LOAD_B_fffc2c(void);
B LOAD_B_fffc2d(void);
B LOAD_B_fffc2e(void);
B LOAD_B_fffc2f(void);
B LOAD_B_fffc30(void);
B LOAD_B_fffc31(void);
B LOAD_B_fffc32(void);
B LOAD_B_fffc33(void);
B LOAD_B_fffc34(void);
B LOAD_B_fffc35(void);
B LOAD_B_fffc36(void);
B LOAD_B_fffc37(void);
B LOAD_B_fffc38(void);
B LOAD_B_fffc39(void);
B LOAD_B_fffc3a(void);
B LOAD_B_fffc3b(void);
B LOAD_B_fffc3c(void);
B LOAD_B_fffc3d(void);
B LOAD_B_fffc3e(void);
B LOAD_B_fffc3f(void);
B (* IOTAB2_funcs_LOAD[])(void)={
LOAD_B_fffa01,
LOAD_B_fffa03,
LOAD_B_fffa05,
LOAD_B_fffa07,
LOAD_B_fffa09,
LOAD_B_fffa0b,
LOAD_B_fffa0d,
LOAD_B_fffa0f,
LOAD_B_fffa11,
LOAD_B_fffa13,
LOAD_B_fffa15,
LOAD_B_fffa17,
LOAD_B_fffa19,
LOAD_B_fffa1b,
LOAD_B_fffa1d,
LOAD_B_fffa1f,
LOAD_B_fffa21,
LOAD_B_fffa23,
LOAD_B_fffa25,
LOAD_B_fffa27,
LOAD_B_fffa29,
LOAD_B_fffa2b,
LOAD_B_fffa2d,
LOAD_B_fffa2f,
LOAD_B_fffa31,
LOAD_B_fffc00,
LOAD_B_fffc02,
LOAD_B_fffc04,
LOAD_B_fffc06,
LOAD_B_fffc20,
LOAD_B_fffc21,
LOAD_B_fffc22,
LOAD_B_fffc23,
LOAD_B_fffc24,
LOAD_B_fffc25,
LOAD_B_fffc26,
LOAD_B_fffc27,
LOAD_B_fffc28,
LOAD_B_fffc29,
LOAD_B_fffc2a,
LOAD_B_fffc2b,
LOAD_B_fffc2c,
LOAD_B_fffc2d,
LOAD_B_fffc2e,
LOAD_B_fffc2f,
LOAD_B_fffc30,
LOAD_B_fffc31,
LOAD_B_fffc32,
LOAD_B_fffc33,
LOAD_B_fffc34,
LOAD_B_fffc35,
LOAD_B_fffc36,
LOAD_B_fffc37,
LOAD_B_fffc38,
LOAD_B_fffc39,
LOAD_B_fffc3a,
LOAD_B_fffc3b,
LOAD_B_fffc3c,
LOAD_B_fffc3d,
LOAD_B_fffc3e,
LOAD_B_fffc3f,
};
UB IOTAB2_flags_LOAD[]={
2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1,10,1,11,1,
12,1,13,1,14,1,15,1,16,1,17,1,18,1,19,1,20,1,21,1,
22,1,23,1,24,1,25,1,26,1,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,27,0,28,0,29,0,30,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,};
void STORE_B_fffa01(B v);
void STORE_B_fffa03(B v);
void STORE_B_fffa05(B v);
void STORE_B_fffa07(B v);
void STORE_B_fffa09(B v);
void STORE_B_fffa0b(B v);
void STORE_B_fffa0d(B v);
void STORE_B_fffa0f(B v);
void STORE_B_fffa11(B v);
void STORE_B_fffa13(B v);
void STORE_B_fffa15(B v);
void STORE_B_fffa17(B v);
void STORE_B_fffa19(B v);
void STORE_B_fffa1b(B v);
void STORE_B_fffa1d(B v);
void STORE_B_fffa1f(B v);
void STORE_B_fffa21(B v);
void STORE_B_fffa23(B v);
void STORE_B_fffa25(B v);
void STORE_B_fffa27(B v);
void STORE_B_fffa29(B v);
void STORE_B_fffa2b(B v);
void STORE_B_fffa2d(B v);
void STORE_B_fffa2f(B v);
void STORE_B_fffa31(B v);
void STORE_B_fffc00(B v);
void STORE_B_fffc02(B v);
void STORE_B_fffc04(B v);
void STORE_B_fffc06(B v);
void STORE_B_fffc20(B v);
void STORE_B_fffc21(B v);
void STORE_B_fffc22(B v);
void STORE_B_fffc23(B v);
void STORE_B_fffc24(B v);
void STORE_B_fffc25(B v);
void STORE_B_fffc26(B v);
void STORE_B_fffc27(B v);
void STORE_B_fffc28(B v);
void STORE_B_fffc29(B v);
void STORE_B_fffc2a(B v);
void STORE_B_fffc2b(B v);
void STORE_B_fffc2c(B v);
void STORE_B_fffc2d(B v);
void STORE_B_fffc2e(B v);
void STORE_B_fffc2f(B v);
void STORE_B_fffc30(B v);
void STORE_B_fffc31(B v);
void STORE_B_fffc32(B v);
void STORE_B_fffc33(B v);
void STORE_B_fffc34(B v);
void STORE_B_fffc35(B v);
void STORE_B_fffc36(B v);
void STORE_B_fffc37(B v);
void STORE_B_fffc38(B v);
void STORE_B_fffc39(B v);
void STORE_B_fffc3a(B v);
void STORE_B_fffc3b(B v);
void STORE_B_fffc3c(B v);
void STORE_B_fffc3d(B v);
void STORE_B_fffc3e(B v);
void STORE_B_fffc3f(B v);
void (* IOTAB2_funcs_STORE[])(B v)={
STORE_B_fffa01,
STORE_B_fffa03,
STORE_B_fffa05,
STORE_B_fffa07,
STORE_B_fffa09,
STORE_B_fffa0b,
STORE_B_fffa0d,
STORE_B_fffa0f,
STORE_B_fffa11,
STORE_B_fffa13,
STORE_B_fffa15,
STORE_B_fffa17,
STORE_B_fffa19,
STORE_B_fffa1b,
STORE_B_fffa1d,
STORE_B_fffa1f,
STORE_B_fffa21,
STORE_B_fffa23,
STORE_B_fffa25,
STORE_B_fffa27,
STORE_B_fffa29,
STORE_B_fffa2b,
STORE_B_fffa2d,
STORE_B_fffa2f,
STORE_B_fffa31,
STORE_B_fffc00,
STORE_B_fffc02,
STORE_B_fffc04,
STORE_B_fffc06,
STORE_B_fffc20,
STORE_B_fffc21,
STORE_B_fffc22,
STORE_B_fffc23,
STORE_B_fffc24,
STORE_B_fffc25,
STORE_B_fffc26,
STORE_B_fffc27,
STORE_B_fffc28,
STORE_B_fffc29,
STORE_B_fffc2a,
STORE_B_fffc2b,
STORE_B_fffc2c,
STORE_B_fffc2d,
STORE_B_fffc2e,
STORE_B_fffc2f,
STORE_B_fffc30,
STORE_B_fffc31,
STORE_B_fffc32,
STORE_B_fffc33,
STORE_B_fffc34,
STORE_B_fffc35,
STORE_B_fffc36,
STORE_B_fffc37,
STORE_B_fffc38,
STORE_B_fffc39,
STORE_B_fffc3a,
STORE_B_fffc3b,
STORE_B_fffc3c,
STORE_B_fffc3d,
STORE_B_fffc3e,
STORE_B_fffc3f,
};
UB IOTAB2_flags_STORE[]={
2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1,10,1,11,1,
12,1,13,1,14,1,15,1,16,1,17,1,18,1,19,1,20,1,21,1,
22,1,23,1,24,1,25,1,26,1,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,27,0,28,0,29,0,30,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,};
