/*
MSP430F149 ADC12 单通道多路采样 
P67为采集端
*/

#include <msp430x14x.h>

#define  uint unsigned int
#define  uchar unsigned char


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

static unsigned    int   rst0 =0;//存放采样数 66
static unsigned    int   rst1 =0;//存放   67
static unsigned    int   rst =0;//存放   67
void int_clk();


void Delay(unsigned int t);   //延时延时t毫秒


////////////lcd显示相关函数

void RDbf(void) ;  //读忙标志

void Lcd_Reset() ;//lcd初始化

void Lcd_WriteData(unsigned char Data);//写数据

void Lcd_WriteCmd(unsigned char CmdCode); //写指令

void Lcd_Clear(void);//清楚GDRAM内容

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str);//写字符串

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //具有反白功能任意点打点

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//任意点打点

void gui_hline(unsigned char x0, unsigned char x1, unsigned char y);//画水平线

void gui_rline(unsigned char x, unsigned char y0, unsigned char y1);//画垂直线

void Adc_Check(); 

void Adc_Figure();

void Adc_Str(); 

//初始化时钟
void int_clk() 
{
  uchar i;
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


// 初始化ADC12，查询方式
void Adc_Check()           
{
  /*
  P6SEL|=0X01;  //Ｐ６功能选择，ｐ６７选择第二功能，Ａ７，选择AD通道
                //设置ENC为0，从而修改ADC12寄存器的值
  
  ADC12CTL0 &= ~(ENC); 
  ADC12CTL0|=ADC12ON+SHT0_2+REF2_5V+REFON;  //ＡＤＣ１２电源开，打开并选择内部参考电压为2.5v，采样保持时间为16个ADC12CLK
  
  //ADC12CTL1=SHP+CONSEQ_2;                 //采样保持脉冲选择采样时序电路产生的信号，单路重复转换
  ADC12CTL1|=ADC12SSEL1+ADC12SSEL1;
  ADC12MCTL0=0X10;   //？？
  ADC12IE|= 0X01;    //使能转换中断p67
  ADC12CTL0|=ENC;    //ADC转换使能
  */
  
  P6SEL|=0X80;  //Ｐ６功能选择，ｐ６７选择第二功能，Ａ７，选择AD通道
  //设置ENC为0，从而修改ADC12寄存器的值
  ADC12CTL0 &= ~(ENC);  
  // 内核开启, 启动内部基准, 选择2.5V基准, 设置采样保持时间
  ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
  // 时钟源为内部振荡器, 出发信号来自采样定时器, 转换地址为ADC12MCTL4
  ADC12CTL1 = ADC12SSEL_0 + SHP + CSTARTADD_4;
  // 转换通道设置
  ADC12MCTL4 = SREF_1 + INCH_7; // 参考电压:V+=Vref+,V-=AVss ADC通道:A0
  // 启动转换
  ADC12CTL0 |= ENC + ADC12SC;    // 转换使能开始转换
  
  while((ADC12IFG & 0x0010) == 0); // 软件查询中断标志, 等待转换结束
  count++;
  rst=rst+ADC12MEM4;
  Delay(1000);            
}



void Adc_Str()
{
   Lcd_WriteStr(0,0,"AD12");              //中文和字符分开调用显示否则乱码
   Lcd_WriteStr(2,0,"电压为");
   Lcd_WriteStr(0,2,"AD12");              //中文和字符分开调用显示否则乱码
   Lcd_WriteStr(2,2,"电压为");
   
  if(count==10)
  {
   //rst=ADC12MEM4;
   //rst=(rst*2.5/4096);
    //67 10次数据
    long int rst = rst0;
    
    rst=rst/10;
    uchar str[6];
    long int temp = rst;
    
    str[0]=rst/1000+48;      //千位
    str[1]=(rst/100)%10+48;  //百位
    str[2]=(rst/10)%10+48;   //十位
    str[3]=rst%10+48;        //个位
    str[4]='\0';
    Lcd_WriteStr(5,1,str);
    Lcd_WriteStr(0,1,"对应数字量");
    
    temp=(temp*250)/4095;
    str[0]=temp/100+48;  
    str[1]='.';
    str[2]=(temp/10)%10+48;
    str[3]=(temp%10)+48;
    str[4]='V';
    str[5]='\0';
    Lcd_WriteStr(5,0,str);
    
    
    //67 10次
    rst = rst1;
    rst=rst/10;
    temp = rst;
    
    str[0]=rst/1000+48;      //千位
    str[1]=(rst/100)%10+48;  //百位
    str[2]=(rst/10)%10+48;   //十位
    str[3]=rst%10+48;        //个位
    str[4]='\0';
    Lcd_WriteStr(5,3,str);
    Lcd_WriteStr(0,3,"对应数字量");
    
    temp=(temp*250)/4095;
    str[0]=temp/100+48;  
    str[1]='.';
    str[2]=(temp/10)%10+48;
    str[3]=(temp%10)+48;
    str[4]='V';
    str[5]='\0';
    Lcd_WriteStr(5,2,str);
    
    rst0 = 0;
    rst1=0;
    count=0;
    Delay(3000);             //延时3秒
  }
}

void Adc_Figure()
{
  static uint i=0;
  static uint j=7;
  if(count==10)
  {
  gui_rline(0,0,63);
  gui_hline(0,127,7);
  if(i==127)
  {
    i=0;
    Lcd_Clear();
  }
  else
  {
    j=rst/10/82;
    lcd_set_dot(i,j,1);
    i++;
  }
  count=0;
  rst=0;
  Delay(1000);
  }
}
/*中断式
#pragma vector=ADC_VECTOR

__interrupt void AD12ISR( void)        //adc中转换中断
{
  //while((ADC12CTL1&0x01)==1);
 // uint results[10];
  adc_flag = 1 ;                       //转换标志置1
  rst=ADC12MEM0;          //adc12mem0 存放内容  赋给对应数组元素，
//  rst+=results[count];
//  count++;                           //数组序号递增

}
*/

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


/******************* 
向 LCD 写入1字节数据 
*******************/ 

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

/*********************
向 LCD 中读出1字节数据
*********************/
unsigned char Lcd_ReadData(void)
{
    unsigned char temp;
    RDbf();
    LCD_RS_H;
    LCD_RW_H;
    LCD_EN_H;
    LCD_DataIn;
    temp=LCD2MCU_Data;
    LCD_EN_L;
    return temp;
}

/***********************************************
有反白显示功能的打点函数:(X-水平位址,Y-竖直位址)
************************************************/
void GUI_Point(unsigned char x,unsigned char y,unsigned char color)
{
    unsigned char x_Dyte,x_byte;		//定义列地址的字节位，及在字节中的哪1位
    unsigned char y_Dyte,y_byte;		//定义为上下两个屏(取值为0，1)，行地址(取值为0~31)
    unsigned char GDRAM_hbit,GDRAM_lbit;
    Lcd_WriteCmd(0x36);		                //扩展指令命令
    x_Dyte=x/16;				//计算在16个字节中的哪一个
    x_byte=x&0x0f;				//计算在该字节中的哪一位
    y_Dyte=y/32;				//0为上半屏，1为下半屏
    y_byte=y&0x1f;				//计算在0~31当中的哪一行
    Lcd_WriteCmd(0x80+y_byte);			//设定行地址(y坐标)
    Lcd_WriteCmd(0x80+x_Dyte+8*y_Dyte);		//设定列地址(x坐标)
    Lcd_ReadData();				//预读取数据
    GDRAM_hbit=Lcd_ReadData();			//读取当前显示高8位数据
    GDRAM_lbit=Lcd_ReadData();			//读取当前显示低8位数据
    Lcd_WriteCmd(0x80+y_byte);			//设定行地址(y坐标)
    Lcd_WriteCmd(0x80+x_Dyte+8*y_Dyte);		//设定列地址(x坐标)
    if(x_byte<8)				//判断其在高8位，还是在低8位
    {
      if(color==1)
      {
        Lcd_WriteData(GDRAM_hbit|(0x01<<(7-x_byte))); //置位GDRAM区高8位数据中相应的点
      }
      else
      Lcd_WriteData(GDRAM_hbit&(~(0x01<<(7-x_byte))));//清除GDRAM区高8位数据中相应的点	
      Lcd_WriteData(GDRAM_lbit);		        //显示GDRAM区低8位数据
    }
    else
    {
      Lcd_WriteData(GDRAM_hbit);
      if(color==1)
        Lcd_WriteData(GDRAM_lbit|(0x01<<(15-x_byte)));//置位GDRAM区高8位数据中相应的点
      else
        Lcd_WriteData(GDRAM_lbit&(~(0x01<<(15-x_byte))));//清除GDRAM区高8位数据中相应的点	
    }
    Lcd_WriteCmd(0x30);			        //恢复到基本指令集
}

/***************************
(给定坐标的)任意位置打点函数
***************************/
void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color)
{
    GUI_Point(x,63-y,color);
}

/*******************************************************
画水平线函数,x0、x1为起始点和终点的水平坐标，y为垂直坐标
*******************************************************/
void gui_hline(unsigned char x0, unsigned char x1, unsigned char y)
{
    unsigned char bak;            //用于对两个数互换的中间变量，使x1为大值
    if(x0>x1)
    {
      bak=x1;
      x1=x0;
      x0=bak;
    }
    do
    {
      lcd_set_dot(x0,y,1);                      //从左到右逐点显示
      x0++;	
    }while(x1>x0);
}

/*******************************************************
画竖直线函数,x为水平坐标，y0、y1为起始点和终点的垂直坐标
*******************************************************/
void gui_rline(unsigned char x, unsigned char y0, unsigned char y1)
{
    unsigned char bak;                          //用于对两个数互换的中间变量，使y1为大值
    if(y0>y1)
    {
      bak=y1;
      y1=y0;
      y0=bak;
    }
    do
    {
      lcd_set_dot(x,y0,1);                      //从上到下逐点显示
      y0++;	
    }while(y1>=y0);
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

//-----------------------------------------------------------------------
// ADC12序列通道单次转换
void Adc_2Int(void)
{
// ADC12控制寄存器设置
ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
// CONSEQ_1表示当前模式为序列通道单次转换, 起始地址为ADC12MCTL4, 结束地址ADC12MCTL6
ADC12CTL1 = ADC12SSEL_0 + SHP + CONSEQ_1 + CSTARTADD_4;
// 转换通道设置
ADC12MCTL4 = SREF_1 + INCH_7; // 参考电压:V+=Vref+,V-=AVss ADC通道:A7
ADC12MCTL5 = SREF_1 + INCH_6+EOS; // 参考电压:V+=Vref+,V-=AVss ADC通道:A6
ADC12MCTL6 = SREF_1 + INCH_10 + EOS; // 参考电压:V+=Vref+,V-=AVss ADC通道:片内温度传感器
// 中断允许
ADC12IE = 0x0040;
_EINT();
// 启动转换
ADC12CTL0 |= ENC + ADC12SC; // 转换使能开始转换
__low_power_mode_0(); // 进入低功耗模式, 等待转换结束

}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// ADC12单通道单次转换(中断查询式)
void Adc_Int(void)
{
P6SEL|=0XC0;  //Ｐ６功能选择，ｐ６７选择第二功能，Ａ７，选择AD通道
//设置ENC为0，从而修改ADC12寄存器的值
ADC12CTL0 &= ~(ENC);  
// ADC12控制寄存器设置
ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2 ;
ADC12CTL1 = ADC12SSEL_0 + SHP + CONSEQ_2+ CSTARTADD_4;
// 转换通道设置
ADC12MCTL4 = SREF_1 + INCH_7; // 参考电压:V+=Vref+,V-=AVss ADC通道:A7
ADC12MCTL5 = SREF_1 + INCH_6;
// 中断允许
ADC12IE = 0x00ff;
_EINT();
// 启动转换
ADC12CTL0 |= ENC + ADC12SC; // 转换使能开始转换
__low_power_mode_0(); // 进入低功耗模式, 等待转换结束
}

// ADC12中断向量
#pragma vector = ADC_VECTOR
__interrupt void ADC12_IRQ(void)
{
  rst0 += ADC12MEM4;          //adc12mem4 存放内容  p67  赋给对应数组元素，
  rst1 += ADC12MEM5;          //p66
  count++;                   //数组序号递增
  __low_power_mode_off_on_exit(); //中断结束时, 退出低功耗模式
}
//-----------------------------------------------------------------------

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
    P2DIR = 0XFF;P2OUT = 0X00;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0X7F;P6OUT = 0xFF; 
    //P6IN  = 0X00; 
    
   WDTCTL=WDTPW+WDTHOLD;                     //关狗
   
   int_clk();                                //初始化时钟
  
   Lcd_Reset();
   
   _EINT();                                  //使能中断
   
   while(1)
   {
    Adc_Int( );
    Adc_Str();
   }
  
}
