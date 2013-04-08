#include <msp430x14x.h>

//****************************************************************************
//ads11114ģ��   ����I2CЭ��
//P4.0  Ready ��Ƭ���� ;  P4.1 SCL ��Ƭ��д ; P4.2   SDA ��Ƭ����
// ת����� �ؾ������main  б���� Adc_Str������
//SPS ѡ��128b/s ��100�� PGAѡ��000�� 2/3 FS6.114  ��Configue_AD������
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

uchar key_new=0;   //�洢��ֵ
uchar key_old=0;

static unsigned    char  count = 0 ;//�����������
static unsigned    long   rst=0;     //��Ų���ֵ


////�˿ڶ���
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



void Wdt_Init();//�رտ��Ź�

void Delay(unsigned int t); //��ʱ��ʱt����

void Lcd_Reset() ;//lcd��ʼ��

void RDbf(void) ;  //���æ��־

void Lcd_WriteData(unsigned char Data);//д����

void Lcd_WriteCmd(unsigned char CmdCode); //дָ��

unsigned char Lcd_ReadData(void);//�����ݣ�һ�ֽڣ�

void Lcd_Clear(void);//���GDRAM����


//////////////////////////��ͼ��������
void Lcd_WritePic(const unsigned char * adder );  //ˮƽɨ�裬��������ʾ

void Lcd_WriteStr(unsigned char x,unsigned char y,unsigned char *Str) ;//�ƶ�λ��д���ַ���

void GUI_Point(unsigned char x,unsigned char y,unsigned char color);   //���з��׹����������

void lcd_set_dot(unsigned char x,unsigned char y,unsigned char color);//�������

void gui_hline(unsigned char x0, unsigned char x1, unsigned char y);//��ˮƽ��

void gui_rline(unsigned char x, unsigned char y0, unsigned char y1);//����ֱ��

void gui_line(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1);//�������㻭��

////////ads11114
//  ����˵��   P40  
//
//
void Adc_Str()
{
  
   Lcd_WriteStr(0,1,"AD16");              //���ĺ��ַ��ֿ�������ʾ��������2^16=65536 0x8000-ox7FFF
   Lcd_WriteStr(2,1,"��ѹΪ");            //���� 0-32767,���� 65535-32768
   Lcd_WriteStr(0,2,"��Ӧ������");
   uchar str[6];
   
    
   if(rst>65535)
     rst=65536;
     
    long int temp=rst;
    
    str[0]=rst/10000+48;
    str[1]=(rst/1000)%10+48; //ǧλ
    str[2]=(rst/100)%10+48;  //��λ
    str[3]=(rst/10)%10+48;   //ʮλ
    str[4]=rst%10+48;        //��λ
    str[5]='\0';
    Lcd_WriteStr(5,2,str);
   
    if(temp>32767)
    temp=65536-temp;
    temp=(temp*6300)/32768;  //ϵ������׼Ϊ������ѹ���˴�ѡΪ6.114��б�ʸ��ݵ���
    str[0]=temp/1000+48;  
    str[1]='.';
    str[2]=(temp/100)%10+48;
    str[3]=(temp/10)%10+48;
    str[4]=temp%10+48;
    str[5]='\0';
    Lcd_WriteStr(5,1,str);
}


//**********************************************************************
//                   MSP430�ڲ����Ź���ʼ��
//**********************************************************************
void Wdt_Init()
{
   WDTCTL = WDTPW + WDTHOLD;       //�رտ��Ź�
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



/*********************************************************
ʱ������:2012.7.28
��������:void Start_ADcom()
��������:ADS1115���Կ�ʼ������
*********************************************************/
void Start_ADcom()
{   
    SDA_OUT;     //����Ϊ�����
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
ʱ������:2012.7.28
��������:void StopADcom()
��������:ֹͣADS1115�ɼ�����
*********************************************************/
void StopADcom()
{
    SDA_OUT;     //����Ϊ�����
    SDAOUT_LOW;
    Delay(1);
    ADSCK_HIGH;
    Delay(1);
    SDAOUT_HIGH;
    Delay(1);
}

/*********************************************************
ʱ������:2012.7.28
��������:void Send_Byte(unsigned char )
��������:SPIͨ�ŷ���һ���ֽ�
*********************************************************/
void Send_Byte(unsigned char Byte)
{
    unsigned char i;
    SDA_OUT;            //����Ϊ�����
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
    Delay(1);//����
    SDAOUT_HIGH;
    Delay(1);
    ADSCK_HIGH;
    Delay(1);
    ADSCK_LOW;
    Delay(1);
}

/*********************************************************
ʱ������:2012.7.28
��������:unsigned int Read_ADData()
��������:SPIͨ�ŷ���һ���ֽ�
*********************************************************/
unsigned char Read_Byte()
{
    uchar ADData = 0;
    uchar i = 0;
    SDA_IN;                 //IO���л�������ģʽ
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
    SDA_OUT;          //IO���л������ģʽ
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
ʱ������:2012.7.28
��������:Confige1115() 
��������:����ADת��оƬ 
****************************************************/
void Confige_AD(unsigned char Channel_Val)
{
    uchar i = 0;
    uchar Channel_Valr = 0;
    switch(Channel_Val)
    {
        case 0: Channel_Valr = 0x81;     //����6.144�����β���
                break;
        case 1: Channel_Valr = 0x85;     //ͨ��1
                break;
        case 2: Channel_Valr = 0x85;     //ͨ��2
                break;
        case 3: Channel_Valr = 0x85;     //ͨ��3
                break;
        default:break;        
    }
    InitData[0] = 0x90;                 //��ַ+д���ѡ��GND��д
    InitData[1] = 0x01;                 //ָ�����üĴ���
    InitData[2] = Channel_Valr;         //���ø��ֽ�
    InitData[3] = 0x83;             //���õ��ֽ� 128SPS  ��ͳ�Ƚ� ��ʹ�ܱȽϹ���
    SDA_OUT;                       //IO���л������ģʽ
    ADSCK_HIGH;                    //SCK �ø�
    Start_ADcom();                 //��ʼд����ָ��
    for(i = 0; i < 4 ; i ++)
    {
        Send_Byte(InitData[i]);
        Delay(1);
    }
    StopADcom();
}

/****************************************************
ʱ������:2012.7.28
��������:void Pointregister()
��������:ָ��ת������Ĵ��� 
****************************************************/
void Point_ConversionRegister()
{
    unsigned char i = 0;
    InitData[0] = 0x90;      //��ַ + д����
    InitData[1] = 0x00;      //ָ��ת������Ĵ���
    SDA_OUT;                 //IO���л������ģʽ
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
ʱ������:2012.7.28
��������:unsigned int Read1115_Data()
��������:��ȡAD����
****************************************************/
unsigned int Read_Data()
{
    uchar Result_L = 0;
    uchar Result_H = 0;
    uint  Result = 0;
    InitData[0] = 0x91;     //��ַ + ��ָ��
    ADSCK_HIGH;
    SDA_OUT;                //����Ϊ�����
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
ʱ������:2012.7.28
��������:unsigned int AD_1115Read_Data(unsigned char)
��������:��ȡAD����
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
    rst+=AD_ReadData(0)+2950;//ϵͳ�������ؾ�
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

