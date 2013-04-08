#include <msp430x14x.h>


#define  uint unsigned int
#define  uchar unsigned char

#define LCD_DataIn    P4DIR=0X00       //���ݿڷ�������Ϊ����
#define LCD_DataOut   P4DIR=0XFF       //���ݿڷ�������Ϊ���
#define LCD2MCU_Data  P4IN
#define MCU2LCD_Data  P4OUT
#define LCD_RS_H      P5OUT|=BIT5      //P5.5
#define LCD_RS_L      P5OUT&=~BIT5     //P5.5
#define LCD_RW_H      P5OUT|=BIT6      //P5.6
#define LCD_RW_L      P5OUT&=~BIT6     //P5.6
#define LCD_EN_H      P5OUT|=BIT7      //P5.7
#define LCD_EN_L      P5OUT&=~BIT7     //P5.7

uint    flag = 0; //�жϱ�־
uint    a = 0;    //��CCR1
uint    cd = 0;   //TAIFG�������

unsigned long t = 0,sum = 0;
unsigned long results[8];


void int_clk();


void Delay(unsigned int t);   //��ʱ��ʱt����


////////////lcd��ʾ��غ���

void RDbf(void) ;  //��æ��־

void Lcd_Reset() ;//lcd��ʼ��

void Lcd_WriteData(unsigned char Data);//д����

void Lcd_WriteCmd(unsigned char CmdCode); //дָ��

void Lcd_Clear(void);//���GDRAM����

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str);//д�ַ���

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //���з��׹����������

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//�������

//��ʾƵ��
void Display(unsigned long f);





//��ʼ��ʱ��
void int_clk() 
{
  uchar i;
  BCSCTL1&=~XT2OFF;                  //��XT����
  BCSCTL2|=SELM_2+SELS;               //�ͣạ̃ˣ��ͣ�����ӣͣạ̃�Ϊ���ͣ��
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
    Lcd_WriteCmd(0x30);        //ѡ�����ָ�
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x0c);        //����ʾ(���αꡢ������)
    Lcd_Clear();               //���Һ��GDRAM�е�����
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x06);        //�����ϵĶ�ȡ��д��ʱ�α��Զ�����
}

/********
��ʱt���� 
********/
void Delay(unsigned int t)
{
  unsigned int i,j;
  for(i=0;i<t;i++)
    for(j=150;j>0;j--) 
      _NOP(); 
}
 

/********** 
���æ��־ 
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
�� LCD д��1�ֽ����� 
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
�� LCD ��д��ָ����� 
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
�� LCD ָ����ʼλ��д��һ���ַ���
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
���Һ��GDRAM�е�����
********************/
void Lcd_Clear(void)
{
    unsigned char i,j,k;
    Lcd_WriteCmd(0x34);        //����չָ�
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
�� LCD �ж���1�ֽ�����
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
�з�����ʾ���ܵĴ�㺯��:(X-ˮƽλַ,Y-��ֱλַ)
************************************************/
void GUI_Point(unsigned char x,unsigned char y,unsigned char color)
{
    unsigned char x_Dyte,x_byte;		//�����е�ַ���ֽ�λ�������ֽ��е���1λ
    unsigned char y_Dyte,y_byte;		//����Ϊ����������(ȡֵΪ0��1)���е�ַ(ȡֵΪ0~31)
    unsigned char GDRAM_hbit,GDRAM_lbit;
    Lcd_WriteCmd(0x36);		                //��չָ������
    x_Dyte=x/16;				//������16���ֽ��е���һ��
    x_byte=x&0x0f;				//�����ڸ��ֽ��е���һλ
    y_Dyte=y/32;				//0Ϊ�ϰ�����1Ϊ�°���
    y_byte=y&0x1f;				//������0~31���е���һ��
    Lcd_WriteCmd(0x80+y_byte);			//�趨�е�ַ(y����)
    Lcd_WriteCmd(0x80+x_Dyte+8*y_Dyte);		//�趨�е�ַ(x����)
    Lcd_ReadData();				//Ԥ��ȡ����
    GDRAM_hbit=Lcd_ReadData();			//��ȡ��ǰ��ʾ��8λ����
    GDRAM_lbit=Lcd_ReadData();			//��ȡ��ǰ��ʾ��8λ����
    Lcd_WriteCmd(0x80+y_byte);			//�趨�е�ַ(y����)
    Lcd_WriteCmd(0x80+x_Dyte+8*y_Dyte);		//�趨�е�ַ(x����)
    if(x_byte<8)				//�ж����ڸ�8λ�������ڵ�8λ
    {
      if(color==1)
      {
        Lcd_WriteData(GDRAM_hbit|(0x01<<(7-x_byte))); //��λGDRAM����8λ��������Ӧ�ĵ�
      }
      else
      Lcd_WriteData(GDRAM_hbit&(~(0x01<<(7-x_byte))));//���GDRAM����8λ��������Ӧ�ĵ�	
      Lcd_WriteData(GDRAM_lbit);		        //��ʾGDRAM����8λ����
    }
    else
    {
      Lcd_WriteData(GDRAM_hbit);
      if(color==1)
        Lcd_WriteData(GDRAM_lbit|(0x01<<(15-x_byte)));//��λGDRAM����8λ��������Ӧ�ĵ�
      else
        Lcd_WriteData(GDRAM_lbit&(~(0x01<<(15-x_byte))));//���GDRAM����8λ��������Ӧ�ĵ�	
    }
    Lcd_WriteCmd(0x30);			        //�ָ�������ָ�
}

/***************************
(���������)����λ�ô�㺯��
***************************/
void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color)
{
    GUI_Point(x,63-y,color);
}


void Display(unsigned long sum)
{ 
   unsigned long f;
   sum>>=3;            //����8����ƽ��
   f=8*1000000/sum;    //8M/sum ,��Ƶ��
   
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

   /*�������г���ر����е�IO��*/
    P1DIR = 0XFF;P1OUT = 0XFF;                 // P1DIR = 0XF0;P1OUT = 0X0F;
    P2DIR = 0XFF;P2OUT = 0XFF;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0X7F;P6OUT = 0xFF; 
    //P6IN  = 0X00; 
    
   WDTCTL=WDTPW+WDTHOLD;                     //�ع�
   
   int_clk();                                //��ʼ��ʱ��
  
   Lcd_Reset();
   /* �ַ���ʾ
   Lcd_WriteStr(0,1,"AD12");              //���ĺ��ַ��ֿ�������ʾ��������
   Lcd_WriteStr(2,1,"��ѹΪ");
   */  
   
   
    TACTL|=TASSEL_2+TACLR+TAIE+ MC_2;//ѡ��MCLK����� TAR��ʱ�ӷ�Ƶ������ģʽ�����ã�����ʱ���ж�;ѡ������ģʽ
    
   
    TACCTL1|=CAP+CM_1+SCS+CCIE+CCIS_0;    //ѡ�񲶻�ģʽ������Ϊ�������ز���ѡ������ԴΪCCI1A P1.1��ͬ�������ж�����
    
    //TACTL|=TASSEL1+MC1;
    
    P2SEL|=0x04;
    P2DIR|= BIT2;                    //P2.0��ACLK�����          
    //P2OUT&=~0X01;                  //P2.0�øߵ�λ
    
    P1SEL=0x04;                     //P1.1ѡ����Χ����ģʽ����Ϊ��׽�������
    P1DIR&=~BIT2;                      //P1.1,P1.2��������
    
    P3SEL=0x00;
    P3DIR=0xff;
    P3OUT&=~0x30;
    _EINT();                             //�����ж�
   
    
        while(flag)
        {
          
        flag=0;
        
        for(uint nu=0;nu<8;nu++)//��¼8��
        {
          sum+=results[nu];   
        }        
        
        Display(sum);          //��ʾֵ
        
        Delay(200);
        
        sum=0;
        
        TACTL|=TACLR+TAIE;    //��� TAR��ʱ�ӷ�Ƶ������ģʽ�����ã�����ʱ���ж�   
        
        TACCTL1|=CCIE;        //�����жϣ�TACCR1��
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
      t=cd*(65535+1)+CCR1-a; //CCR1��CCIFG������ÿ��ʵ�ʼ�����ֵֵ
          a=CCR1;                                   
          cd=0; 
          results[index]=t; //�洢���
          index++;
           if(index==8)       //�����洢����
            { 
              index=0;       //��������
              TACCTL1&=~CCIE;  //���ж�CCR1
              TACTL&=~TAIE;  //�ض�ʱ���ж�
              flag=1;        //�жϱ�־��λ����������ʾ
            }         
    }
          break;
  case 4: break;              //CCR2 �� CCIFG
  case 10:cd++;
          P3OUT^=0x0ff;  
          break;        //TAIFG�������������
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