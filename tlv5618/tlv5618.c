//===========================================================
//MSP430F149 ADC12 ��ͨ����·���� P60�ɼ�tlv5618���������p64ͨ������ת��
//      ref 2.048 ����ref3032оƬ �Ƽ�ֵAGND - VDD-1.5V�����ֵΪ2*ref*Dignum/4095
//     tlv5618��A B 2ͨ��������ֱ�� p60 p64   
//==========================================================

#include <msp430x14x.h>

#define  uint  unsigned int
#define  uchar unsigned char

#define Channal_A     1    //ͨ��A
#define Channal_B     2    //ͨ��B
#define Channal_AB    3    //ͨ��A&B


#define DIN_H    P6OUT|=BIT3      //p63DIN O
#define DIN_L    P6OUT&=~BIT3     //p63DIN O
#define SCLK_H   P6OUT|=BIT2      //p62SCLK O   
#define SCLK_L   P6OUT&=~BIT2     //p62SCLK O
#define CS_H     P6OUT|=BIT1      //p61CS   O
#define CS_L     P6OUT&=~BIT1     //p61CS   O

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

 

//static unsigned    char  adc_flag = 0 ;  
static unsigned    char  count = 0 ; //�����������
static unsigned    int   rst=0;


void int_clk();

void int_adc();

void Delay(unsigned int t);   //��ʱ��ʱt����


////////////lcd��ʾ��غ���

void RDbf(void) ;  //��æ��־

void Lcd_Reset() ;//lcd��ʼ��

void Lcd_WriteData(unsigned char Data);//д����

void Lcd_WriteCmd(unsigned char CmdCode); //дָ��

void Lcd_Clear(void);//���GDRAM����

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str);//д�ַ���

void DA_Conver(uint Dignum);
void Dac_Write(uint Data_A,uint Data_B,uchar Channal,uchar Model);





void Dac_Init(void)
{
  P6SEL=0x00;
  P6DIR=0x6E;     //p64,p60�ֱ��OUTB��OUTA������Ϊ����Ĵ�
  P6OUT=0x00;
}

//================================================================= 
//   void DA_conver(uint Dignum)

void DA_Conver(uint Dignum)
{
uint Dig=0;
uchar i=0;
SCLK_H;
CS_L;                //Ƭѡ��Ч
for(i=0;i<16;i++)   //д��16ΪBit�Ŀ���λ������
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
CS_H;       //Ƭѡ��Ч
}
//================================================================= 
// �������� ��void Write_A_B(uint Data_A,uint Data_B,uchar Channal,bit Model)
// �������� ��ģʽ��ͨ��ѡ�񲢽���DAת�� 
// ��ڲ��� ��Data_A��Aͨ��ת���ĵ�ѹֵ
//            Data_B��Bͨ��ת���ĵ�ѹֵ
//            Channal��ͨ��ѡ����ֵΪChannal_A��Channal_B,��Channal_AB
//            Model���ٶȿ���λ 0��slow mode 1��fast mode
// ���ڲ��� ����
// ˵����     Data_A��Data_B�ķ�ΧΪ��0-0x0fff
//            ���������ֻ��Ҫһ��ͨ��ʱ������һ��ͨ����ֵ�����⣬���ǲ���ȱʡ
//=================================================================
void Dac_Write(uint Data_A,uint Data_B,uchar Channal,uchar Model)
{
uint Temp;
if(Model) 
{
   Temp=0x4000;        //���ٻ���
}
else 
    {
   Temp=0x0000;       //���ٻ���
}
switch(Channal)
{
    case Channal_A:         //Aͨ��
         DA_Conver(Temp|0x8000|(0x0fff&Data_A));
      break; 
    case Channal_B:         //Bͨ��
         DA_Conver(Temp|0x0000|(0x0fff&Data_B));
    break; 
    case Channal_AB:        //A&Bͨ��
         DA_Conver(Temp|0x1000|(0x0fff&Data_B));       
         DA_Conver(Temp|0x8000|(0x0fff&Data_A));
    break;
    default:
         break;
}
}

//��ʼ��ʱ��
void int_clk() 
{uchar i;
  BCSCTL1&=~XT2OFF;                  //��XT����
  BCSCTL2|=SELM1+SELS;               //�ͣạ̃ˣ��ͣ�����ӣͣạ̃�Ϊ���ͣ��
  do
  {
    IFG1 &= ~OFIFG;
    for(i=0;i<100;i++)
      _NOP();
  }
  while((IFG1&OFIFG)!=0);
    IFG1&=~OFIFG;
}



//��ʼ��ADC12����ѯ��ʽ
void int_adc_check()           
{
  P6SEL|=0X01;//�У�����ѡ�񣬣�0ѡ��ڶ����ܣ���0��ѡ��ADͨ��
  //����ENCΪ0���Ӷ��޸�ADC12�Ĵ�����ֵ
  ADC12CTL0 &= ~(ENC);
  
  // �ں˿���, �����ڲ���׼, ѡ��2.5V��׼, ���ò�������ʱ��
  ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
  // ʱ��ԴΪ�ڲ�����, �����ź����Բ�����ʱ��, ת����ַΪADC12MCTL4
  ADC12CTL1 = ADC12SSEL_0 + SHP + CSTARTADD_4;
  // ת��ͨ������
  ADC12MCTL4 = SREF_1 + INCH_0; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:A0
  // ����ת��
  ADC12CTL0 |= ENC + ADC12SC;    // ת��ʹ�ܿ�ʼת��
  while((ADC12IFG & 0x0010) == 0); // �����ѯ�жϱ�־, �ȴ�ת������
  count++;
  rst=rst+ADC12MEM4;
  Delay(3000);           //��ʱ3��
  if(count==10)
  {
   //rst=ADC12MEM4;
   //rst=(rst*2.5/4096);
    rst=rst/10;
    uchar str[6];
    long int temp=rst;
   
    str[0]=rst/1000+48;      //ǧλ
    str[1]=(rst/100)%10+48;  //��λ
    str[2]=(rst/10)%10+48;   //ʮλ
    str[3]=rst%10+48;        //��λ
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
    Delay(3000);             //��ʱ3��
  }
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
 
/******************* 
�� LCD д��1�ֽ����� 
*******************/ 


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
/********* 
LCD ��ʼ�� 
*********/
void Lcd_Reset() 
{ 
    Lcd_WriteCmd(0x30);        //ѡ�����ָ�
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x0c);        //����ʾ(���αꡢ������)
    Lcd_Clear();               //���Һ��GDRAM�е�����
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x06);        //�����ϵĶ�ȡ��д��ʱ�α��Զ�����
}




int main(void)
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
   Lcd_WriteStr(0,1,"AD12");              //���ĺ��ַ��ֿ�������ʾ��������
   Lcd_WriteStr(2,1,"��ѹΪ");    
   
   Dac_Init();
  
  //Write_A_B(0x0fff,0x07ff,Channal_AB,1);   //����ABͨ��������ʱ��������������
                                             //ʵ�ⲻ������ֿ�����
   _EINT();                                  //ʹ���ж�
   while(1)
   {
   Dac_Write(0x0fff,0x0000,Channal_A,1);     //����Aͨ����ģ�����Ϊ2*REF*Dignum/0x0fff
   Dac_Write(0x0000,0x07ff,Channal_B,1);    //����Bͨ��
   int_adc_check();                        //��ʼ��adc
   }
}
