#include<msp430x16x.h>
#include"lcd.c"


unsigned char flag;

void Init_System(void)
{
    unsigned char dt;
    BCSCTL1 &= ~XT2OFF;   //XT2 = HF XTAL
    do {
       IFG1 &= ~OFIFG;   //Clear OSCFault flag
       for (dt=0xFF;dt>0;dt--);  //Time for flag to set
      }while((IFG1&OFIFG));         //OSCFault flag still set?
    BCSCTL2 |= (SELM_2 + SELS);   //MCLK = SMCLK = XT2，1MHz
}

void Init_cap(void)
{
    P1SEL  |= 0x04;                   //选择P1.2作为捕获的输入端子
    P1DIR&=~BIT2;
    TACCTL1|= CM0+SCS+CCIS_0+CAP+CCIE;//上升沿触发，同步模式，使能中断
    TACTL  |= TASSEL1+ID_3+MC1;       //选择8M－SMCLK时钟，8分频、8*1=8M
}


void main(void)
{
  
     /*下面六行程序关闭所有的IO口*/
    P1DIR = 0XFF;P1OUT = 0XFF;                 // P1DIR = 0XF0;P1OUT = 0X0F;
    P2DIR = 0XFF;P2OUT = 0XFF;
    P3DIR = 0XFF;P3OUT = 0XFF;
    P4DIR = 0XFF;P4OUT = 0XFF;
    P5DIR = 0XFF;P5OUT = 0XFF;
    P6DIR = 0XFF;P6OUT = 0XFF;
    
    
    unsigned char temp=0,i=0,j=0;
    
    unsigned int str[2]={0},cap[10]={0};
    
    unsigned  long  count;
    
    WDTCTL = WDTPW + WDTHOLD;
    
    Init_System();
    
    Lcd_Reset();
    
    Lcd_WriteStr(0,1,"you");
    
    Init_cap();
    
    _EINT();
    
    while(1)
    {
      while(flag)
      {
        Lcd_WriteStr(3,0,"进入测频");
        _DINT();
        flag=0;
        str[temp]=TACCR1;
        temp++;
        
        if(temp==2)
        {
          temp=0;
          count=str[1]-str[0];
        /*  
          cap[i]=count;
          i++;
          if(i==10)
          {
            i=0; 
            for(j=0;j<10;j++)
            {
             count+=cap[j];
            }
          */  
          unsigned long f; 
        //  count=count/10;
          count=1000000/count;
          f=count;
         f=100000;
        //count/=10;
        // f=8*100000000/count;
        //if(f>=1000)
        // f/=100;
        
        unsigned char ptr[7];
        ptr[0]=f/1000000;
        ptr[1]=f%1000000/100000;
        ptr[2]=f%100000/10000;
        ptr[3]=f%10000/1000;
        ptr[4]=f%1000/100;
        ptr[5]=f%100/10;
        ptr[6]=f%10;
        
        Lcd_WriteStr(0,0,ptr);
   
         //Lcd_WriteStr(0,2,"Hz");
         //unsigned char *rst=&count;
         //Lcd_WriteStr(0,0,rst);
          Delay(2000);
          }
        }
         
      };
      _EINT();
  
}


#pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A(void)
{
    switch(TAIV)
    {
    case 2:{flag=1 ;break;};    //置位捕获标志1
    case 4:break;
    case 10:break;
    }
}