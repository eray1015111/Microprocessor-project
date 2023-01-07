/****************************************************************************
 * @file     main.c
 * @version  V2.0
 * $Revision: 1 $
 * $Date: 22/12/23 11:49a $
 * @brief    hakuna matata!!!!
 * @note
 * Copyright (C) 2022 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "NUC100Series.h"
#define PLL_CLOCK       50000000
#define RXBUFSIZE 256
#define frequency 20
#define Compare(a,b) a>=b?1:0
#define max_nack_counter 5
uint8_t g_u8RecData[RXBUFSIZE]  = {0};

volatile uint32_t g_u32comRbytes = 0;
volatile uint32_t g_u32comRhead  = 0;
volatile uint32_t g_u32comRtail  = 0;
volatile int32_t g_bWait_input   = TRUE;
volatile unsigned char crc_table[256] = {0};
volatile uint32_t g_trans_flag = 0;
volatile 	uint8_t receive_seq=0x00;
volatile uint32_t g_u32AdcIntFlag, g_u32mean_value = 2048, g_u32ConversionData;
typedef void (*ADC_FUNC)(void);
int nack_counter = 0 ;
static volatile ADC_FUNC s_Conversion = NULL;
/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
void SYS_Init(void);
void UART0_Init(void);
void AdcSingleModeTest(void);
void AdcSingleModeConversion(int);
void ADC_Init(void);
void TIMER_Init(void);
void decode(void);
void TMR0_IRQHandler(void);
void create_crc_table(void);
void trans_data(char data , uint8_t seq);
void trans_start();
void trans_header(int s_len);
int  trans_string(uint8_t s[],int s_len);
int	 main(void);
void UART_TEST_HANDLE(void);
void UART_get_input();
void receive(void);
void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);
		
    /* Enable ADC module clock */
    CLK_EnableModuleClock(ADC_MODULE);		

    /* Select UART module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_PLL, CLK_CLKDIV_UART(1));

		/* Enable Timer 0 module clock */
    CLK_EnableModuleClock(TMR0_MODULE);
		
		/* Select Timer 0 module clock source */
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, NULL);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
    SYS->GPB_MFP |= SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD;

    /* Disable the GPA0 - GPA3 digital input path to avoid the leakage current. */
    GPIO_DISABLE_DIGITAL_PATH(PA, 0xF);

    /* Configure the GPA0 - GPA3 ADC analog input pins */
    SYS->GPA_MFP &= ~(SYS_GPA_MFP_PA0_Msk | SYS_GPA_MFP_PA1_Msk | SYS_GPA_MFP_PA2_Msk | SYS_GPA_MFP_PA3_Msk) ;
    SYS->GPA_MFP |= SYS_GPA_MFP_PA0_ADC0 | SYS_GPA_MFP_PA1_ADC1 | SYS_GPA_MFP_PA2_ADC2 | SYS_GPA_MFP_PA3_ADC3 ;
    SYS->ALT_MFP1 = 0;
}

/* Init UART                                                                                               */
void UART0_Init()
{
    SYS_ResetModule(UART0_RST);
    UART_Open(UART0, 115200);
}

void GPIO_Init()
{
		GPIO_SetMode(PC, BIT12, GPIO_PMD_OUTPUT);
		GPIO_SetMode(PC, BIT13, GPIO_PMD_OUTPUT);
		GPIO_SetMode(PC, BIT14, GPIO_PMD_OUTPUT);
		GPIO_SetMode(PC, BIT15, GPIO_PMD_OUTPUT);
		PC13 =0;
		//PC13 =1;
}
void ADC_Init()////////
{
		/* Set the ADC operation mode as single, input mode as single-end and enable the analog input channel 1 */
    ADC_Open(ADC, ADC_ADCR_DIFFEN_SINGLE_END, ADC_ADCR_ADMD_SINGLE, 0x1 << 1);

    /* Power on ADC module */
    ADC_POWER_ON(ADC);
	
		/* Clear the A/D interrupt flag for safe */
		ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT);

    /* Enable the ADC interrupt */
    ADC_EnableInt(ADC, ADC_ADF_INT);
	
		//ADC_DisableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
}

void decode()
{
		uint8_t result; 
		result = Compare(g_u32ConversionData,g_u32mean_value);
		printf("In ADC\n");
		//printf("%d",result);
}
void AdcSingleModeConversion(int index)
{		
		//s_Conversion = NULL;
		printf("index %d\n",index);
		g_u32AdcIntFlag = 0;
		ADC_START_CONV(ADC); //ADC->ADCR |= (1<<11)
		while(g_u32AdcIntFlag==0);
		g_u32ConversionData = ADC_GET_CONVERSION_DATA(ADC, 1); //i32ConversionData = ADC->ADDR[1]&0XFFFFul;
		receive_seq |= ((Compare(g_u32ConversionData,g_u32mean_value) & 0x01) << index);
		
		//receive();
}
void ADC_IRQHandler(void)
{
		g_u32AdcIntFlag = 1;
    ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT); /* clear the A/D conversion flag ADSR->BUSY=0  ADC_ADF_INT=1*/
}
void TMR0_IRQHandler()
{
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
        TIMER_ClearIntFlag(TIMER0);
        g_trans_flag = 1;
				if(nack_counter > max_nack_counter){
					printf("TOO MUCH ERROR, Please enter again!\n\n");
					NVIC_SystemReset();
				} 
    }
}

void UART02_IRQHandler(void)
{
    UART_TEST_HANDLE();
}
void UART_TEST_HANDLE()
{
	int i; 
    uint8_t u8InChar = 0xFF;// 1111 1111 1111 1111
    uint32_t u32IntSts = UART0->ISR;
		
	GPIO_SetMode(PA,BIT12,GPIO_PMD_OUTPUT);
	GPIO_SetMode(PA,BIT13,GPIO_PMD_OUTPUT);
	GPIO_SetMode(PA,BIT14,GPIO_PMD_OUTPUT);	
    if(u32IntSts & UART_ISR_RDA_INT_Msk){ 
		// UART_ISR_RDA_INT_Msk(Rx ready interrupt) = (1ul << UART_ISR_RDA_INT_Pos)
		// lul = 0000 0000 0000 0001; UART_ISR_RDA_INT_Pos = 0000 0000 0000 1000; by definition
		// So that UART_ISR_RDA_INT_Msk = 0000 0001 0000 0000
		
		u8InChar = UART_READ(UART0);	
		if(u8InChar==0X7f ){ //Backspce button is pressed
			if(g_u32comRtail> 0){
				g_u32comRtail--;
				g_u8RecData[g_u32comRtail]='\0';

				if(u32IntSts & UART_ISR_THRE_INT_Msk){
					UART_WAIT_TX_EMPTY(UART0);
					UART_WRITE(UART0, u8InChar);
					PA12 = 0;
					PA13 = 1;
					PA14 = 1;
				}
			}
		}else if(u8InChar==0X0D){ //Enter button is pressed
			if(g_u32comRtail> 0){
				g_u8RecData[g_u32comRtail]='\0'; //end of string
				g_bWait_input = FALSE;
				if(u32IntSts & UART_ISR_THRE_INT_Msk){
					UART_WAIT_TX_EMPTY(UART0);
					UART_WRITE(UART0, '\n');
				}
				PA13 = 0;
				PA12 = 1;
				PA14 = 1;
			}	
		}else if(g_u32comRtail < RXBUFSIZE){
			g_u8RecData[g_u32comRtail] = u8InChar;
			g_u32comRtail++;
			if(u32IntSts & UART_ISR_THRE_INT_Msk){
				UART_WAIT_TX_EMPTY(UART0);
				UART_WRITE(UART0, u8InChar);
				PA14 = 0;
				PA12 = 1;
				PA13 = 1;
			}
		}
	}
}
void UART_get_input()
{
    printf("\nInput:");

    /* Enable Interrupt and install the call back function */
    UART_EnableInt(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_TOUT_IEN_Msk));
    while(g_bWait_input);

    /* Disable Interrupt */
    UART_DisableInt(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_TOUT_IEN_Msk));
    g_bWait_input = TRUE;
    printf("UART Enter End.\n");

}

void create_crc_table(void)
{
	unsigned char element = 0;
	unsigned char crc;
	int i;
	
	while(1)
	{
		crc = 0x00;
		
		crc ^= element;
		for(i=0;i<8;i++){
			if(crc & 1){
				crc = (crc >> 1) ^ 0x8C;    // 0x8C = reverse 0x31
			}else{
				 crc >>= 1;
			}
		}
      
		if(element == 0xff){
			break;
		}
		//printf("element:%x crc:%x\n",element,crc);
	crc_table[element] = crc;
	element++;
	}
	
	printf("\n\nFinish create CRC table\n");
}

void trans_data(char data, uint8_t seq){
	unsigned char output = data;
	int counter;
	printf("transmit data with crc:%x + %x\n",data,crc_table[data]);
	output = (seq & 0x01)<<7 ;
	output |= data;
	for(counter = 0;counter<8;counter++){
		while(g_trans_flag == 0);
		g_trans_flag = 0;
		*(&PC13) = !(output & 0x01);
		printf("%d",(output & 0x01));
		output >>= 1;
	}
	printf("\n");
	
	output = crc_table[data];
	for(counter = 0;counter<8;counter++){
		while(g_trans_flag == 0);
		g_trans_flag = 0;
		*(&PC13) = !(output & 0x01);
		printf("%d",(output & 0x01));
		output >>= 1;
	}
	printf("\n");
}

void trans_start(){
	int counter;
	unsigned char output = 0xAA;
	printf("\ntransmit strat bit\n");
	for(counter = 0;counter<8;counter++){
		
		while(g_trans_flag == 0);
		g_trans_flag = 0;
		
		*(&PC13) = !(output & 0x01);
		printf("%d",output & 0x01);
		output >>= 1;
	}
}

void trans_header(int s_len){
	unsigned char output = s_len;
	unsigned char data = s_len;
	int counter;
	printf("transmit header\n");
	//trans_data(output);
	printf("transmit data with crc:%c + %x\n",data,crc_table[data]);
	for(counter = 0;counter<8;counter++){
		while(g_trans_flag == 0);
		g_trans_flag = 0;
		*(&PC13) = !(output & 0x01);
		printf("%d",(output & 0x01));
		output >>= 1;
	}
	printf("\n");
	
	output = crc_table[data];
	for(counter = 0;counter<8;counter++){
		while(g_trans_flag == 0);
		g_trans_flag = 0;
		*(&PC13) = !(output & 0x01);
		printf("%d",!(output & 0x01));
		output >>= 1;
	}
	printf("\n");
}
void receive(void){
		while(1){
				if(s_Conversion != NULL){
						s_Conversion();
				}
		}
		/* Disable ADC module */
    ADC_Close(ADC);
    /* Disable ADC IP clock */
    CLK_DisableModuleClock(ADC_MODULE);
    /* Disable External Interrupt */
    NVIC_DisableIRQ(ADC_IRQn);

}
int trans_string(uint8_t s[],int s_len){
	int counter;
	uint8_t send_seq=0;
	unsigned char output = 0x00;
	int i=0,j , result;
	uint8_t ack[] = {0xaa, 0xab};
	int index =0;
	printf("transmit content\n");
	for(counter = 0;counter<=s_len;){
		printf("s_len;= %d counter =%d\n",s_len,counter);
		output = counter == s_len?0x0D:s[counter];//end of string
		send_seq = ((counter % 2)|0xaa);//0xaa 0xab
		trans_data(output,send_seq);
		printf("receive :\n");
		while(g_trans_flag == 0);
				g_trans_flag = 0;
		for(i=0;i<8;i++){
				//printf("i %d\n",i);
				while(g_trans_flag == 0);
				g_trans_flag = 0;
				g_u32AdcIntFlag = 0;
				ADC_START_CONV(ADC); //ADC->ADCR |= (1<<11)
				//printf("i %d\n",i);
				while(g_u32AdcIntFlag==0){}
						g_u32ConversionData = ADC_GET_CONVERSION_DATA(ADC, 1); //i32ConversionData = ADC->ADDR[1]&0XFFFFul;
						result = Compare(g_u32ConversionData,g_u32mean_value);
						receive_seq |= ((result & 0x01) << i);
						//printf("g_u32ConversionData:  %d \n",g_u32ConversionData);
						printf("result:  %d \n",result);
						
		}
		//receive_seq=(receive_seq|0xaa);
		printf("receive_seq = %x\n",receive_seq);
		
		//receive_seq=0;
		//UART_EnableInt(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_TOUT_IEN_Msk));
		if((receive_seq ^ send_seq)==1 ){//next pkt
			printf("ACK %d\n",receive_seq);
			counter++;
			nack_counter = 0 ;
			//send_seq = receive_seq;
			//index++;
		}else{// if(receive_seq != ack[index+1]){//retrans pkt
			printf("NACK %d\n",receive_seq);
			nack_counter ++ ;
			//counter--;
			//counter++;//Must DELETE!!!! 
		}
		//else if(receive_seq==0xFF){//retrans all
			//UART_DisableInt(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_TOUT_IEN_Msk));
			//return 1;
		//}else{//set time out??
		
		//}
		
		//index%=1;
		//send_seq = receive_seq;
		receive_seq = 0;
		printf("send_seq = %x\n",send_seq);
		printf("counter = %d\n",counter);
		printf("\n");
		//UART_DisableInt(UART0, (UART_IER_RDA_IEN_Msk | UART_IER_THRE_IEN_Msk | UART_IER_TOUT_IEN_Msk));
	}
	return 0;
}


/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int main(void)
{
	int t_fre;
	int i,j,k;
	int retran_flag=0;
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

		//Init GPIO
		GPIO_Init();
 
		ADC_Init();
		/* Disable ADC module */
    //ADC_Close(ADC);
    /* Disable ADC IP clock */
    //CLK_DisableModuleClock(ADC_MODULE);
    /* Disable External Interrupt */
    //NVIC_DisableIRQ(ADC_IRQn);
		
	/* Open Timer0 in periodic mode, enable interrupt and 1 interrupt tick per second */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, frequency);
		
		create_crc_table(); 
    
	while(1){
			ADC_Init();
			UART_get_input();
		retran:		
			printf("\nData transport start!\n");	
			TIMER_EnableInt(TIMER0);
			NVIC_EnableIRQ(TMR0_IRQn);
			TIMER_Start(TIMER0);			
			printf("Output Data:\t%s\n",g_u8RecData);
			printf("string length = %d \n",g_u32comRtail);
			PA13 = 1;			
			//PC13 = 1;
			trans_start();
			trans_header(g_u32comRtail);
			retran_flag = trans_string(g_u8RecData,g_u32comRtail);
			if(retran_flag) goto retran;
			printf("\033[32;40;1m transport end!\n \033[0m");
			//claer string
			memset(g_u8RecData,RXBUFSIZE,'\0');
			g_u32comRtail=0;
			//stop timer
			TIMER_DisableInt(TIMER0);
			NVIC_DisableIRQ(TMR0_IRQn);
			TIMER_Stop(TIMER0);
/*			receive_seq=0x00;
			g_u32comRbytes = 0;
			g_u32comRhead  = 0;
			g_u32comRtail  = 0;
			s_Conversion = NULL;*/
			//ADC_Close(ADC);

		}

}

