

// ALO-095BWNN-J9 test program
// See also "http://www.aitendo.co.jp/product/2099"
// Ver1.00 初版完成
// Ver1.01 フォントをk6x8フォントに変更。定義Ｈファイルも全角とは分離
//         フォント描画ルーチンが１ドットすくなく動くのを修正
//         任意カラーパターンを描画できるように修正
// Ver1.02 SD1331/1332用 GACcommand対応
//         ただし現状ではRectangeしか機能しない模様
// Ver1.03 Box (GAC) Color Bug fix
// Ver.104 Font Position Convert Bug fix
//         Position Clip is refine.

#include "mbed.h"
#include "spioled96x64.h"

//Serial pc(USBTX, USBRX);
#define countof(x) ( sizeof(x) / sizeof(x[0]) )


// コンストラクタ
SPIOLED96x64::SPIOLED96x64(PinName cs_pin, PinName rst_pin, PinName a0_pin, PinName mosi_pin, PinName miso_pin, PinName sclk_pin)
            : CS(cs_pin), RES(rst_pin), DC(a0_pin), spi(mosi_pin, miso_pin, sclk_pin) 
{
    Init();
}

// イニシャライザ
void SPIOLED96x64::Init(void)
{

    spi.format(8,3); //8bit frame and POL=1 /PHA=1(UpEdge Sampled)
    spi.frequency(20000000); // modify later
    //spi.frequency(42000000); // modify later
    
    // reset
    wait_ms(200);
    RES = 0;       //Reset active
    wait_ms(200);
    RES = 1;
    
    
    // initialize sequence
    RegWrite(0xae);    //OLED display OFF
    
    //Row Address
    RegWrite(0x75);    /* Set Row Address */
    RegWrite(0x00);    /* Start = 0 */
    RegWrite(0x3f);    /* End = 63 */
    RegWrite(0x15);    /* Set Column Address */
    RegWrite(0x00);    /* Start = 0 */
    RegWrite(0x5F);    /* End = 95 */
    
     //Contrast
    RegWrite(0xa0);    //Set remap & data format 0111 0000
    RegWrite(0b01110010);    // 65k Color/8bit buswidth/format1
    RegWrite(0xa1);    //set display start row RAM
    RegWrite(0x00);
    RegWrite(0xa2);    //set dispaly offset
    RegWrite(0x00);
    RegWrite(0xa4);    //Set Display Mode
    RegWrite(0xa8);    //Set Multiplex Ratio
    RegWrite(0x3f);
    RegWrite(0xad);    //Set Master Configuration
    RegWrite(0x8f);    //(External VCC Supply Selected)
    RegWrite(0xB0);    //Set Power Saving Mode
    RegWrite(0x1a);
    RegWrite(0xB1);    //Set Phase 1 & 2 Period Adjustment
    RegWrite(0x74);
    RegWrite(0xb3);    //Set Display Clock Divide Ratio / Oscillator Frequency
    RegWrite(0xd0);
    RegWrite(0x8A);    //Set Second Pre-charge Speed of Color A
    RegWrite(0x81);
    RegWrite(0x8B);    //Set Second Pre-charge Speed of Color B
    RegWrite(0x82);
    RegWrite(0x8C);    //Set Second Pre-charge Speed of Color C
    RegWrite(0x83);
    RegWrite(0xBB);    //Set Pre-charge Level
    RegWrite(0x3e);
    RegWrite(0xBE);    //Set VCOMH
    RegWrite(0x3e);
    RegWrite(0x87);    //Set Master Current Control
    RegWrite(0x0f);
    RegWrite(0x81);    //Set Contrast Control for Color &#129;gA&#129;h
    RegWrite(0x80);
    RegWrite(0x82);    //Set Contrast Control for Color &#129;gB&#129;h
    RegWrite(0x80);
    RegWrite(0x83);    //Set Contrast Control for Color &#129;gC&#129;h
    RegWrite(0x80);
    RegWrite(0xaf);    //display ON

    x_locate = 0;
    y_locate = 0;
    chr_size = CS_NORMAL;
    Fill_Screen(0);
    GAC_Useflag = 0;
}

// レジスタライト
void  SPIOLED96x64::RegWrite(unsigned char Command)
{
    DC = 0;    // Command
    CS = 0;    // CS active(toggle OLED)
    spi.write(Command);
    CS = 1;
}
// マルチバイト レジスタ設定
void  SPIOLED96x64::RegWriteM(unsigned char *Command, int count)
{
    int i;
    DC = 0;    // Command
    CS = 0;    // CS active(toggle OLED)
    for( i=0;i<count;i++){
       spi.write(*Command++);
    }
    CS = 1;
}


// データライト
void  SPIOLED96x64::DataWrite(unsigned char c)
{
     DC = 1;    // DATA
     CS = 0;    // CS active(toggle OLED)
     spi.write(c);
     CS = 1;
}

// 16ビット書き込み
void  SPIOLED96x64::DataWrite_to(unsigned int Dat)
{
     DC = 1;    // DATA
     CS = 0;    // CS active(toggle OLED)
     spi.write((unsigned char)((Dat >> 8)& 0x00ff));
     spi.write((unsigned char)(Dat & 0x00ff));
     CS = 1;
    /*
     DataWrite((unsigned char)((Dat >> 8)& 0x00ff));
     DataWrite((unsigned char)(Dat & 0x00ff));
     */
}



// 指定座標に指定色でドット描画
void SPIOLED96x64::Draw_Dot(int x,int y,unsigned int Color)
{
    unsigned char cmd[7]={0x15,0x00,0x00,0x75,0x00,0x00,0x00};
    if (  ( x < 0 )||( y < 0 )
        ||( x > Dis_X_MAX )||( y > Dis_Y_MAX ) ) return ;
    cmd[1] = (unsigned char)x; 
    cmd[2] = (unsigned char)x; 
    cmd[4] = (unsigned char)y; 
    cmd[5] = (unsigned char)y; 
    RegWriteM(cmd, 6);
    DataWrite_to(Color);
    //DataWrite_to(Color);
}

    

void SPIOLED96x64::ChangeFontColor(unsigned int color)
{
    Char_Color = color;
}
void SPIOLED96x64::ChangeBGColor(unsigned int color)
{
    BGround_Color = color;
}


// CS_NORMAL/CS_WIDE/CS_HIGH/CS_WH 
void SPIOLED96x64::SetFontSize(int Csize)
{
    chr_size = Csize;
}
//　縦横の倍率変化量を返す
void SPIOLED96x64::CS_Conv(int *lpx,int *lpy)
{
    switch( chr_size ){
    case CS_WIDE:
        *lpx=2;
        *lpy=1;
        break;
    case CS_HIGH:
        *lpx=1;
        *lpy=2;
        break;
    case CS_WH  :
        *lpx=2;
        *lpy=2;
        break;
    case CS_WHx36  :
        *lpx=6;
        *lpy=6;
        break;
    case CS_NORMAL:
    default:
        *lpx=1;
        *lpy=1;
        break;
    }    
    
}

// 全角キャラクタの描画処理
// アスキーキャラクタの位置指定表示(5x7dotフォント)
// フォントの特性上、最上位ビットから順に↑方向に記述される
void SPIOLED96x64::PutChar(int x,int y,unsigned int a)
{
    int i,j;
    unsigned char Temp=0;        
    int lpx, lpy, k,l;
    int xw;    
    j = 0;
    i = 0;

    CS_Conv(&lpx, &lpy);
    if ( a < 0x80 ) { xw = X_WitchASC; if(a < 32)a=32; } // ASCII
    else              xw = X_Witch;                      // Kanji

    for(i=0; i<xw; i++){    
        for ( l=0;l<lpx;l++){
            if ( a < 0x80 ) Temp = FontLookup_ABC[a-32][i];     // ASCII(0x20~0x7f)==> 0x00~0x5f
            else            Temp = FontLookup[a][i];            // Kanji
            
            for(j=Y_Witch-1;j>=0;j--){
                for ( k=0;k<lpy;k++){
                    Draw_Dot(x+(i*lpx)+l,y+(((j+1)*lpy)-1)-k,  ((Temp & 0x80)==0x80) ? Char_Color : BGround_Color);
                }
                Temp = Temp << 1;
            }
        }
    }
}


// 純粋仮想関数でputcを実現
int SPIOLED96x64::_putc( int c )
{
    static bool kstate = false;
    static unsigned int kbuf = 0;
    unsigned int p;
    int lpx, lpy, w;

    if(!kstate && ((0x81 <= c && c <= 0x9f) || (0xe0 <= c && c <= 0xfc))){
        kstate = true;
        kbuf = c;
        return c;
    }    

    if(kstate){ // 2nd byte of shift-jis
        kstate = false;
        p = findface(kbuf << 8 | c);
        w = X_Witch;
    } else {
        p = c;
        w = X_WitchASC;  // 半角は近接するので、１ドットあける
    }
    PutChar( x_locate , y_locate ,p);
    CS_Conv(&lpx, &lpy);
    x_locate += (w*lpx);


    /*
    if(kstate== false){
        //画面右端でのクリッピング
        if(x_locate + (xsize - 1) >= Dis_X_MAX){
            x_locate = 0;
            y_locate += Y_Witch;
            if(y_locate >= Dis_Y_MAX)  y_locate=0;
        }
    } 
    */      
    return c;
}

int SPIOLED96x64::_getc() 
{
    return -1;
}

/*
void SPIOLED96x64::puts( char *s )
{
    unsigned char c;
    while((c = *s++) != '\0')  putc((int)c & 0xff);
}
*/


void SPIOLED96x64::locate(int column, int row)
{
    x_locate = column;
    y_locate = row;
}

int SPIOLED96x64::row()
{
    return y_locate;
}
int SPIOLED96x64::column()
{
    return x_locate;
}

// SJIS漢字コードテーブルから指定の配列アドレスを算出する
unsigned int SPIOLED96x64::findface(unsigned short c)
{
    unsigned int p = 0;
    int i, sum;
    for(sum = i = 0; i < countof(font8table); i++){
        if(font8table[i].start <= c && c <= font8table[i].end){
            p = (sum + c - font8table[i].start);
            break;
        }
        sum += font8table[i].end - font8table[i].start + 1;
    }
    return p;
}

// ＲＧＢ(555)をRGB565値に変換
// 00~1F(0~31)
unsigned int SPIOLED96x64::ColorConv(unsigned int R,unsigned int G,unsigned int B)
{
    unsigned int ret = 0;
    unsigned int rv, gv, bv;

    if ( R > 0x1f ) rv= 0x1f;
    else            rv= (unsigned int)( R & 0x1F );
    if ( G > 0x1f ) gv= 0x3f;
    else            gv= (unsigned int)( (G<<1) & 0x3F );
    if ( B > 0x1f ) bv= 0x1f;
    else            bv= (unsigned int)( B & 0x1F );
    
    ret = (bv << 11) + (gv << 5) + rv;
    return ret;
}
// 画面の指定色塗りつぶし指定領域し
void SPIOLED96x64::Box(int x1,int y1, int x2, int y2, unsigned int Color, int fill)
{
    
    if ( !GAC_Useflag ){
        unsigned char i,j;
    
        switch( fill ) {
        case 0:     // box
            line( x1, y1, x2, y1, Color, 0 );
            line( x2, y1, x2, y2, Color, 0 );
            line( x2, y2, x1, y2, Color, 0 );
            line( x1, y2, x1, y1, Color, 0 );
            break;
        default:    // fill
            for(i=y1;i<=y2;i++)
            {
                for(j=x1;j<=x2;j++)
                {
                    Draw_Dot(j,i,Color);
                    // DataWrite_to(Dot);
                }
            }
            break;
        }
    }else{
        // SSD1331/1332用 GAC(アクセラレートコマンド)
        unsigned char cmd[11]={ 0 };
        
        if  ( x1 < 0 ) x1 = 0;
        else if  ( x1 > Dis_X_MAX ) x1 = Dis_X_MAX;
        if  ( y1 < 0 ) y1 = 0;
        else if  ( y1 > Dis_Y_MAX ) y1 = Dis_Y_MAX;
        if  ( x2 < 0 ) x2 = 0;
        else if  ( x2 > Dis_X_MAX ) x2 = Dis_X_MAX;
        if  ( y2 < 0 ) y2 = 0;
        else if  ( y2 > Dis_Y_MAX ) y2 = Dis_Y_MAX;
    
        cmd[0] = GAC_FILL_ENABLE_DISABLE;      // 
        cmd[1] = (unsigned char)fill;  //( fill ) ? 0 : 0;
        RegWriteM(cmd, 2);
    
        cmd[0] = GAC_DRAW_RECTANGLE;      // 
        cmd[1] = (unsigned char)x1; 
        cmd[2] = (unsigned char)y1; 
        cmd[3] = (unsigned char)x2; 
        cmd[4] = (unsigned char)y2; 
        cmd[5] = (unsigned char)(((Color>>11)&0x1f)<<1);   // Outline Blue
        cmd[6] = (unsigned char) ((Color>>5 )&0x3f);   // Outline Green
        cmd[7] = (unsigned char)(( Color     &0x1f)<<1);   // Outline Red
        cmd[8] = (unsigned char)(((Color>>11)&0x1f)<<1);   // Outline Blue
        cmd[9] = (unsigned char) ((Color>>5 )&0x3f);   // Outline Green
        cmd[10]= (unsigned char)(( Color     &0x1f)<<1);   // Outline Red
        
        RegWriteM(cmd, 11);
        wait_ms( 1 );
        
    }
}

// 画面の指定色塗りつぶし
void SPIOLED96x64::Fill_Screen(unsigned int Color)
{
    BGround_Color = Color;
    Box( 0,0, Dis_X_MAX, Dis_Y_MAX , Color , 1);
}

// Bresenhamの線分発生アルゴリズム
// 整数のみによる線分描画
// (x0,y0)-(x1,y1)へのライン描画          参照:http //www2.starcat.ne.jp/~fussy/algo/algo1-1.htm
// pat : 0:実線    1:破線
void SPIOLED96x64::line( int x0,  int y0,  int x1,  int y1, unsigned int fore_col ,int pat  )
{
    int E,x,y;
    int dx,dy,sx,sy,i;
    int cl=1 ;
    int clc=0;    // 破線用のカウンタ
    
    sx = ( x1 > x0 ) ? 1 : -1;              // xの移動方向
    dx = ( x1 > x0 ) ? x1 - x0 : x0 - x1;   // xの移動量
    sy = ( y1 > y0 ) ? 1 : -1;              // yの移動方向
    dy = ( y1 > y0 ) ? y1 - y0 : y0 - y1;   // yの移動量

    x = x0;
    y = y0;

    if( dx >= dy ) {
        /* 傾きが1以下の場合 */
        E = -dx;
        for( i = 0; i <= dx; i++ ) {
            switch( pat ) {
            case 0:     // 実線
                cl = fore_col;
                break;
            default:    // 破線
                if ( clc>=3 ) {
                    if ( cl==fore_col ) cl=BGround_Color;
                    else                cl=fore_col;
                    clc=0;
                }else{
                    clc++;
                }
                break;
            }

            Draw_Dot(x,y,cl);
            x += sx;
            E += 2 * dy;
            if( E >= 0 ) {
                y += sy;
                E -= 2 * dx;
            }
        }
    }else{
        /* 傾きが1より大きい場合 */
        E = -dy;
        for( i = 0; i <= dy; i++ ) {
            switch( pat ) {
            case 0:     // 実線
                cl = fore_col;
                break;
            default:    // 破線
                if ( clc>=3 ) {
                    if ( cl==fore_col ) cl=BGround_Color;
                    else                cl=fore_col;
                    clc=0;
                }else{
                    clc++;
                }
                break;
            }

            Draw_Dot(x,y,cl);
            y += sy;
            E += 2 * dx;
            if( E >= 0 ) {
                x += sx;
                E -= 2 * dy;
            }
        }
    }
}

// ミッチェナー(Miechener) の円描画  参考:http://dencha.ojaru.jp/programs_07/pg_graphic_09a1.html
// x,y : 中心点
// radius: 半径
void SPIOLED96x64::circle (int radius, int x, int y , unsigned int col, int fill)
{
    int  cx, cy, d;

    d = 3 - 2 * radius;
    cy = radius;

    // 初期座標をあらかじめ描画する
    Draw_Dot(         x,  radius+y, col );    // 座標 ( 0, R );
    Draw_Dot(         x, -radius+y, col );    // 座標 ( 0, -R);
    Draw_Dot(  radius+x,         y, col );    // 座標 ( R, 0 );
    Draw_Dot( -radius+x,         y, col );    // 座標 (-R, 0);
    if ( fill ) {
        line(        x,  radius+y,          x, -radius+y, col ,0  );    
        line( radius+x,         y,  -radius+x,         y, col ,0  );    
    }

    for (cx = 0; cx <= cy; cx++){
        if (d >= 0) {
            d += 10 + 4 * cx - 4 * cy;
            cy--;
        } else {
            d += 6 + 4 * cx;
        }

        // 描画
        Draw_Dot(  cy + x,  cx + y, col);      // 0-45     度の間
        Draw_Dot(  cx + x,  cy + y, col);      // 45-90    度の間
        Draw_Dot( -cx + x,  cy + y, col);     // 90-135   度の間
        Draw_Dot( -cy + x,  cx + y, col);     // 135-180  度の間

        Draw_Dot( -cy + x, -cx + y, col);    // 180-225  度の間
        Draw_Dot( -cx + x, -cy + y, col);    // 225-270  度の間
        Draw_Dot(  cx + x, -cy + y, col);     // 270-315  度の間
        Draw_Dot(  cy + x, -cx + y, col);     // 315-360  度の間

        if ( fill ) {
            line(  cy + x,  cx + y,  cy + x, -cx + y, col ,0 );    
            line(  cx + x,  cy + y,  cx + x, -cy + y, col ,0 );      // 45-90    度の間
            line( -cx + x,  cy + y, -cx + x,  cy + y, col ,0 ) ;     // 90-135   度の間
            line( -cy + x,  cx + y, -cy + x,  cx + y, col ,0 );     // 135-180  度の間

            line( -cy + x, -cx + y, -cy + x,  cx + y, col ,0 );    // 180-225  度の間
            line( -cx + x, -cy + y, -cx + x,  cy + y, col ,0 );    // 225-270  度の間
            line(  cx + x, -cy + y,  cx + x,  cy + y, col ,0 );     // 270-315  度の間
            line(  cy + x, -cx + y,  cy + x,  cx + y, col ,0 );     // 315-360  度の間
        }

    }
}

// Color Pattern
// pattern *P はＲＧＢ５６５(２バイト型)のデータ配列で渡す
void SPIOLED96x64::Pattern(int x,int y,int width, int height, unsigned int *p)
{

    int i,j;
    
    for(i=0; i<width; i++){    
        for(j=height-1;j>=0;j--){
            Draw_Dot(x+i,y+j,*p++);
        }
    }

}
void SPIOLED96x64::GACEnable(int enable)
{
    GAC_Useflag = enable;
}


// 任意領域のコピー
// SSD1332では機能しない模様...
void SPIOLED96x64::Copy(int src_x1,int src_y1,int src_x2,int src_y2, int dst_x,int dst_y)
{
    
    if ( GAC_Useflag ){
        // SSD1331/1332用 GAC(アクセラレートコマンド)
        unsigned char cmd[11]={ 0 };
        int p;
        if (  ( src_x1 > Dis_X_MAX )||( src_y1 > Dis_Y_MAX ) 
            ||( src_x2 > Dis_X_MAX )||( src_y2 > Dis_Y_MAX ) ) return ;
        //if ( dst_x  > Dis_X_MAX )||( dst_y > Dis_Y_MAX )  return;
        
        
        p=0;
        cmd[p++] = GAC_COPY_AREA;      // 
        cmd[p++] = (unsigned char)src_x1; 
        cmd[p++] = (unsigned char)src_y1; 
        cmd[p++] = (unsigned char)src_x2; 
        cmd[p++] = (unsigned char)src_y2; 
        cmd[p++] = (unsigned char)dst_x; 
        cmd[p++] = (unsigned char)dst_y; 
        
        RegWriteM(cmd, p);
    
    }
}

// スクロール設定
void SPIOLED96x64::ScrollSet(int a, int b, int c, int d , int frame_interval)
{
    if ( GAC_Useflag ){
        // SSD1331/1332用 GAC(アクセラレートコマンド)
        unsigned char cmd[11]={ 0 };
        int p;
        //if (  ( startline > Dis_Y_MAX+1 )||( (startline+lincount) > Dis_Y_MAX+1 ) ) return ;
        if ( frame_interval > 3 ) frame_interval = 3;
        
        p=0;
        cmd[p++] = GAC_SCROLL_SETUP;      // 
        cmd[p++] = (unsigned char)a; 
        cmd[p++] = (unsigned char)b;
        cmd[p++] = (unsigned char)c;
        cmd[p++] = (unsigned char)d;
        cmd[p++] = (unsigned char)frame_interval;
            
        RegWriteM(cmd, p);
        
    }
    
}
void SPIOLED96x64::Scroll(int enable)
{
    if ( GAC_Useflag ){
        // SSD1331/1332用 GAC(アクセラレートコマンド)
        unsigned char cmd = (enable) ? GAC_SCROLL_START : GAC_SCROLL_STOP;      // 
            
        RegWrite(cmd);
        
    }
}

