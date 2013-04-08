//===========================================================
//MSP430F149 ADC12 单通道多路采样 P60采集tlv5618的输出，在p64通道进行转换
//      ref 2.048 采用ref3032芯片 推荐值AGND - VDD-1.5V、输出值为2*ref*Dignum/4095
//     tlv5618有A B 2通道输出，分别接 p60 p64   
//==========================================================

#include <msp430x14x.h>

#define  uint  unsigned int
#define  uchar unsigned char

#define Channal_A     1    //通道A
#define Channal_B     2    //通道B
#define Channal_AB    3    //通道A&B


#define DIN_H    P6OUT|=BIT3      //p63DIN O
#define DIN_L    P6OUT&=~BIT3     //p63DIN O
#define SCLK_H   P6OUT|=BIT2      //p62SCLK O   
#define SCLK_L   P6OUT&=~BIT2     //p62SCLK O
#define CS_H     P6OUT|=BIT1      //p61CS   O
#define CS_L     P6OUT&=~BIT1     //p61CS   O

#define LCD_DataIn    P4DIR=0X00       //数据口方向设置为输入
#define LCD_DataOut   P4DIR=0XFF       //数据口方向设置为输出
#define LCD2MCU_Data  P4IN
#define MCU2LCD_Data  P4OUT
#define LCD_RS_H      P5OUT|=BIT5      //P5.5
#define LCD_RS_L      P5OUT&=~BIT5     //P5.5
#define LCD_RW_H      P5OUT|=BIT6      //P5.6
#define LCD_RW_L      P5OUT&=~BIT6     //P5.6
#define LCD_EN_H      P5OUT|=BIT7      //P5.7
#define LCD_EN_L      P5OUT&=~BIT7     //P5.7

 

//static unsigned    char  adc_flag = 0 ;  
static unsigned    char  count = 0 ; //采样次数标记
static unsigned    int   rst=0;


void int_clk();

void int_adc();

void Delay(unsigned int t);   //延时延时t毫秒


////////////lcd显示相关函数

void RDbf(void) ;  //读忙标志

void Lcd_Reset() ;//lcd初始化

void Lcd_WriteData(unsigned char Data);//写数据

void Lcd_WriteCmd(unsigned char CmdCode); //写指令

void Lcd_Clear(void);//清楚GDRAM内容

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str);//写字符串

void DA_Conver(uint Dignum);
void Dac_Write(uint Data_A,uint Data_B,uchar Channal,uchar Model);





void Dac_Init(void)
{
  P6SEL=0x00;
  P6DIR=0x6E;     //p64,p60分别接OUTB和OUTA，定义为输入寄存
  P6OUT=0x00;
}

//================================================================= 
//   void DA_conver(uint Dignum)

void DA_Conver(uint Dignum)
{
uint Dig=0;
uchar i=0;
SCLK_H;
CS_L;                //片选有效
for(i=0;i<16;i++)   //写入16为Bit的控制位和数据
{
   Dig=Dignum&0x8000;
   if(Dig) 
   {
    DIN_H;
   }
   else
   {
    DIN_L;
   }
   SCLK_L;
   _NOP();
   Dignum<<=1;
   SCLK_H;
   _NOP();
}
SCLK_H;
CS_H;       //片选无效
}
//================================================================= 
// 函数名称 ：void Write_A_B(uint Data_A,uint Data_B,uchar Channal,bit Model)
// 函数功能 ：模式、通道选择并进行DA转换 
// 入口参数 ：Data_A：A通道转换的电压值
//            Data_B：B通道转换的电压值
//            Channal：通道选择，其值为Channal_A，Channal_B,或Channal_AB
//            Model：速度控制位 0：slow mode 1：fast mode
// 出口参数 ：无
// 说明：     Data_A，Data_B的范围为：0-0x0fff
//            本程序如果只需要一个通道时，另外一个通道的值可任意，但是不能缺省
//=================================================================
void Dac_Write(uint Data_A,uint Data_B,uchar Channal,uchar Model)
{
uint Temp;
if(Model) 
{
   Temp=0x4000;        //快速基本
}
else 
    {
   Temp=0x0000;       //慢速基本
}
switch(Channal)
{
    case Channal_A:         //A通道
         DA_Conver(Temp|0x8000|(0x0fff&Data_A));
      break; 
    case Channal_B:         //B通道
         DA_Conver(Temp|0x0000|(0x0fff&Data_B));
    break; 
    case Channal_AB:        //A&B通道
         DA_Conver(Temp|0x1000|(0x0fff&Data_B));       
         DA_Conver(Temp|0x8000|(0x0fff&Data_A));
    break;
    default:
         break;
}
}

//初始化时钟
void int_clk() 
{uchar i;
  BCSCTL1&=~XT2OFF;                  //打开XT振荡器
  BCSCTL2|=SELM1+SELS;               //ＭＣＬＫ８Ｍｈｚ，ＳＭＣＬＫ为１Ｍｈｚ
  do
  {
    IFG1 &= ~OFIFG;
    for(i=0;i<100;i++)
      _NOP();
  }
  while((IFG1&OFIFG)!=0);
    IFG1&=~OFIFG;
}



//初始化ADC12，查询方式
void int_adc_check()           
{
  P6SEL|=0X01;//Ｐ６功能选择，ｐ６0选择第二功能，Ａ0，选择AD通道
  //设置ENC为0，从而修改ADC12寄存器的值
  ADC12CTL0 &= ~(ENC);
  
  // 内核开启, 启动内部基准, 选择2.5V基准, 设置采样保持时间
  ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
  // 时钟源为内部振荡器, 出发信号来自采样定时器, 转换地址为ADC12MCTL4
  ADC12CTL1 = ADC12SSEL_0 + SHP + CSTARTADD_4;
  // 转换通道设置
  ADC12MCTL4 = SREF_1 + INCH_0; // 参考电压:V+=Vref+,V-=AVss ADC通道:A0
  // 启动转换
  ADC12CTL0 |= ENC + ADC12SC;    // 转换使能开始转换
  while((ADC12IFG & 0x0010) == 0); // 软件查询中断标志, 等待转换结束
  count++;
  rst=rst+ADC12MEM4;
  Delay(3000);           //延时3秒
  if(count==10)
  {
   //rst=ADC12MEM4;
   //rst=(rst*2.5/4096);
    rst=rst/10;
    uchar str[6];
    long int temp=rst;
   
    str[0]=rst/1000+48;      //千位
    str[1]=(rst/100)%10+48;  //百位
    str[2]=(rst/10)%10+48;   //十位
    str[3]=rst%10+48;        //个位
    str[4]='\0';
    Lcd_WriteStr(5,2,str);
  
    temp=(temp*250)/4095;
    str[0]=temp/100+48;  
    str[1]='.';
    str[2]=(temp/10)%10+48;
    str[3]=(temp%10)+48;
    str[4]='V';
    str[5]='\0';
    Lcd_WriteStr(5,1,str);
    rst=0;
    count=0;
    Delay(3000);             //延时3秒
  }
}


/********
延时t毫秒 
********/
void Delay(unsigned int t)
{
  unsigned int i,j;
  for(i=0;i<t;i++)
    for(j=150;j>0;j--) 
      _NOP(); 
}
 
/******************* 
向 LCD 写入1字节数据 
*******************/ 


/********** 
检测忙标志 
***********/  
void RDbf(void) 
{ 
  while(1)
  {
    LCD_RS_L;
    LCD_RW_H;
    LCD_EN_L;
    LCD_DataIn;
    LCD_EN_H;
    if((LCD2MCU_Data&BIT7)==0)
    break;
  } 
}
void Lcd_WriteData(unsigned char Data) 
{ 
  RDbf(); 
  LCD_RS_H; 
  LCD_RW_L; 
  LCD_EN_H;
  LCD_DataOut;
  MCU2LCD_Data=Data; 
  LCD_EN_L;
} 

/*********************
向 LCD 中写入指令代码 
*********************/ 
void Lcd_WriteCmd(unsigned char CmdCode) 
{
    RDbf(); 
    LCD_RS_L; 
    LCD_RW_L; 
    LCD_EN_H; 
    LCD_DataOut;
    MCU2LCD_Data=CmdCode;  
    LCD_EN_L; 
}

/********************************
向 LCD 指定起始位置写入一个字符串
********************************/
void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) 
{
    Lcd_WriteCmd(0x30);
    switch(y) 
    { 
        case 0: Lcd_WriteCmd(0x80+x); break;
        case 1: Lcd_WriteCmd(0x90+x); break; 
        case 2: Lcd_WriteCmd(0x88+x); break; 
        case 3: Lcd_WriteCmd(0x98+x); break; 
    } 
    while(*Str>0)
    { 
        Lcd_WriteData(*Str);
        Str++; 
    }  
}

/********************
清除液晶GDRAM中的数据
********************/
void Lcd_Clear(void)
{
    unsigned char i,j,k;
    Lcd_WriteCmd(0x34);        //打开扩展指令集
    i = 0x80;            
    for(j = 0;j < 32;j++)
    {
        Lcd_WriteCmd(i);
        Lcd_WriteCmd(0x80);
  	for(k = 0;k < 16;k++)
	    Lcd_WriteData(0x00);
        Lcd_WriteCmd(i++);
        Lcd_WriteCmd(0x88);	   
  	for(k = 0;k < 16;k++)
	    Lcd_WriteData(0x00);
    }
}
/********* 
LCD 初始化 
*********/
void Lcd_Reset() 
{ 
    Lcd_WriteCmd(0x30);        //选择基本指令集
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x0c);        //开显示(无游标、不反白)
    Lcd_Clear();               //清除液晶GDRAM中的数据
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x06);        //在资料的读取及写入时游标自动右移
}




int main(void)
{
    P1SEL = 0;
    P2SEL = 0;
    P3SEL = 0;
    P4SEL = 0;
    P5SEL = 0;
    P6SEL = 0;

   /*下面六行程序关闭所有的IO口*/
    P1DIR = 0XFF;P1OUT = 0XFF;                 // P1DIR = 0XF0;P1OUT = 0X0F;
    P2DIR = 0XFF;P2OUT = 0XFF;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0X7F;P6OUT = 0xFF; 
    //P6IN  = 0X00; 
 
 
   WDTCTL=WDTPW+WDTHOLD;                     //关狗
   
   int_clk();                                //初始化时钟
  
   Lcd_Reset();
   Lcd_WriteStr(0,1,"AD12");              //中文和字符分开调用显示否则乱码
   Lcd_WriteStr(2,1,"电压为");    
   
   Dac_Init();
  
  //Write_A_B(0x0fff,0x07ff,Channal_AB,1);   //测量AB通道，测量时需屏蔽上面两句
                                             //实测不如上面分开方法
   _EINT();                                  //使能中断
   while(1)
   {
   Dac_Write(0x0fff,0x0000,Channal_A,1);     //测量A通道，模拟输出为2*REF*Dignum/0x0fff
   Dac_Write(0x0000,0x07ff,Channal_B,1);    //测量B通道
   int_adc_check();                        //初始化adc
   }
}
