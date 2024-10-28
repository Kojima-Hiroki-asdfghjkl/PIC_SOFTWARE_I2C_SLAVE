#include <xc.h>

#include "SOFT_I2C.h"

//USER--------------------------------------------------------------------------
//PIC割り込み設定
void INT_SETTNG(void){//必ず環境，大本のプログラムに合わせて割り込みの設定を書くこと
    INTCON = 0xC0;//PIE, PEIE
    PIE0 = 0x10;//IOC
}

//ソフトウェアI2Cを使用しているときに割り込みのように使う関数
void SOFT_I2C_INT(void){
    if (SOFT_I2C_STATUS & S_D_nA){//DATA
        if (SOFT_I2C_BUFF & 0x01){
            LATC2 = 1;
        }
        else {
            LATC2 = 0;
        }
    }
    
}

//通信エラーが発生したときに飛ぶ関数
void I2C_ERROR_INT(void){
    
}

//END---------------------------------------------------------------------------

//#define SDA_HI() SDA_TRIS = TRIS_INPUT; SDA_IOC_P = 1; SDA_IOC_N = 1
//#define SDA_LO() SDA_TRIS = TRIS_OUTPUT; SDA_PIN = PIN_LO; SDA_IOC_P = 0; SDA_IOC_N = 0
#define SDA_HI() SDA_TRIS = TRIS_INPUT; SDA_IOC_N = 1
#define SDA_LO() SDA_TRIS = TRIS_OUTPUT; SDA_PIN = PIN_LO; SDA_IOC_N = 0

//#define SCL_HI() SCL_TRIS = TRIS_INPUT
//#define SCL_LO() SCL_TRIS = TRIS_OUTPUT; SCL_PIN = PIN_LO

void SOFT_I2C_INIT(void){
    //ピンの設定
    SDA_TRIS = TRIS_INPUT;
    SDA_ANSEL = ANSEL_DIGI;
    
    SCL_TRIS = TRIS_INPUT;
    SCL_ANSEL = ANSEL_DIGI;
    
    SDA_PIN = PIN_LO;
    //ここからSDAピンの状態はLOになっている状態としてプログラムを書く
    //そのためこれ以降このレジスタを変更することは禁止する
    
    //クロックストレッチを行うため
    SCL_PIN = PIN_LO;
    
    //INT-----------------------------------------------------------------------
    INT_SETTNG();
    
    //IOC-----------------------------------------------------------------------
    //SDA_IOC_P = 1;
    SCL_IOC_P = 1;
    
    SDA_IOC_N = 1;
    SCL_IOC_N = 1;
    
    
}

//    //debug
//    LATC1 = 1;
//    LATC1 = 0;
//    //end

//TEST
char SEND_DATA = 'B';

void __interrupt() SOFTWARE_I2C_INT_PIC(void) {
    if(IOC_FLAG){
        //ここでデータを見ると，取りこぼしが減るかもしれないのでここでデータを見る
        SOFT_I2C_BUFF_PORT = SDA_PORT;
        if (SCL_IOC){
            SCL_IOC = 0;
            if (SOFT_I2C_FLAG0 & S_MINE){
                if(SCL_PIN == PIN_HI){//CLK↑
                    if ((SOFT_I2C_FLAG0 & S_R_nW) == 0){//READ(ADD or DATA)
                        if (SOFT_I2C_BUFF_PORT & SDA_PIN_NUM) {//SDA:HI
                            SOFT_I2C_BUFF_SR |= SOFT_I2C_BIT_COUNT2;
                        }
                        if (SOFT_I2C_BIT_COUNT2 == 1){//W or R BIT
                            TX1REG = SOFT_I2C_BUFF_SR;
                            if ((SOFT_I2C_FLAG0 & S_D_nA) == 0){//ADDRESS
                                //まずは，この通信のアドレスが自分のものかどうか判定する．
                                if ((SOFT_I2C_BUFF_SR & SOFT_I2C_ADD_MASK) != SOFT_I2C_ADD) {
                                    SOFT_I2C_FLAG0 &= ~S_MINE;
                                }
                                else {
                                    while (SCL_PIN == PIN_HI);//タイミング合わせ
                                    SDA_LO();//ACK
                                    //自分のデータだった場合は処理を続ける
                                    if (SOFT_I2C_BUFF_SR & 0x01){//Slave W
                                        SOFT_I2C_FLAG0 |= S_R_nW;
                                    }
                                    else {//Slave R

                                    }
                                }
                                SOFT_I2C_FLAG0 |= S_D_nA;//NEXT DATA
                            }
                            else {//DATA
                                while (SCL_PIN == PIN_HI);//CLKがLOになってからACKするための待ち
                                SDA_LO();//ACK
                                SOFT_I2C_BUFF = SOFT_I2C_BUFF_SR;
                            }
                            SCL_IOC_P = 0;
                            SCL_IOC_N = 1;
                        //コレをすることで，データを代入するときにそのままorで入れることができる（今までは普通にインクリメントしてたので，1をビット数分シフトしてた．そうすると遅い）
                        }
                        SOFT_I2C_BIT_COUNT2 >>= 1;//高速化のため
                    }
                    else {//WRITE
                        if (SOFT_I2C_BUFF_PORT & SDA_PIN_NUM){//SDA:HI
                            //NACK
                            SOFT_I2C_FLAG0 &= ~(S_R_nW | S_WRITE);
                        }
                        else {
                            //ACK
                            //V_CLEAR
                            //SOFT_I2C_BUFF_SR = SOFT_I2C_BUFF;
                            SOFT_I2C_BUFF_SR = SEND_DATA;
                            //SEND_DATA ++;
                            SOFT_I2C_BIT_COUNT2 = 0x80;
                            SOFT_I2C_FLAG0 |= S_WRITE;
                            SOFT_I2C_BYTE_COUNT ++;
                        }
                        SCL_IOC_P = 0;
                        SCL_IOC_N = 1;
                    }
                }
                else{//CLK↓
                    if (SOFT_I2C_FLAG0 & S_WRITE) goto WRITE;
                    else if (SOFT_I2C_FLAG0 & S_S){
                        SOFT_I2C_BUFF_SR = 0x00;
                        SOFT_I2C_FLAG0 &= ~(S_S | S_D_nA | S_R_nW);//START ADDRESS READ
                        SOFT_I2C_STATUS = SOFT_I2C_FLAG0;
                        SOFT_I2C_BIT_COUNT2 = 0x80;
                        SOFT_I2C_BYTE_COUNT = 0;
                        SCL_IOC_N = 0;
                        SCL_IOC_P = 1;
                    }
                    //ACK
                    else if (SOFT_I2C_BIT_COUNT2 == 0){
                        SDA_HI();//ACK END
                        //クロックストレッチするならここ
                        
                        SOFT_I2C_INT();
                        SOFT_I2C_STATUS = SOFT_I2C_FLAG0;
                        if (SOFT_I2C_FLAG0 & S_R_nW){//WRITE
                            //V_CLEAR
                            //SOFT_I2C_BUFF_SR = SOFT_I2C_BUFF;
                            SOFT_I2C_BUFF_SR = SEND_DATA;
                            //SEND_DATA ++;
                            SOFT_I2C_BIT_COUNT2 = 0x80;
                            SOFT_I2C_FLAG0 |= S_WRITE;
                            SOFT_I2C_BYTE_COUNT ++;
                        }
                        else {//READ
                            //V_CLEAR
                            SOFT_I2C_BUFF_SR = 0;
                            SOFT_I2C_BIT_COUNT2 = 0x80;
                            SOFT_I2C_BYTE_COUNT ++;
                            SCL_IOC_N = 0;
                            SCL_IOC_P = 1;
                        }
                    }
                    //送信時のみ使用
                    if (SOFT_I2C_FLAG0 & S_R_nW){//WRITE
WRITE:
                        if (SOFT_I2C_BIT_COUNT2 == 0){//SEND_END
                            SDA_HI();
                            //マスタからのACKかくにんのために割り込みをNegativeからPositiveに変える
                            SCL_IOC_N = 0;
                            SCL_IOC_P = 1;
                        }
                        else {
                            if (SOFT_I2C_BUFF_SR & 0x80){
                                SDA_HI();
                            }
                            else {
                                SDA_LO();
                            }
                        }
                        SOFT_I2C_BUFF_SR <<= 1;
                        SOFT_I2C_BIT_COUNT2 >>= 1;
                    }
                }
            }
        }
        //まずはこのビットがスタート，ストップBITかどうか判別する
        else if (SDA_IOC){//SDAが変化した
            if(SCL_PIN == PIN_HI){
                //START
                SOFT_I2C_FLAG0 |= S_S | S_MINE;//START
                
                SCL_IOC_N = 1;
                SCL_IOC_P = 1;
            }
        }
        SDA_IOC = 0;//割り込みフラグ　リセット
        //↑コレここに書かないとバグるわ
        //タイミングなのか？
    }
}