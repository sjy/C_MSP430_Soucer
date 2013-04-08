/*
MSP430F149 ADC12 ��ͨ����·���� 
P67Ϊ�ɼ���
*/

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

 

//static unsigned    char  adc_flag = 0 ;  
static unsigned    char  count = 0 ; //�����������

static unsigned    int   rst0 =0;//��Ų����� 66
static unsigned    int   rst1 =0;//���   67
static unsigned    int   rst =0;//���   67
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

void gui_hline(unsigned char x0, unsigned char x1, unsigned char y);//��ˮƽ��

void gui_rline(unsigned char x, unsigned char y0, unsigned char y1);//����ֱ��

void Adc_Check(); 

void Adc_Figure();

void Adc_Str(); 

//��ʼ��ʱ��
void int_clk() 
{
  uchar i;
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


// ��ʼ��ADC12����ѯ��ʽ
void Adc_Check()           
{
  /*
  P6SEL|=0X01;  //�У�����ѡ�񣬣𣶣�ѡ��ڶ����ܣ�������ѡ��ADͨ��
                //����ENCΪ0���Ӷ��޸�ADC12�Ĵ�����ֵ
  
  ADC12CTL0 &= ~(ENC); 
  ADC12CTL0|=ADC12ON+SHT0_2+REF2_5V+REFON;  //���ģã�����Դ�����򿪲�ѡ���ڲ��ο���ѹΪ2.5v����������ʱ��Ϊ16��ADC12CLK
  
  //ADC12CTL1=SHP+CONSEQ_2;                 //������������ѡ�����ʱ���·�������źţ���·�ظ�ת��
  ADC12CTL1|=ADC12SSEL1+ADC12SSEL1;
  ADC12MCTL0=0X10;   //����
  ADC12IE|= 0X01;    //ʹ��ת���ж�p67
  ADC12CTL0|=ENC;    //ADCת��ʹ��
  */
  
  P6SEL|=0X80;  //�У�����ѡ�񣬣𣶣�ѡ��ڶ����ܣ�������ѡ��ADͨ��
  //����ENCΪ0���Ӷ��޸�ADC12�Ĵ�����ֵ
  ADC12CTL0 &= ~(ENC);  
  // �ں˿���, �����ڲ���׼, ѡ��2.5V��׼, ���ò�������ʱ��
  ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
  // ʱ��ԴΪ�ڲ�����, �����ź����Բ�����ʱ��, ת����ַΪADC12MCTL4
  ADC12CTL1 = ADC12SSEL_0 + SHP + CSTARTADD_4;
  // ת��ͨ������
  ADC12MCTL4 = SREF_1 + INCH_7; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:A0
  // ����ת��
  ADC12CTL0 |= ENC + ADC12SC;    // ת��ʹ�ܿ�ʼת��
  
  while((ADC12IFG & 0x0010) == 0); // �����ѯ�жϱ�־, �ȴ�ת������
  count++;
  rst=rst+ADC12MEM4;
  Delay(1000);            
}



void Adc_Str()
{
   Lcd_WriteStr(0,0,"AD12");              //���ĺ��ַ��ֿ�������ʾ��������
   Lcd_WriteStr(2,0,"��ѹΪ");
   Lcd_WriteStr(0,2,"AD12");              //���ĺ��ַ��ֿ�������ʾ��������
   Lcd_WriteStr(2,2,"��ѹΪ");
   
  if(count==10)
  {
   //rst=ADC12MEM4;
   //rst=(rst*2.5/4096);
    //67 10������
    long int rst = rst0;
    
    rst=rst/10;
    uchar str[6];
    long int temp = rst;
    
    str[0]=rst/1000+48;      //ǧλ
    str[1]=(rst/100)%10+48;  //��λ
    str[2]=(rst/10)%10+48;   //ʮλ
    str[3]=rst%10+48;        //��λ
    str[4]='\0';
    Lcd_WriteStr(5,1,str);
    Lcd_WriteStr(0,1,"��Ӧ������");
    
    temp=(temp*250)/4095;
    str[0]=temp/100+48;  
    str[1]='.';
    str[2]=(temp/10)%10+48;
    str[3]=(temp%10)+48;
    str[4]='V';
    str[5]='\0';
    Lcd_WriteStr(5,0,str);
    
    
    //67 10��
    rst = rst1;
    rst=rst/10;
    temp = rst;
    
    str[0]=rst/1000+48;      //ǧλ
    str[1]=(rst/100)%10+48;  //��λ
    str[2]=(rst/10)%10+48;   //ʮλ
    str[3]=rst%10+48;        //��λ
    str[4]='\0';
    Lcd_WriteStr(5,3,str);
    Lcd_WriteStr(0,3,"��Ӧ������");
    
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
    Delay(3000);             //��ʱ3��
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
/*�ж�ʽ
#pragma vector=ADC_VECTOR

__interrupt void AD12ISR( void)        //adc��ת���ж�
{
  //while((ADC12CTL1&0x01)==1);
 // uint results[10];
  adc_flag = 1 ;                       //ת����־��1
  rst=ADC12MEM0;          //adc12mem0 �������  ������Ӧ����Ԫ�أ�
//  rst+=results[count];
//  count++;                           //������ŵ���

}
*/

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

/*******************************************************
��ˮƽ�ߺ���,x0��x1Ϊ��ʼ����յ��ˮƽ���꣬yΪ��ֱ����
*******************************************************/
void gui_hline(unsigned char x0, unsigned char x1, unsigned char y)
{
    unsigned char bak;            //���ڶ��������������м������ʹx1Ϊ��ֵ
    if(x0>x1)
    {
      bak=x1;
      x1=x0;
      x0=bak;
    }
    do
    {
      lcd_set_dot(x0,y,1);                      //�����������ʾ
      x0++;	
    }while(x1>x0);
}

/*******************************************************
����ֱ�ߺ���,xΪˮƽ���꣬y0��y1Ϊ��ʼ����յ�Ĵ�ֱ����
*******************************************************/
void gui_rline(unsigned char x, unsigned char y0, unsigned char y1)
{
    unsigned char bak;                          //���ڶ��������������м������ʹy1Ϊ��ֵ
    if(y0>y1)
    {
      bak=y1;
      y1=y0;
      y0=bak;
    }
    do
    {
      lcd_set_dot(x,y0,1);                      //���ϵ��������ʾ
      y0++;	
    }while(y1>=y0);
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

//-----------------------------------------------------------------------
// ADC12����ͨ������ת��
void Adc_2Int(void)
{
// ADC12���ƼĴ�������
ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2;
// CONSEQ_1��ʾ��ǰģʽΪ����ͨ������ת��, ��ʼ��ַΪADC12MCTL4, ������ַADC12MCTL6
ADC12CTL1 = ADC12SSEL_0 + SHP + CONSEQ_1 + CSTARTADD_4;
// ת��ͨ������
ADC12MCTL4 = SREF_1 + INCH_7; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:A7
ADC12MCTL5 = SREF_1 + INCH_6+EOS; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:A6
ADC12MCTL6 = SREF_1 + INCH_10 + EOS; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:Ƭ���¶ȴ�����
// �ж�����
ADC12IE = 0x0040;
_EINT();
// ����ת��
ADC12CTL0 |= ENC + ADC12SC; // ת��ʹ�ܿ�ʼת��
__low_power_mode_0(); // ����͹���ģʽ, �ȴ�ת������

}
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// ADC12��ͨ������ת��(�жϲ�ѯʽ)
void Adc_Int(void)
{
P6SEL|=0XC0;  //�У�����ѡ�񣬣𣶣�ѡ��ڶ����ܣ�������ѡ��ADͨ��
//����ENCΪ0���Ӷ��޸�ADC12�Ĵ�����ֵ
ADC12CTL0 &= ~(ENC);  
// ADC12���ƼĴ�������
ADC12CTL0 = ADC12ON + REFON + REF2_5V + SHT0_2 ;
ADC12CTL1 = ADC12SSEL_0 + SHP + CONSEQ_2+ CSTARTADD_4;
// ת��ͨ������
ADC12MCTL4 = SREF_1 + INCH_7; // �ο���ѹ:V+=Vref+,V-=AVss ADCͨ��:A7
ADC12MCTL5 = SREF_1 + INCH_6;
// �ж�����
ADC12IE = 0x00ff;
_EINT();
// ����ת��
ADC12CTL0 |= ENC + ADC12SC; // ת��ʹ�ܿ�ʼת��
__low_power_mode_0(); // ����͹���ģʽ, �ȴ�ת������
}

// ADC12�ж�����
#pragma vector = ADC_VECTOR
__interrupt void ADC12_IRQ(void)
{
  rst0 += ADC12MEM4;          //adc12mem4 �������  p67  ������Ӧ����Ԫ�أ�
  rst1 += ADC12MEM5;          //p66
  count++;                   //������ŵ���
  __low_power_mode_off_on_exit(); //�жϽ���ʱ, �˳��͹���ģʽ
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

   /*�������г���ر����е�IO��*/
    P1DIR = 0XFF;P1OUT = 0XFF;                 // P1DIR = 0XF0;P1OUT = 0X0F;
    P2DIR = 0XFF;P2OUT = 0X00;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0X7F;P6OUT = 0xFF; 
    //P6IN  = 0X00; 
    
   WDTCTL=WDTPW+WDTHOLD;                     //�ع�
   
   int_clk();                                //��ʼ��ʱ��
  
   Lcd_Reset();
   
   _EINT();                                  //ʹ���ж�
   
   while(1)
   {
    Adc_Int( );
    Adc_Str();
   }
  
}
