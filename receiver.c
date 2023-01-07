#include <stdio.h>
#include  <stdbool.h>
#include <stdlib.h>
#include "NUC100Series.h"
#define PLL_CLOCK       50000000
#define Compare(a,b) a>=b?1:0
#define start_signal 0xAA
#define frequency 15
#define max_nack_counter 10
/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
	void(*function)(uint8_t);
	int length;
}decode_function;
typedef struct
{
	int index;
	int alphabet;
	int next_function;
	int buffer_index;
}decode_parameter;
void SYS_Init(void);
void Response(void);
void UART0_Init(void);
void AdcSingleModeConversion(void);
void GPIO_Init(void);
void ADC_Init(void);
void TIMER_Init(void);
void decode(void);
void create_crc_table(void);
typedef void(*ADC_FUNC)(void);
static volatile ADC_FUNC s_Conversion = NULL;
void parameter_init(void);
void crc_function(uint8_t);
void start_function(uint8_t);
void header_function(uint8_t);
void data_function(uint8_t);
/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint32_t g_u32AdcIntFlag, g_u32mean_value = 1508, g_u32ConversionData;
int correct = 0;
int R_T = 1;
int times = 0;
int next_seq = 0;
int error_seq = 0;
int the_end = 0;
int nack_counter = 0;
volatile unsigned char crc_table[256] = { 0 };
void create_crc_table(void);
char* buffer;
volatile uint8_t g_receive_seq = 0x00;
volatile decode_parameter g_parameter;
volatile uint8_t header_strlens = 0, actually_strlens = 0;
const decode_function g_function_list[] = {
				crc_function , 8,
				start_function , 8,
				header_function , 8,
				data_function , 8
				//
};
enum
{
	Crc = 0,
	Start,
	Header,
	Data

};
void create_crc_table(void)
{
	unsigned char element = 0;
	unsigned char crc;
	int i;

	while (1)
	{
		crc = 0x00;

		crc ^= element;
		for (i = 0; i < 8; i++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0x8C;    // 0x8C = reverse 0x31
			}
			else {
				crc >>= 1;
			}
		}

		if (element == 0xff) {
			break;
		}
		crc_table[element] = crc;
		element++;
	}
}
void parameter_init()
{
	g_parameter.index = 0;
	g_parameter.alphabet = 0;
	g_parameter.buffer_index = 0;
	g_parameter.next_function = 1;
	free(buffer);
	buffer = NULL;
}
void crc_function(uint8_t result)
{
	int i = 0;
	static int crc = 0;
	if (error_seq)
	{
		g_parameter.index++;
		if (g_parameter.index == g_function_list[g_parameter.next_function].length) {
			g_parameter.index = 0;
			if (times != 0) {
				R_T = 0;
				correct = 0;
			}
			times++;
			printf("error_seq\n");
			g_parameter.next_function = Data;
		}
	}
	else {
		crc |= (result << g_parameter.index++);
		if (g_parameter.index == g_function_list[g_parameter.next_function].length) {
			g_parameter.index = 0;
			if (crc == crc_table[g_parameter.alphabet]) {
				printf("CRC correct\n"); //	
				nack_counter = 0;
				actually_strlens++;
				//printf("actually_strlens =%d\n",actually_strlens);
				if (times != 0) {
					correct = 1;
					R_T = 0;
				}
				crc = 0;
				if (g_parameter.alphabet != 0x0d) {  //enter
					g_parameter.next_function = Data;

				}
				else {
					printf("Receive End\n");
					buffer[g_parameter.buffer_index] = '\0';
					printf("Data:\t%s\n", buffer);
					printf("Data len %d\n ", sizeof(buffer));

					parameter_init();
					//printf("header_strlens = %d  actually_strlens-2 = %d\n",header_strlens,actually_strlens-2);
					if (header_strlens != actually_strlens - 2) nack_counter = max_nack_counter + 1;//Respones(2); 
					else printf("\033[32;40;1m ALL CORRECT!!\033[0m");

					the_end = 1;
				}
				g_parameter.alphabet = 0;
			}
			else {		//return nack to modulator
				printf("\033[31;40;1m CRC Error!\n \033[0m");
				nack_counter++;
				if (times != 0) {
					R_T = 0;
					correct = 0;
				}
				crc = 0;
				g_parameter.alphabet = 0;
				g_parameter.next_function = Data;

			}
			times++;
			printf("\n");
		}
	}

}
void Response()
{
	static int j = 0;
	int result;
	const uint8_t ack[] = { 0xaa, 0xab };
	s_Conversion = NULL;
	//printf("correct = %d\n",correct);
	if (correct == 1) {
		//printf("result =%d\n",((ack[index] >> j)&0x01));
		result = ((ack[!next_seq] >> j++) & 0x01);
		if (!the_end) {
			printf("result = %d\n", result);
		}

		//PC13 = ((ack[!next_seq] >> j++)&0x01);
		PC13 = result;
		if (j == 8) {
			R_T = 3;
			j = 0;
			if (the_end) {
				next_seq = 0;
				the_end = 0;
				times = 0;
				actually_strlens = 0;
				//printf("correct = %d\nR_T = %d\ntimes = %d\nnext_seq = %d\ncorrect = %d\n",correct,R_T,times,next_seq,error_seq);
			}
			else {
				next_seq = !next_seq;
				g_parameter.buffer_index++;

			}
		}
	}
	if (correct == 0) {
		//printf("result =%d\n",((ack[index] >> j)&0x01));
		result = ((ack[next_seq] >> j++) & 0x01);
		printf("result = %d\n", result);
		//PC13 = ((ack[next_seq] >> j++)&0x01);
		PC13 = result;
		if (j == 8) {
			R_T = 3;
			j = 0;
		}
	}
	if (correct == 2) {
		//printf("result =%d\n",((ack[index] >> j)&0x01));
		PC13 = ((0xff >> j++) & 0x01);
		if (j == 8) {
			R_T = 3;
			j = 0;
		}
	}


}
void start_function(uint8_t result)
{
	//printf("g_parameter.index = %d\n",g_parameter.index);
	if (result == ((start_signal >> g_parameter.index) & 0x01)) {
		g_parameter.index++;
		//printf("g_parameter.index %d\n\n",g_parameter.index);
		if (g_parameter.index == g_function_list[g_parameter.next_function].length) {
			printf("Start!\n");
			g_parameter.index = 0;
			g_parameter.next_function = Header;
		}
	}
	else {
		g_parameter.index = 0;
	}
}
void header_function(uint8_t result)
{
	g_parameter.alphabet |= (result << g_parameter.index++);
	//printf("g_parameter.index %d\n",g_parameter.index);
	if (g_parameter.index == g_function_list[g_parameter.next_function].length) {
		printf("header = %d\n", g_parameter.alphabet); //
		header_strlens = g_parameter.alphabet;
		buffer = (char*)malloc(sizeof(char)*g_parameter.alphabet);
		g_parameter.index = 0;
		g_parameter.next_function = Crc;
	}
}
void data_function(uint8_t result)
{
	g_parameter.alphabet |= (result << g_parameter.index++);
	if (g_parameter.index == g_function_list[g_parameter.next_function].length) {
		g_receive_seq = g_parameter.alphabet & 0x80;//get seq 
		g_receive_seq = g_receive_seq >> 7;
		g_parameter.alphabet &= 0x7f; //remove seq 
		printf("next_seq = %d\n", next_seq);
		//printf("g_receive_seq = %d\n",g_receive_seq);
		//printf("g_ack = %d\n",g_ack);
		if (g_receive_seq == next_seq) {//correct frame seq
			printf("data = %c\n", g_parameter.alphabet);
			buffer[g_parameter.buffer_index] = g_parameter.alphabet;
			g_parameter.index = 0;
			g_parameter.next_function = Crc;
			error_seq = 0;


		}
		else {//duplicate frame
			error_seq = 1;
			printf("\033[31;40;1m Drop duplicate frame!\n \033[0m");
			printf("return nack\n");
			nack_counter++;
			g_parameter.index = 0;
			g_parameter.next_function = Crc;
		}
	}

}



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

	/* ADC clock source is 22.1184MHz, set divider to 7, ADC clock is 22.1184/7 MHz */
	CLK_SetModuleClock(ADC_MODULE, CLK_CLKSEL1_ADC_S_HIRC, CLK_CLKDIV_ADC(7));

	/* Enable Timer 0~3 module clock */
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);
	CLK_EnableModuleClock(TMR3_MODULE);

	/* Select Timer 0~3 module clock source */
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0_S_HXT, NULL);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HCLK, NULL);
	CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2_S_HIRC, NULL);
	CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3_S_HXT, NULL);

	/*---------------------------------------------------------------------------------------------------------*/
	/* Init I/O Multi-function                                                                                 */
	/*---------------------------------------------------------------------------------------------------------*/

	/* Set GPB multi-function pins for UART0 RXD and TXD */
	SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
	SYS->GPB_MFP |= SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD;

	/* Disable the GPA0 - GPA3 digital input path to avoid the leakage current. */
	GPIO_DISABLE_DIGITAL_PATH(PA, 0xF);

	/* Configure the GPA0 - GPA3 ADC analog input pins */
	SYS->GPA_MFP &= ~(SYS_GPA_MFP_PA0_Msk | SYS_GPA_MFP_PA1_Msk | SYS_GPA_MFP_PA2_Msk | SYS_GPA_MFP_PA3_Msk);
	SYS->GPA_MFP |= SYS_GPA_MFP_PA0_ADC0 | SYS_GPA_MFP_PA1_ADC1 | SYS_GPA_MFP_PA2_ADC2 | SYS_GPA_MFP_PA3_ADC3;
	SYS->ALT_MFP1 = 0;

}

/*---------------------------------------------------------------------------------------------------------*/
/* Init UART                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_Init()
{
	/* Reset IP */
	SYS_ResetModule(UART0_RST);

	/* Configure UART0 and set UART0 Baudrate */
	UART_Open(UART0, 115200);
}
void GPIO_Init()
{
	GPIO_SetMode(PC, BIT12, GPIO_PMD_OUTPUT);
	GPIO_SetMode(PC, BIT13, GPIO_PMD_OUTPUT);
	GPIO_SetMode(PC, BIT14, GPIO_PMD_OUTPUT);
	GPIO_SetMode(PC, BIT15, GPIO_PMD_OUTPUT);
	//PC13 = 0;
}
void ADC_Init()
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
void TIMER_Init()
{
	// Open Timer0 in periodic mode, enable interrupt and 1 interrupt tick per second 
	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, frequency);
	TIMER_EnableInt(TIMER0);

	// Open Timer1 in periodic mode, enable interrupt and 2 interrupt ticks per second 
	/*TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 2);
	TIMER_EnableInt(TIMER1);

	// Open Timer2 in periodic mode, enable interrupt and 4 interrupt ticks per second
	TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 4);
	TIMER_EnableInt(TIMER2);

	// Open Timer3 in periodic mode, enable interrupt and 8 interrupt ticks per second
	TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, 8);
	TIMER_EnableInt(TIMER3);*/

	// Enable Timer0 ~ Timer3 NVIC 
	NVIC_EnableIRQ(TMR0_IRQn);
	//NVIC_EnableIRQ(TMR1_IRQn);
	//NVIC_EnableIRQ(TMR2_IRQn);
	//NVIC_EnableIRQ(TMR3_IRQn);
}
void AdcSingleModeConversion()
{
	s_Conversion = NULL;
	g_u32AdcIntFlag = 0;
	ADC_START_CONV(ADC); //ADC->ADCR |= (1<<11)
	while (g_u32AdcIntFlag == 0);
	g_u32ConversionData = ADC_GET_CONVERSION_DATA(ADC, 1); //i32ConversionData = ADC->ADDR[1]&0XFFFFul;
	decode();


}

void decode()
{
	uint8_t result;
	result = Compare(g_u32ConversionData, g_u32mean_value);
	//printf("g_u32ConversionData = %d\n result= %d\n",g_u32ConversionData,result);
	g_function_list[g_parameter.next_function].function(result);

}

/*---------------------------------------------------------------------------------------------------------*/
/* ADC interrupt handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void ADC_IRQHandler(void)
{
	g_u32AdcIntFlag = 1;
	ADC_CLR_INT_FLAG(ADC, ADC_ADF_INT); /* clear the A/D conversion flag ADSR->BUSY=0  ADC_ADF_INT=1*/
}
/*---------------------------------------------------------------------------------------------------------*/
/* TMR0 interrupt handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void TMR0_IRQHandler(void)
{
	if (TIMER_GetIntFlag(TIMER0) == 1)
	{
		/* Clear Timer0 time-out interrupt flag */
		TIMER_ClearIntFlag(TIMER0);
		//printf("100\n");
		if (R_T == 1) {
			s_Conversion = (ADC_FUNC)AdcSingleModeConversion;
			//AdcSingleModeConversion();
		}
		else if (R_T == 0) {
			s_Conversion = (ADC_FUNC)Response;
		}
		else if (R_T == 3) {
			R_T = 1;
		}
		if (nack_counter > max_nack_counter) {
			printf("TOO MUCH ERROR, Please enter again!\n\n");
			NVIC_SystemReset();
		}
	}

}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
	int i = 0;
	/* Unlock protected registers */
	SYS_UnlockReg();

	/* Init System, IP clock and multi-function I/O */
	SYS_Init();

	/* Lock protected registers */
	SYS_LockReg();

	/* Init UART0 for printf */
	UART0_Init();

	TIMER_Init();

	GPIO_Init();
	//PC13 = 0;
	ADC_Init();
	create_crc_table();
	g_parameter.index = 0;
	g_parameter.alphabet = 0;
	g_parameter.buffer_index = 0;
	g_parameter.next_function = 1;
	g_receive_seq = 0x00;
	buffer = NULL;
	TIMER_Start(TIMER0);
	printf("Ready!\n\n");
	while (1) {
		if (s_Conversion != NULL) {
			//TIMER_Stop(TIMER0);
				//PC13 = !PC13;
			//TIMER_Start(TIMER0);
			s_Conversion();
		}
	}

	/* Disable ADC module */
	ADC_Close(ADC);

	/* Disable ADC IP clock */
	CLK_DisableModuleClock(ADC_MODULE);

	/* Disable External Interrupt */
	NVIC_DisableIRQ(ADC_IRQn);

	while (1);

}



