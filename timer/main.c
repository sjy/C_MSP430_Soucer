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

uint    flag = 0; //中断标志
uint    a = 0;    //读CCR1
uint    cd = 0;   //TAIFG溢出次数

unsigned long t = 0,sum = 0;
unsigned long results[8];


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

//显示频率
void Display(unsigned long f);





//初始化时钟
void int_clk() 
{
  uchar i;
  BCSCTL1&=~XT2OFF;                  //打开XT振荡器
  BCSCTL2|=SELM_2+SELS;               //ＭＣＬＫ８Ｍｈｚ，ＳＭＣＬＫ为１Ｍｈｚ
  do
  {
    IFG1 &= ~OFIFG;
    for(i=0;i<100;i++)
      _NOP();
  }
  while((IFG1&OFIFG)!=0);
    IFG1&=~OFIFG;
}

void Lcd_Reset() 
{ 
    Lcd_WriteCmd(0x30);        //选择基本指令集
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x0c);        //开显示(无游标、不反白)
    Lcd_Clear();               //清除液晶GDRAM中的数据
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x06);        //在资料的读取及写入时游标自动右移
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


void Display(unsigned long sum)
{ 
   unsigned long f;
   sum>>=3;            //除以8，求平均
   f=8*1000000/sum;    //8M/sum ,求频率
   
   uchar ptr[5];
   if(f>=1000000)//M,2
     { 
        ptr[0]=f/10000000+48;
        ptr[1]=f%10000000/1000000+48;
        ptr[4]='\0';
        Lcd_WriteStr(0,0,ptr);
     }
    else if( (f<1000000) && (f>=1000) )//k,3
     {
        ptr[0]=f/100000+48;
        ptr[1]=f%100000/10000+48;
        ptr[2]=f%10000/1000+48;
        ptr[4]='\0';
        Lcd_WriteStr(0,0,ptr);
     }
  else
     {  
       // f+=1;
        ptr[0]=f/100+48;
        ptr[1]=f%100/10+48;
        ptr[2]=f%10+48;
        ptr[4]='\0';
        Lcd_WriteStr(0,0,ptr);
     }
}

uint new_cap=0;
uint old_cap=0;
uint cap_diff=0;

uint diff_array[16];
uint capture_array[16];
uchar index=0;
uchar count=0;
void main(void)
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
   /* 字符显示
   Lcd_WriteStr(0,1,"AD12");              //中文和字符分开调用显示否则乱码
   Lcd_WriteStr(2,1,"电压为");
   */  
   
   
    TACTL|=TASSEL_2+TACLR+TAIE+ MC_2;//选择MCLK；清除 TAR，时钟分频，计数模式的设置；允许定时器中断;选择增加模式
    
   
    TACCTL1|=CAP+CM_1+SCS+CCIE+CCIS_0;    //选择捕获模式；设置为：上升沿捕获；选择输入源为CCI1A P1.1；同步捕获；中断允许
    
    //TACTL|=TASSEL1+MC1;
    
    P2SEL|=0x04;
    P2DIR|= BIT2;                    //P2.0（ACLK）输出          
    //P2OUT&=~0X01;                  //P2.0置高电位
    
    P1SEL=0x04;                     //P1.1选择外围器件模式，作为捕捉的输入端
    P1DIR&=~BIT2;                      //P1.1,P1.2方向输入
    
    P3SEL=0x00;
    P3DIR=0xff;
    P3OUT&=~0x30;
    _EINT();                             //开总中断
   
    
        while(flag)
        {
          
        flag=0;
        
        for(uint nu=0;nu<8;nu++)//记录8次
        {
          sum+=results[nu];   
        }        
        
        Display(sum);          //显示值
        
        Delay(200);
        
        sum=0;
        
        TACTL|=TACLR+TAIE;    //清除 TAR，时钟分频，计数模式的设置；允许定时器中断   
        
        TACCTL1|=CCIE;        //允许中断（TACCR1）
        }
}



#pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A(void)
{ 
  static uint index;
  switch(TAIV)
  {
  case 2:
    {
      t=cd*(65535+1)+CCR1-a; //CCR1的CCIFG，计算每次实际计数差值值
          a=CCR1;                                   
          cd=0; 
          results[index]=t; //存储结果
          index++;
           if(index==8)       //计数存储次数
            { 
              index=0;       //个数清零
              TACCTL1&=~CCIE;  //关中断CCR1
              TACTL&=~TAIE;  //关定时器中断
              flag=1;        //中断标志归位，主程序显示
            }         
    }
          break;
  case 4: break;              //CCR2 的 CCIFG
  case 10:cd++;
          P3OUT^=0x0ff;  
          break;        //TAIFG，计数溢出次数
  }    
}



/*   
#pragma vector=TIMERA0_VECTOR
__interrupt void TimerA0(void)
{
  new_cap=TACCR0;
  cap_diff=new_cap-old_cap;
  
  diff_array[index]=cap_diff;
  capture_array[index++]=new_cap;
  if(index==16)
  {
    index=0;
    Display(10000000);
    Delay(1000);
    P3OUT^=0x30;
  }
  old_cap=new_cap;
  count++;
  if(count==32)
  {
    count=0;
    _NOP();
  }
}
*/