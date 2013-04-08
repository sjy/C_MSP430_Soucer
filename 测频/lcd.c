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

void Delay(unsigned int t);   		  //延时延时t毫秒

void Lcd_Reset() ;                        //lcd初始化

void RDbf(void) ;                         //检测忙标志

void Lcd_WriteData(unsigned char Data);   //写数据

void Lcd_WriteCmd(unsigned char CmdCode); //写指令

unsigned char Lcd_ReadData(void);         //读数据（一字节）

void Lcd_Clear(void);                     //清楚GDRAM内容


//////////////////////////画图函数
void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) ;//制定位置写入字符串

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //具有反白功能任意点打点

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//任意点打点

void gui_hline(unsigned char x0, unsigned char x1, unsigned char y);//画水平线

void gui_rline(unsigned char x, unsigned char y0, unsigned char y1);//画垂直线

void gui_line(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1);//任意两点画线

void gui_linewith(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char width);//画制定宽度的线

void gui_rectangle(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1);//画矩形框

void gui_rectangle_fill(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1);//画填充矩形

void gui_circle(unsigned char x0,unsigned char y0,unsigned char r);//画圆













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
    unsigned char bak;                          //用于对两个数互换的中间变量，使x1为大值
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

/****************************************************
任意两点间画直线,x0、y0为起始点坐标，x1、y1为终点坐标
****************************************************/
void gui_line(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1)
{
    signed char dx;                                    //直线x轴差值
    signed char dy;                                    //直线y轴差值
    signed char dx_sym;                                //x轴增长方向,为-1时减值方向,为1时增值方向
    signed char dy_sym;                                //y轴增长方向,为-1时减值方向,为1时增值方向
    signed char di;                                    //决策变量
    if(x0==x1)                                  //判断是否为垂直线
    {
      gui_rline(x0,y0,y1);
      return;
    }
    if(y0==y1)                                  //判断是否为水平线
    {
      gui_hline(x0,x1,y0);
      return;
    }
    dx=x1-x0;                                   //求取两点之间的差值
    dy=y1-y0;
    if(dx>0)                                    //判断x轴方向
      dx_sym=1;
    else
      dx_sym=-1;
    if(dy>0)                                    //判断y轴方向
    dy_sym=1;
    else
      dy_sym=-1;
    dx=dx_sym*dx;                               //将dx、dy取绝对值
    dy=dy_sym*dy;
    /***使用bresenham法进行画直线***/
    if(dx>=dy)                                  //对于dx>=dy，使用x轴为基准
    {
      di=dy-dx;
      while(x0!=x1)
      {
        lcd_set_dot(x0,y0,1);
        x0+=dx_sym;
        if(di<0)
          di+=dy;                               //计算出下一步的决策值
        else
        {
          di+=dy-dx;
          y0+=dy_sym;
        }
      }
      lcd_set_dot(x0,y0,1);                     //显示最后一点
    }
    else                                        //对于dx<dy使用y轴为基准
    {
      di=dx-dy;
      while(y0!=y1)
      {
        lcd_set_dot(x0,y0,1);
        y0+=dy_sym;
        if(di<0)
          di+=dx;
        else
        {
          di+=dx-dy;
          x0+=dx_sym;
        }
      }
      lcd_set_dot(x0,y0,1);                     //显示最后一点
    }
}

/******************************************************************************
画指定宽度的任意两点之间的直线,x0、y0为起始点坐标，x1、y1为终点坐标，width为线宽
******************************************************************************/
void gui_linewith(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char width)
{
    unsigned char i,bak;
    if(x0>x1)
    {
      bak=x1;
      x1=x0;
      x0=bak;
    }
    if(y0>y1)
    {
      bak=y1;
      y1=y0;
      y0=bak;
    }
    if(x1-x0>y1-y0)
      for(i=0;i<width;i++)
        gui_line(x0,y0+i,x1,y1+i);
    else
      for(i=0;i<width;i++)
        gui_line(x0+i,y0,x1+i,y1);
    return;
}

/************************************************************
画矩形函数,x0、y0为矩形左上角坐标值，x1、y1为矩形右下角坐标值
************************************************************/
void gui_rectangle(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1)
{
    gui_hline(x0,x1,y0);
    gui_rline(x0,y0,y1);
    gui_rline(x1,y0,y1);
    gui_hline(x0,x1,y1);
}

/****************************************************************
画填充矩形函数,x0、y0为矩形左上角坐标值，x1、y1为矩形右下角坐标值
****************************************************************/
void gui_rectangle_fill(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1)
{
    unsigned char bak;
    if(y0>y1)
    {
      bak=y1;
      y1=y0;
      y0=bak;
    }
    gui_linewith(x0,y0,x1,y0,y1-y0);
}

/*************************************
画圆函数,x0、y0为圆心坐标，r为圆的半径
*************************************/
void gui_circle(unsigned char x0,unsigned char y0,unsigned char r)
{
    signed char a,b;
    signed char di;
    if(r>31||r==0)//圆大于液晶屏或者没半径则返回
      return;
    a=0;
    b=r;
    di=3-2*r;//判断下个点位置的标志
    while(a<=b)
    {
      lcd_set_dot(x0-b,y0-a,1);//3
      lcd_set_dot(x0+b,y0-a,1);//0
      lcd_set_dot(x0-a,y0+b,1);//1
      lcd_set_dot(x0-b,y0-a,1);//7
      lcd_set_dot(x0-a,y0-b,1);//2
      lcd_set_dot(x0+b,y0+a,1);//4
      lcd_set_dot(x0+a,y0-b,1);//5
      lcd_set_dot(x0+a,y0+b,1);//6
      lcd_set_dot(x0-b,y0+a,1);
      a++;
      /***使用bresenham算法画圆***/
      if(di<0)
      di+=4*a+6;
      else
      {
        di+=10+4*(a-b);
        b--;
      }
      lcd_set_dot(x0+a,y0+b,1);
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
