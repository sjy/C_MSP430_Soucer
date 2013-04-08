#include  <msp430x14x.h>
//**********************************************************************
//    ads 8519 驱动程序 spi协议
//    P6.0 单片机读忙标志，低，则在转换 ；P6.1 单片机写 RC 选择位，低则选择装换
//    P6.2 单片机读 16位数字数据        ；P6.3 单片机读数据时钟
//**********************************************************************

#define  uchar  unsigned char
#define  uint   unsigned int


#define CTL_RC1     P6OUT |=  BIT1//选择读取数据
#define CTL_RC0     P6OUT &= ~BIT1 //选择转换数据
#define BUSY_PIN    P6IN&0x01   //忙标志位
#define DATA_PIN    P6IN&0X04   //读数据位
#define DATACLK_PIN P6IN&0x08   //数据时钟位;内部时钟


////显示端口定义
#define LCD_DataIn    P4DIR=0X00       //数据口方向设置为输入
#define LCD_DataOut   P4DIR=0XFF       //数据口方向设置为输出
#define LCD2MCU_Data  P4IN             //从液晶读数据
#define MCU2LCD_Data  P4OUT            //向液晶写数据

////指令定义
#define LCD_RS_H      P5OUT|=BIT5      //P5.5
#define LCD_RS_L      P5OUT&=~BIT5     //P5.5
#define LCD_RW_H      P5OUT|=BIT6      //P5.6
#define LCD_RW_L      P5OUT&=~BIT6     //P5.6
#define LCD_EN_H      P5OUT|=BIT7      //P5.7
#define LCD_EN_L      P5OUT&=~BIT7     //P5.7


static uchar  count = 0 ;//采样次数标记
static unsigned long   rst=0;     //存放采样值



void Wdt_Init();//关闭看门狗

void Delay(unsigned int t); //延时延时t毫秒

void Lcd_Reset() ;//lcd初始化

void RDbf(void) ;  //检测忙标志

void Lcd_WriteData(unsigned char Data);//写数据

void Lcd_WriteCmd(unsigned char CmdCode); //写指令

uchar Lcd_ReadData(void);//读数据（一字节）

void Lcd_Clear(void);//清楚GDRAM内容

uint Ads_Convert();

void Ads_Str();



//////////////////////////画图函数申明
void Lcd_WritePic(const unsigned char * adder );  //水平扫描，上下屏显示

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) ;//制定位置写入字符串

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //具有反白功能任意点打点

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//任意点打点






//**********************************************************************
//                   MSP430内部看门狗初始化
//**********************************************************************
void Wdt_Init()
{
   WDTCTL = WDTPW + WDTHOLD;       //关闭看门狗
}

//**********************************************************************
//                   MSP430时钟初始化
//**********************************************************************
void Clock_Init()
{
  uchar i;
  BCSCTL1&=~XT2OFF;
  BCSCTL2|=SELM1+SELS;
  do{
    IFG1&=~OFIFG;
    for(i=0;i<100;i++)
       _NOP();
  }
  while((IFG1&OFIFG)!=0);
  IFG1&=~OFIFG; 
}
  

//**********************************************************************
//                         延时t毫秒 
//**********************************************************************
void Delay(unsigned int t)
{
  unsigned int i,j;
  for(i=0;i<t;i++)
    for(j=150;j>0;j--) 
      _NOP(); 
}
//**********************************************************************
//                        检测忙标志 
//**********************************************************************
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

//**********************************************************************
//                   向 LCD 写入1字节数据 
//**********************************************************************
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

//**********************************************************************
//                  向 LCD 中写入指令代码 
//********************************************************************** 
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


//**********************************************************************
//                    向 LCD 中读出1字节数据
//**********************************************************************
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

//**********************************************************************
//              向 LCD 指定起始位置写入一个字符串
//**********************************************************************
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

//**********************************************************************
//                    清除液晶GDRAM中的数据
//**********************************************************************
void Lcd_Clear(void)
{
    unsigned char i,j,k;
    Lcd_WriteCmd(0x34);         //打开扩展指令集
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

//**********************************************************************
//         有反白显示功能的打点函数:(X-水平位址,Y-竖直位址)
//**********************************************************************
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
        Lcd_WriteData(GDRAM_hbit|(0x01<<(7-x_byte)));//置位GDRAM区高8位数据中相应的点
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



//**********************************************************************
//                (给定坐标的)任意位置打点函数
//**********************************************************************
void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color)
{
    GUI_Point(x,63-y,color);
}

//**********************************************************************
//                         LCD 初始化 
//**********************************************************************
void Lcd_Reset() 
{ 
    Lcd_WriteCmd(0x30);        //选择基本指令集
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x0c);        //开显示(无游标、不反白)
    Lcd_Clear();               //清除液晶GDRAM中的数据
    Lcd_WriteCmd(0x01);        //清除显示，并且设定地址指针为00H
    Lcd_WriteCmd(0x06);        //在资料的读取及写入时游标自动右移
}



//**********************************************************************
//
//**********************************************************************
uint Ads_Convert()
{
  uint temp=0;        //无符号int 2字节 0x0000-0xffff;
  uchar i;
  
  
  CTL_RC0;          //RC 负脉冲
  CTL_RC1;
  while(BUSY_PIN);  //Busy 为高，等待，直到置低（转换开始）
  for(i = 0;i<16;i++)// 16bit
  {
    while(DATACLK_PIN);//  dataclock为低时等待
    if(DATA_PIN)      //将数据位采集的数据转换为10进制
    {
      temp++;
      temp = temp*2;     //每次均会左移一位，先收到的是高位数据
    }
    else 
      temp =temp * 2;
    while(!DATACLK_PIN);//等待dataclock置低，开始新一轮转换
  }
     
  return temp;  //返回以此转换的十进制数据值
  
}

//**********************************************************************
//   以字符形式显示测试的ADS8519值
//**********************************************************************
void Ads_Str()
{
  
   Lcd_WriteStr(0,1,"AD16");              //中文和字符分开调用显示否则乱码2^16=65536 0x8000-ox7FFF
   Lcd_WriteStr(2,1,"电压为");            //正数 0-32767,负数 65535-32768
   Lcd_WriteStr(0,2,"对应数字量");
   uchar str[6];
   
   // if(rst>65535)
   // rst=65536;
     
    long int temp=rst;
    
    str[0]=rst/10000+48;
    str[1]=(rst/1000)%10+48; //千位
    str[2]=(rst/100)%10+48;  //百位
    str[3]=(rst/10)%10+48;   //十位
    str[4]=rst%10+48;        //个位
    str[5]='\0';
    Lcd_WriteStr(5,2,str);
   
 // if(temp>32767)
 // temp=65536-temp;
    temp=(temp*10000)/32768;  //系数不精准为满幅电压，此处选为10000，斜率根据调整
    str[0]=temp/1000+48;  
    str[1]='.';
    str[2]=(temp/100)%10+48;
    str[3]=(temp/10)%10+48;
    str[4]=temp%10+48;
    str[5]='\0';
    Lcd_WriteStr(5,1,str);
}

int main (void)
{
    P1DIR = 0XFF;P1OUT = 0XFF;               // P1DIR = 0XF0;P1OUT = 0X0F;
    P2DIR = 0XFF;P2OUT = 0XFF;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0XFF;P6OUT = 0XFF; 
  
  Wdt_Init();
  Clock_Init();
  Lcd_Reset();
  //Lcd_WriteStr(0,0,"yes");
  
  P6SEL=0x00;
  P6DIR=0x02;
  
  //CTL_RC1;

  while(1)
  {
  rst=Ads_Convert();
  Delay(100);
  Ads_Str();
  }
  /*
  while(1)
  {
  if(count<10)
  {
    Ads_Convert();//系统误差，调整截距
    Delay(1000);
    count++;
  }
  else
  {
    count=0;
    rst/=10;
    Ads_Str();
    Delay(1000);
    rst=0;
  }
  }
  */
}