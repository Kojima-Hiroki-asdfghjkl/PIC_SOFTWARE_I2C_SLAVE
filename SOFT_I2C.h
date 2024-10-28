/* 
 * File:   SOFT_I2C.h
 * Author: kojim
 *
 * Created on 2024/10/20, 23:17
 */

#ifndef SOFT_I2C_H
#define	SOFT_I2C_H
/*
 * MEMO
 * 
 * このプログラムはソフトウェア的にI2Cのスレーブを実行します
 * 通信速度は100KHzが限界です（32MHz．他の物を実行しないならもう少しできるかもしれないけど，200KHzは無理）
 * 割り込み処理で通信を行うため，他の割り込み処理が長い場合は処理が追いつかないことがあるため，実際に試しながら使用してください
 * 最低限の通信ができるくらいしか機能が乗っていないのでそこは勘弁してください
 * アセンブリレベルで最適化すればもっとなんとかなるかもしれないけど，Cのレベルで雑なので勘弁して
 * 
 * テスト事項はした
 * 
 * 使用リソース
 * 外部割り込み
 * ポート変化割り込み
 * タイマ（1個）
 * 
 * もちろんポート2個（Clockピンは出力ができなくても大丈夫だけど，クロックストレッチできない）
 * 
 * 設定した後で機能を殺したくなったら使ってる割り込みを全部止めてください
 */

//ここから環境ごとにプログラムを書き換えること
//例としてはSDA:RA5 SCL:RA4 を使用する


//SETTING-----------------------------------------------------------------------
#define USE_ERROR_INT
//#define USE_SOFT_CLOCK_STRETCH//クロックストレッチを実行するかどうか
//CLKピンが出力に対応していない場合はどうやっても実行できないので注意が必要．その場合は無効にすること

//ADDRESS-----------------------------------------------------------------------
#define SOFT_I2C_ADD 0x10 //左1シフトした値を入れること
#define SOFT_I2C_ADD_MASK 0xFE

//PIN---------------------------------------------------------------------------
#define SDA_PIN PORTAbits.RA5
#define SDA_PIN_NUM 0x20//コレはポートのレジスタのビットを示す "SDA_PORT & SDA_PIN"として使用したい.
#define SDA_PORT PORTA//これはSDAのあるポートのレジスタ名を入れる

//例として，PORT1をつかうなら 0x02となる
//ピンのマクロでPORTxbits.Rx0の様にマクロするとうまくBSF, BCFが使用されるため，レジスタの中身がぐちゃぐちゃにならずにすむ
//ただし，このプログラムの場合，入力も必要になるため，PORTレジスタを指定する
#define SCL_PIN PORTAbits.RA4

#define SDA_TRIS TRISAbits.TRISA5
#define SCL_TRIS TRISAbits.TRISA4

#define SDA_ANSEL ANSELAbits.ANSA5
#define SCL_ANSEL ANSELAbits.ANSA4

//INT-ON-CHANGE-----------------------------------------------------------------
#define SDA_IOC_P IOCAPbits.IOCAP5
#define SCL_IOC_P IOCAPbits.IOCAP4

#define SDA_IOC_N IOCANbits.IOCAN5
#define SCL_IOC_N IOCANbits.IOCAN4

#define SDA_IOC IOCAFbits.IOCAF5
#define SCL_IOC IOCAFbits.IOCAF4

//INT---------------------------------------------------------------------------
#define IOC_FLAG PIR0bits.IOCIF//INT-ON-CHANGE

//VARIABLE----------------------------------------------------------------------
char SOFT_I2C_BIT_COUNT2 = 0; //現在受信・送信完了したビット数を格納する変数
//char SOFT_I2C_BIT_COUNT1 = 0; //送信時に使用する変数　ぶっちゃけ無くてもできなくはないが，ステータスレジスタをよむのが面倒
char SOFT_I2C_BYTE_COUNT = 0; //
char SOFT_I2C_BUFF = 0;
char SOFT_I2C_BUFF_PORT = 0;
char SOFT_I2C_BUFF_SR = 0;//PICのレジスタの名前に沿ってこの名前にしました
//送受信するときに使用します
char SOFT_I2C_FLAG0 = 0; //STATUS FLAG
//7:CKP, 6:この通信は自分に対して送られている物(1:自分の, 0:他人の) , 5:D_nA, 4:P, 3:S, 2:R_nA, 1:データが1バイトそろったかどうか(1:そろった 0:そろってない)0:送信状況を記憶する
//↑よく使うフラグはPICのハードウェアと互換性を持たせたつもりです．そのほかは埋め合わせ
char SOFT_I2C_FLAG1 = 0;
//
char SOFT_I2C_STATUS = 0;
//ユーザーが仕様する変数
//これから現在の状態を判定する


//OTHER-------------------------------------------------------------------------

//MACRO-------------------------------------------------------------------------
//PIN
#define TRIS_INPUT 1
#define TRIS_OUTPUT 0
//くだらないところでミスしたくないからね
#define ANSEL_ANA 1
#define ANSEL_DIGI 0

#define PIN_HI 1
#define PIN_LO 0

//I2C
#define S_CKP  0x80
#define S_MINE 0x40
#define S_D_nA 0x20
#define S_P    0x10
#define S_S    0x08
#define S_R_nW 0x04
#define S_END_DATA 0x02
#define S_WRITE 0x01

//PROTOTYPE---------------------------------------------------------------------
void SOFT_I2C_INIT(void);//ソフトウェアI2C初期化関数//この関数は全ての設定が終わった後に呼び出すこと．そうすれば，いい感じになる

//INT
void INT_SETTNG(void);

//SOFTWARE INT
void SOFT_I2C_INT(void);
void I2C_ERROR_INT(void);

//END---------------------------------------------------------------------------

#endif	/* SOFT_I2C_H */

/*
 * テスト下内容として
 * アドレス→データ(0x01)→アドレス→データ(0x00)→...
 * をDELAYなしで実行して10分耐えたから十分かと思う
 * 
 * データの連続受信について，連続10バイトまではテストした．まぁそれ以上でも動くと思うからいいよね？
 * 
 * 
 * 多分動く
 */

/*
 * 技術メモ
 * 
 * 面倒くさいので，アドレス受信時は送受信モードビットを受信側にすることにする
 * 
 * まず，スタートビットと，ストップビットを見分ける必要がある
 * コレはクロックラインがHIに保持されているタイミングでデータラインが変化することによって判別できる
 * HI→LO:START, LO→HI:STOP
 * 通常通信時はクロックラインがLOに保持されている場合にデータラインが変化する
 * 
 * とは言った物の，ストップビット見てないです（見た場合，ストップのすぐ次にスタート飛んでくると処理落ちする）
 * 
 * 
 * ACKのタイミングでCLKがHIの状態でSDAをLOにするとマスタがエラー吐く
 * 
 * Slave送信時のNACKかくにんタイミングはクロックの立ち上がり時でよさそうな気がする
 * 
 * CKPの実装どうしよう
 */