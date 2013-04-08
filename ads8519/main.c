#include  <msp430x14x.h>
//**********************************************************************
//    ads 8519 �������� spiЭ��
//    P6.0 ��Ƭ����æ��־���ͣ�����ת�� ��P6.1 ��Ƭ��д RC ѡ��λ������ѡ��װ��
//    P6.2 ��Ƭ���� 16λ��������        ��P6.3 ��Ƭ��������ʱ��
//**********************************************************************

#define  uchar  unsigned char
#define  uint   unsigned int


#define CTL_RC1     P6OUT |=  BIT1//ѡ���ȡ����
#define CTL_RC0     P6OUT &= ~BIT1 //ѡ��ת������
#define BUSY_PIN    P6IN&0x01   //æ��־λ
#define DATA_PIN    P6IN&0X04   //������λ
#define DATACLK_PIN P6IN&0x08   //����ʱ��λ;�ڲ�ʱ��


////��ʾ�˿ڶ���
#define LCD_DataIn    P4DIR=0X00       //���ݿڷ�������Ϊ����
#define LCD_DataOut   P4DIR=0XFF       //���ݿڷ�������Ϊ���
#define LCD2MCU_Data  P4IN             //��Һ��������
#define MCU2LCD_Data  P4OUT            //��Һ��д����

////ָ���
#define LCD_RS_H      P5OUT|=BIT5      //P5.5
#define LCD_RS_L      P5OUT&=~BIT5     //P5.5
#define LCD_RW_H      P5OUT|=BIT6      //P5.6
#define LCD_RW_L      P5OUT&=~BIT6     //P5.6
#define LCD_EN_H      P5OUT|=BIT7      //P5.7
#define LCD_EN_L      P5OUT&=~BIT7     //P5.7


static uchar  count = 0 ;//�����������
static unsigned long   rst=0;     //��Ų���ֵ



void Wdt_Init();//�رտ��Ź�

void Delay(unsigned int t); //��ʱ��ʱt����

void Lcd_Reset() ;//lcd��ʼ��

void RDbf(void) ;  //���æ��־

void Lcd_WriteData(unsigned char Data);//д����

void Lcd_WriteCmd(unsigned char CmdCode); //дָ��

uchar Lcd_ReadData(void);//�����ݣ�һ�ֽڣ�

void Lcd_Clear(void);//���GDRAM����

uint Ads_Convert();

void Ads_Str();



//////////////////////////��ͼ��������
void Lcd_WritePic(const unsigned char * adder );  //ˮƽɨ�裬��������ʾ

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) ;//�ƶ�λ��д���ַ���

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //���з��׹����������

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//�������






//**********************************************************************
//                   MSP430�ڲ����Ź���ʼ��
//**********************************************************************
void Wdt_Init()
{
   WDTCTL = WDTPW + WDTHOLD;       //�رտ��Ź�
}

//**********************************************************************
//                   MSP430ʱ�ӳ�ʼ��
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
//                         ��ʱt���� 
//**********************************************************************
void Delay(unsigned int t)
{
  unsigned int i,j;
  for(i=0;i<t;i++)
    for(j=150;j>0;j--) 
      _NOP(); 
}
//**********************************************************************
//                        ���æ��־ 
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
//                   �� LCD д��1�ֽ����� 
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
//                  �� LCD ��д��ָ����� 
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
//                    �� LCD �ж���1�ֽ�����
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
//              �� LCD ָ����ʼλ��д��һ���ַ���
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
//                    ���Һ��GDRAM�е�����
//**********************************************************************
void Lcd_Clear(void)
{
    unsigned char i,j,k;
    Lcd_WriteCmd(0x34);         //����չָ�
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
//         �з�����ʾ���ܵĴ�㺯��:(X-ˮƽλַ,Y-��ֱλַ)
//**********************************************************************
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
        Lcd_WriteData(GDRAM_hbit|(0x01<<(7-x_byte)));//��λGDRAM����8λ��������Ӧ�ĵ�
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



//**********************************************************************
//                (���������)����λ�ô�㺯��
//**********************************************************************
void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color)
{
    GUI_Point(x,63-y,color);
}

//**********************************************************************
//                         LCD ��ʼ�� 
//**********************************************************************
void Lcd_Reset() 
{ 
    Lcd_WriteCmd(0x30);        //ѡ�����ָ�
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x0c);        //����ʾ(���αꡢ������)
    Lcd_Clear();               //���Һ��GDRAM�е�����
    Lcd_WriteCmd(0x01);        //�����ʾ�������趨��ַָ��Ϊ00H
    Lcd_WriteCmd(0x06);        //�����ϵĶ�ȡ��д��ʱ�α��Զ�����
}



//**********************************************************************
//
//**********************************************************************
uint Ads_Convert()
{
  uint temp=0;        //�޷���int 2�ֽ� 0x0000-0xffff;
  uchar i;
  
  
  CTL_RC0;          //RC ������
  CTL_RC1;
  while(BUSY_PIN);  //Busy Ϊ�ߣ��ȴ���ֱ���õͣ�ת����ʼ��
  for(i = 0;i<16;i++)// 16bit
  {
    while(DATACLK_PIN);//  dataclockΪ��ʱ�ȴ�
    if(DATA_PIN)      //������λ�ɼ�������ת��Ϊ10����
    {
      temp++;
      temp = temp*2;     //ÿ�ξ�������һλ�����յ����Ǹ�λ����
    }
    else 
      temp =temp * 2;
    while(!DATACLK_PIN);//�ȴ�dataclock�õͣ���ʼ��һ��ת��
  }
     
  return temp;  //�����Դ�ת����ʮ��������ֵ
  
}

//**********************************************************************
//   ���ַ���ʽ��ʾ���Ե�ADS8519ֵ
//**********************************************************************
void Ads_Str()
{
  
   Lcd_WriteStr(0,1,"AD16");              //���ĺ��ַ��ֿ�������ʾ��������2^16=65536 0x8000-ox7FFF
   Lcd_WriteStr(2,1,"��ѹΪ");            //���� 0-32767,���� 65535-32768
   Lcd_WriteStr(0,2,"��Ӧ������");
   uchar str[6];
   
   // if(rst>65535)
   // rst=65536;
     
    long int temp=rst;
    
    str[0]=rst/10000+48;
    str[1]=(rst/1000)%10+48; //ǧλ
    str[2]=(rst/100)%10+48;  //��λ
    str[3]=(rst/10)%10+48;   //ʮλ
    str[4]=rst%10+48;        //��λ
    str[5]='\0';
    Lcd_WriteStr(5,2,str);
   
 // if(temp>32767)
 // temp=65536-temp;
    temp=(temp*10000)/32768;  //ϵ������׼Ϊ������ѹ���˴�ѡΪ10000��б�ʸ��ݵ���
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
    Ads_Convert();//ϵͳ�������ؾ�
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