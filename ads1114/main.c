#include <msp430x14x.h>

//****************************************************************************
//ads11114模块   基于I2C协议
//P4.0  Ready 单片机读 ;  P4.1 SCL 单片机写 ; P4.2   SDA 单片机读
// 转换结果 截距调整在main  斜率在 Adc_Str函数里
//SPS 选择128b/s “100” PGA选择“000” 2/3 FS6.114  在Configue_AD里设置
//****************************************************************************

unsigned char InitData[4] = {0};

#define  CMD_Write        0x90  //B10010000
#define  CMD_Read         0x91  //B10010001
#define  CMD_CONF_REG     0x01  //B00000001
#define  CMD_CONV_REG     0x00  //B00000000 
#define  CONF_L           0xe3  //B11100011

#define  SDA_OUT          P4DIR |= BIT0    //P4.0 SDA
#define  SDA_IN           P4DIR &= ~BIT0

#define  SDAOUT_HIGH      P4OUT |= BIT0    //
#define  SDAOUT_LOW       P4OUT &= ~BIT0    

#define  ADSCK_HIGH       P4OUT |= BIT1    //P4.1 SCK
#define  ADSCK_LOW        P4OUT &= ~BIT1   //

#define  Ready            (P4IN&BIT2)      //P4.2  RDY
#define  SDAIN            (P4IN&BIT0)      //


void Start_ADcom();
void Stop_ADcom();
void  Send_Byte(unsigned char Byte);
uchar Read_Byte();

void  Confige_AD(unsigned char Channel_Val);
void  Point_ConversionRegister();
uint  Read_Data();
uint  AD_ReadData(unsigned char Channel_Ad);



#define uchar unsigned char
#define uint  unsigned int
#define ulong unsigned long

uchar key_new=0;   //存储键值
uchar key_old=0;

static unsigned    char  count = 0 ;//采样次数标记
static unsigned    long   rst=0;     //存放采样值


////端口定义
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



void Wdt_Init();//关闭看门狗

void Delay(unsigned int t); //延时延时t毫秒

void Lcd_Reset() ;//lcd初始化

void RDbf(void) ;  //检测忙标志

void Lcd_WriteData(unsigned char Data);//写数据

void Lcd_WriteCmd(unsigned char CmdCode); //写指令

unsigned char Lcd_ReadData(void);//读数据（一字节）

void Lcd_Clear(void);//清楚GDRAM内容


//////////////////////////画图函数申明
void Lcd_WritePic(const unsigned char * adder );  //水平扫描，上下屏显示

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) ;//制定位置写入字符串

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //具有反白功能任意点打点

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//任意点打点

void gui_hline(unsigned char x0, unsigned char x1, unsigned char y);//画水平线

void gui_rline(unsigned char x, unsigned char y0, unsigned char y1);//画垂直线

void gui_line(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1);//任意两点画线

////////ads11114
//  引脚说明   P40  
//
//
void Adc_Str()
{
  
   Lcd_WriteStr(0,1,"AD16");              //中文和字符分开调用显示否则乱码2^16=65536 0x8000-ox7FFF
   Lcd_WriteStr(2,1,"电压为");            //正数 0-32767,负数 65535-32768
   Lcd_WriteStr(0,2,"对应数字量");
   uchar str[6];
   
    
   if(rst>65535)
     rst=65536;
     
    long int temp=rst;
    
    str[0]=rst/10000+48;
    str[1]=(rst/1000)%10+48; //千位
    str[2]=(rst/100)%10+48;  //百位
    str[3]=(rst/10)%10+48;   //十位
    str[4]=rst%10+48;        //个位
    str[5]='\0';
    Lcd_WriteStr(5,2,str);
   
    if(temp>32767)
    temp=65536-temp;
    temp=(temp*6300)/32768;  //系数不精准为满幅电压，此处选为6.114，斜率根据调整
    str[0]=temp/1000+48;  
    str[1]='.';
    str[2]=(temp/100)%10+48;
    str[3]=(temp/10)%10+48;
    str[4]=temp%10+48;
    str[5]='\0';
    Lcd_WriteStr(5,1,str);
}


//**********************************************************************
//                   MSP430内部看门狗初始化
//**********************************************************************
void Wdt_Init()
{
   WDTCTL = WDTPW + WDTHOLD;       //关闭看门狗
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



/*********************************************************
时间日期:2012.7.28
函数名称:void Start_ADcom()
函数功能:ADS1115可以开始读数据
*********************************************************/
void Start_ADcom()
{   
    SDA_OUT;     //设置为输出口
    SDAOUT_LOW;
    ADSCK_HIGH;
    Delay(2);
    SDAOUT_HIGH;
    Delay(2);
    SDAOUT_LOW;
    ADSCK_LOW; 
    Delay(2);
}

/*********************************************************
时间日期:2012.7.28
函数名称:void StopADcom()
函数功能:停止ADS1115采集数据
*********************************************************/
void StopADcom()
{
    SDA_OUT;     //设置为输出口
    SDAOUT_LOW;
    Delay(1);
    ADSCK_HIGH;
    Delay(1);
    SDAOUT_HIGH;
    Delay(1);
}

/*********************************************************
时间日期:2012.7.28
函数名称:void Send_Byte(unsigned char )
函数功能:SPI通信发送一个字节
*********************************************************/
void Send_Byte(unsigned char Byte)
{
    unsigned char i;
    SDA_OUT;            //设置为输出口
    for(i = 0; i < 8; i ++)
    {
        if((Byte<<i)&0x80)
        {
            SDAOUT_HIGH;
        }
        else 
        {
            SDAOUT_LOW;
        }
        Delay(1);
        ADSCK_HIGH;
        Delay(1);
        ADSCK_LOW;
        Delay(1);
    }
    Delay(1);//结束
    SDAOUT_HIGH;
    Delay(1);
    ADSCK_HIGH;
    Delay(1);
    ADSCK_LOW;
    Delay(1);
}

/*********************************************************
时间日期:2012.7.28
函数名称:unsigned int Read_ADData()
函数功能:SPI通信发送一个字节
*********************************************************/
unsigned char Read_Byte()
{
    uchar ADData = 0;
    uchar i = 0;
    SDA_IN;                 //IO口切换到输入模式
    for(i = 0; i < 8; i ++)
    {
        ADData = ADData<<1;
        ADSCK_LOW;
        Delay(1);
        ADSCK_HIGH;
        Delay(1);
        if(SDAIN == 1)
        {
            ADData |= 0x01;
        }
        Delay(1);
    }
    SDA_OUT;          //IO口切换到输出模式
    ADSCK_LOW;
    Delay(1);
    SDAOUT_LOW;
    Delay(1);
    ADSCK_HIGH;
    Delay(1);
    ADSCK_LOW;
    Delay(1);
    SDAOUT_HIGH;
    return ADData;
}

/****************************************************
时间日期:2012.7.28
函数名称:Confige1115() 
函数功能:配置AD转换芯片 
****************************************************/
void Confige_AD(unsigned char Channel_Val)
{
    uchar i = 0;
    uchar Channel_Valr = 0;
    switch(Channel_Val)
    {
        case 0: Channel_Valr = 0x81;     //满幅6.144，单次采样
                break;
        case 1: Channel_Valr = 0x85;     //通道1
                break;
        case 2: Channel_Valr = 0x85;     //通道2
                break;
        case 3: Channel_Valr = 0x85;     //通道3
                break;
        default:break;        
    }
    InitData[0] = 0x90;                 //地址+写命令，选择GND，写
    InitData[1] = 0x01;                 //指向配置寄存器
    InitData[2] = Channel_Valr;         //配置高字节
    InitData[3] = 0x83;             //配置低字节 128SPS  传统比较 不使能比较功能
    SDA_OUT;                       //IO口切换到输出模式
    ADSCK_HIGH;                    //SCK 置高
    Start_ADcom();                 //开始写配置指令
    for(i = 0; i < 4 ; i ++)
    {
        Send_Byte(InitData[i]);
        Delay(1);
    }
    StopADcom();
}

/****************************************************
时间日期:2012.7.28
函数名称:void Pointregister()
函数功能:指向转换结果寄存器 
****************************************************/
void Point_ConversionRegister()
{
    unsigned char i = 0;
    InitData[0] = 0x90;      //地址 + 写命令
    InitData[1] = 0x00;      //指向转换结果寄存器
    SDA_OUT;                 //IO口切换到输出模式
    ADSCK_HIGH;
    Start_ADcom();
    for(i = 0; i < 2; i ++)
    {
        Send_Byte(InitData[i]);
        Delay(1);
    }
    StopADcom();
    Delay(1);
}

/****************************************************
时间日期:2012.7.28
函数名称:unsigned int Read1115_Data()
函数功能:读取AD数据
****************************************************/
unsigned int Read_Data()
{
    uchar Result_L = 0;
    uchar Result_H = 0;
    uint  Result = 0;
    InitData[0] = 0x91;     //地址 + 读指令
    ADSCK_HIGH;
    SDA_OUT;                //设置为输出口
    Start_ADcom();
    Delay(1);
    Send_Byte(InitData[0]);
    SDA_IN;
    Delay(1);
    Result_H = Read_Byte();
    Delay(1);
    Result_L = Read_Byte();
    StopADcom();
    Result = Result_H*256 + Result_L;
    return Result; 
}

/*****************************************************
时间日期:2012.7.28
函数名称:unsigned int AD_1115Read_Data(unsigned char)
函数功能:读取AD数据
****************************************************/
unsigned int AD_ReadData(unsigned char Channel_Ad)
{
    uint AD_Result = 0;
    Confige_AD(Channel_Ad);
    Delay(1);
    Point_ConversionRegister();
    Delay(1);
    AD_Result = Read_Data(); 
    return AD_Result;
}

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
  while(1)
  {
  if(count<10)
  {
    rst+=AD_ReadData(0)+2950;//系统误差，调整截距
    Delay(1000);
    count++;
  }
  else
  {
    count=0;
    rst/=10;
    Adc_Str();
    Delay(1000);
    rst=0;
  }
  }
}

