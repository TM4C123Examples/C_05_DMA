#include "TM4C123.h"                    // Device header

/*
	Signal 	PIN	 			Description	
	AIN8 		60 PE5  	Analog Analog-to-digital converter input 8.
	AIN9 		59 PE4  	Analog Analog-to-digital converter input 9.
	AIN10 	58 PB4  	Analog Analog-to-digital converter input 10.
	AIN11 	57 PB5  	Analog Analog-to-digital converter input 11.
*/
void config_ADC(void);
void start_ADC(void);

int ADC_samples[4];
int DMA_CH14_Transfers=0;
int transferComplete=1;
int a=0;

void config_DMA(void);
void reloadDMA_DMA_Chanel14(void);


typedef struct {
  void * sourceEndPointer;
	void * destinationEndPointer;
  unsigned int controlWord;
	unsigned int unused;
} CHControlStruct;

typedef struct {
  CHControlStruct primary[32];
	CHControlStruct alternate[32];
} DMAControlStruct;

DMAControlStruct myControlStruct __attribute__((aligned(1024)));
 
int *USTAT=(int *)0x40038018;
int *SSFIFO=(int *)0x4003804C;
int main(){
	config_ADC();
	config_DMA();
	//NVIC_EnableIRQ(UDMA_IRQn); //this is for sofware triggerd trasfers 
	NVIC_EnableIRQ(ADC0SS0_IRQn);//the DMA asserts this interrupt when transfer complete
	while(1){
		a++;
		if(transferComplete){
			transferComplete=0;
			start_ADC();//Start another conversion sequence 
		}
	}
}


void config_DMA(){
	//Module Initialization
	SYSCTL->RCGCDMA|=0x1;//enable DMA peripherial
	UDMA->CFG=0x0;//Disables DMA controller
	myControlStruct.primary[14].sourceEndPointer=(void *)0x40038048;
	myControlStruct.primary[14].destinationEndPointer=(void *)&ADC_samples[3];
	myControlStruct.primary[14].controlWord=(0x2U<<30)|//Destination increment: Word
																					(0x2<<28)|//Destination Data Size: Word
																					(0x3<<26)|//Source address increment:	No increment						
																					(0x2<<24)|//Source Data Size: Word
																					(0x1<<14)|//Arbitration Size 2 Transfers
																					(0x3<<4)|//Trasfers Size 4 //reg =(n-1)
																					(0x1<<0);//Transfer Mode Basic
	UDMA->CTLBASE=(int)&myControlStruct;//pointer to base of DMA Control Structure
	//Configure Channel 14  Map Select 0 = ADC0 SS0
	UDMA->ALTCLR|=(0x1<<14);//use primary control structure
	UDMA->USEBURSTSET|=(0x1<<14);//channel 14 responds onnly to burst requests
	UDMA->REQMASKCLR|=(0x1<<14);
	UDMA->CHMAP1|=(0x0<<24);
	
	UDMA->ENASET|=(0x1<<14);//Enable Chanel
	UDMA->CFG=0x1;//Enables DMA controller
}

void reloadDMA_DMA_Chanel14(){
	//Module Initialization
	myControlStruct.primary[14].controlWord=(0x2U<<30)|//Destination increment: Word
																					(0x2<<28)|//Destination Data Size: Word
																					(0x3<<26)|//Source address increment:	No increment						
																					(0x2<<24)|//Source Data Size: Word
																					(0x1<<14)|//Arbitration Size 2 Trasfers
																					(0x3<<4)|//Trasfers Size 4 //reg =(n-1)
																					(0x1<<0);//Transfer Mode Basic
	//UDMA->USEBURSTSET|=(0x1<<14);//channel 14 responds onnly to burst requests
	//UDMA->REQMASKCLR|=(0x1<<14);
	//UDMA->CHMAP1|=(0x0<<24);
	
	UDMA->ENASET|=(0x1<<14);//Enable Chanel
}

void start_ADC(){
	ADC0->PSSI=(0x1<<0);//inicia secuenciador 0
	//ADC_sampleEnd=0;
}

void config_ADC(){
	/*Configuracion de Pines*/
	
	SYSCTL->RCGCGPIO=(0x1<<4)|(0x1<<1)|(0x1<<3);
	SYSCTL->RCGCADC|=0x01;//habilitamos ADC0
	
	GPIOE->DEN&=~((0x1<<4)|(0x1<<5));
	GPIOB->DEN&=~((0x1<<4)|(0x1<<5));
	GPIOE->AMSEL|=(0x1<<4)|(0x1<<5);
	GPIOB->AMSEL|=(0x1<<4)|(0x1<<5);
	
	
	//Configuracion ADC principal
	
	ADC0->ACTSS&=~(0xF);//desactivamos secuenciadores durantes la configuracion
	ADC0->EMUX=0; //para trigger por sofware (default)
	
	//Configuracion del sequenciador
	ADC0->SSMUX0=(0x8)|(0x9<<4)|(0xA<<8)|(0xB<<12);
	ADC0->SSCTL0=(0x1<<14)|(0x1<<13)|(0x1<<6);//Final de secuencia en muestra 3, y genera interrupcion
	// en muestra 1 y muestra 3 
	//Desenmacaramos interrupcion por secuenciador 0
	//ADC0->IM|=(0x1<<0);
	ADC0->ACTSS|=(0x1<<0);//activamos secuenciador 0
}

void ADC0SS0_Handler(){
		UDMA->CHIS|=(0x1<<14);//clear interrupts caused by channel 14
		DMA_CH14_Transfers++;
		reloadDMA_DMA_Chanel14();
	  transferComplete=1;
}
/*
void ADC0SS0_Handler(){
	int i;
	ADC0->ISC=(0x1<<0);//Limpiamos interrupcion por secuenciador 0
	for(i=0;i<4;i++){
		if((ADC0->SSFSTAT0&(0x1<<8))){
			 break;
		}
		ADC_samples[i]=ADC0->SSFIFO0;
	}
	if(((ADC0->SSFSTAT0&(0x1<<8))!=0)&&i==4){
			ADC_sampleOk=1;
	}else{
		  ADC_sampleOk=0;
	}
	ADC_sampleEnd=1;
}
*/
