// ALO-095BWNN-J9 test program
// See also "http://www.aitendo.co.jp/product/2099"


#ifndef __SPIOLED96x64_H__
#define __SPIOLED96x64_H__

#include "mbed.h"
#include "ascii_font6x8.h"
#include "small_font.h"

//#define __USE_GAC__     // use SSD1331/1332 Graphi Accelaration Command

#define Dis_X_MAX 96-1      // Max X axial direction in screen
#define Dis_Y_MAX 64-1      // Max Y axial direction in screen
#define X_WitchASC 6        // character's width
#define X_Witch 8           // character's width
#define Y_Witch 8           // character's height

#define CS_NORMAL  0
#define CS_WIDE    1
#define CS_HIGH    2
#define CS_WH      3
#define CS_WHx36   4



#define GAC_FILL_ENABLE_DISABLE  0x26   // �h��Ԃ��ۃR�}���h
#define GAC_DRAW_RECTANGLE       0x22   // Rectangle cmd

#define GAC_COPY_AREA            0x23   // Copy Area
#define GAC_SCROLL_SETUP         0x27
#define GAC_SCROLL_STOP          0x2E
#define GAC_SCROLL_START         0x2F

class SPIOLED96x64 : public Stream {
public:
    // constructor
    SPIOLED96x64(PinName cs_pin, PinName rst_pin, PinName a0_pin, PinName mosi_pin, PinName miso_pin, PinName sclk_pin);
 #if DOXYGEN_ONLY
     int putc(int c);
     int printf(const char* format, ...);
 #endif
    void  RegWrite(unsigned char Command);
    void  RegWriteM(unsigned char *Command, int count);
    void  DataWrite(unsigned char c);
    void  DataWrite_to(unsigned int Dat);

    void Draw_Dot(int x,int y,unsigned int Color);

    void Box(int x1,int y1, int x2, int y2, unsigned int Color, int fill);
    void Fill_Screen(unsigned int Color);
    
    void CS_Conv(int *lpx, int *lpy);
    unsigned int ColorConv(unsigned int R,unsigned int G,unsigned int B);
    void ChangeFontColor(unsigned int color);
    void ChangeBGColor(unsigned int color);
    void SetFontSize(int);
    void locate(int column, int row);
    int row();
    int column();

    void line( int x0,int y0,int x1,int y1, unsigned int fore_col ,int pat  );
    void circle (int radius, int x, int y , unsigned int col, int fill);
    void Pattern(int x,int y,int width, int height, unsigned int *p);
    void GACEnable(int enable);
    
    void Copy(int src_x1,int src_y1,int src_x2,int src_y2, int dst_x,int dst_y);
    void ScrollSet(int a, int b, int c, int d , int frame_interval);
    void Scroll(int enable);
    
protected:
     // Stream implementation functions
     virtual int _putc( int c );
     virtual int _getc();
private:
    DigitalOut  CS,  RES,  DC;
    SPI spi; // mosi, miso, sclk

    unsigned int Char_Color;    // character's color
    unsigned int BGround_Color;
    int GAC_Useflag;
    int x_locate;
    int y_locate;
    int chr_size;
    void Init(void);
    void PutChar(int x,int y,unsigned int a);
    unsigned int findface(unsigned short c);


};


#endif