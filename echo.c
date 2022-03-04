// CC-by-www.Electrosmash.com Pedal-Pi open-source project.
// echo audio effect.

#include <stdio.h>
#include <bcm2835.h>

// Define Input Pins
#define PUSH1 			RPI_GPIO_P1_08  	//GPIO14
#define PUSH2 			RPI_V2_GPIO_P1_38  	//GPIO20 
#define TOGGLE_SWITCH 	RPI_V2_GPIO_P1_32 	//GPIO12
#define FOOT_SWITCH 	RPI_GPIO_P1_10 		//GPIO15
#define LED   			RPI_V2_GPIO_P1_36 	//GPIO16

//Define Delay Effect parameters MAX_DELAY 800000 is 4 seconds approx.
#define DELAY_MAX 800000
#define DELAY_MIN 0

uint32_t Echo_Buffer[DELAY_MAX];
uint32_t DelayCounter = 0;
uint32_t Delay_Depth = 100000; //default starting delay is 100000 is 0.5 sec approx.

uint32_t input_signal=0;
uint32_t output_signal=0;
uint32_t read_timer, delay;

uint8_t FOOT_SWITCH_val;
uint8_t TOGGLE_SWITCH_val;
uint8_t PUSH1_val;
uint8_t PUSH2_val;

int main(int argc, char** argv) {
    // Start the BCM2835 Library to access GPIO.
    if (!bcm2835_init())
    {printf("bcm2835_init failed. Are you running as root??\n");
      return 1;}
	// Start the SPI BUS.
	if (!bcm2835_spi_begin())
    {printf("bcm2835_spi_begin failed. Are you running as root??\n");
      return 1;}
	  
		//define PWM	
    bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_ALT5 ); //PWM0 signal on GPIO18    
    bcm2835_gpio_fsel(13,BCM2835_GPIO_FSEL_ALT0 ); //PWM1 signal on GPIO13    
	bcm2835_pwm_set_clock(2); // Max clk frequency (19.2MHz/2 = 9.6MHz)
    bcm2835_pwm_set_mode(0,1 , 1); //channel 0, markspace mode, PWM enabled. 
	bcm2835_pwm_set_range(0,64);   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz switching PWM freq.
    bcm2835_pwm_set_mode(1, 1, 1); //channel 1, markspace mode, PWM enabled.
	bcm2835_pwm_set_range(1,64);   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz switching PWM freq.
	
	//define SPI bus configuration
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); 	  // 4MHz clock with _64 
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
	
   	uint8_t mosi[10] = { 0x01, 0x00, 0x00 }; //12 bit ADC read 0x08 ch0, - 0c for ch1 
	uint8_t miso[10] = { 0 };
		
    //Define GPIO pins configuration
    bcm2835_gpio_fsel(PUSH1, BCM2835_GPIO_FSEL_INPT); 			//PUSH1 button as input
	bcm2835_gpio_fsel(PUSH2, BCM2835_GPIO_FSEL_INPT); 			//PUSH2 button as input
	bcm2835_gpio_fsel(TOGGLE_SWITCH, BCM2835_GPIO_FSEL_INPT);	//TOGGLE_SWITCH as input
	bcm2835_gpio_fsel(FOOT_SWITCH, BCM2835_GPIO_FSEL_INPT); 	//FOOT_SWITCH as input
	bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);				//LED as output
	
    bcm2835_gpio_set_pud(PUSH1, BCM2835_GPIO_PUD_UP);           //PUSH1 pull-up enabled   
	bcm2835_gpio_set_pud(PUSH2, BCM2835_GPIO_PUD_UP);           //PUSH2 pull-up enabled 
	bcm2835_gpio_set_pud(TOGGLE_SWITCH, BCM2835_GPIO_PUD_UP);   //TOGGLE_SWITCH pull-up enabled 
	bcm2835_gpio_set_pud(FOOT_SWITCH, BCM2835_GPIO_PUD_UP);     //FOOT_SWITCH pull-up enabled 

 
 while(1) //Main Loop
 {
	//read 12 bits ADC
    bcm2835_spi_transfernb(mosi, miso, 3);
    input_signal = miso[2] + ((miso[1] & 0x0F) << 8); 

	//Read the PUSH buttons every 50000 times (0.25s) to save resources.
	read_timer++;
	if (read_timer==50000)
	{
	read_timer=0;
	PUSH1_val = bcm2835_gpio_lev(PUSH1);
	PUSH2_val = bcm2835_gpio_lev(PUSH2);
    TOGGLE_SWITCH_val = bcm2835_gpio_lev(TOGGLE_SWITCH);
	FOOT_SWITCH_val = bcm2835_gpio_lev(FOOT_SWITCH);
	//light the effect when the footswitch is activated.
	bcm2835_gpio_write(LED,!FOOT_SWITCH_val); 
		
	//update volume variable when the PUSH buttons are pushed.
	if (PUSH2_val==0)
		{   bcm2835_delay(100); //100ms delay for buttons debouncing. 
			if (Delay_Depth<DELAY_MAX)Delay_Depth=Delay_Depth+50000; //50000 is 250ms approx.
		}
           if (PUSH1_val==0) 
			{ 	
			    bcm2835_delay(100); //100ms delay for buttons debouncing. 
				if (Delay_Depth>DELAY_MIN)Delay_Depth=Delay_Depth-50000;
			}
			
	}

	//**** ECHO EFFECT ***///
	//The echo effect is very similar to the delay, but instead of saving the delayed signal in a buffer and
	//add it to the current guitar input, we store the delayed+current signal in the Echo_Buffer
	//With PUSH1 and PUSH2 the delay time is controlled.
	
	//Store current readings  
	//the ">>1" makes the echo to decay, if you want a faster decay change it for ">>2" or ">>3" 
    Echo_Buffer[DelayCounter]  = (input_signal + Echo_Buffer[DelayCounter])>>1;
    DelayCounter++;	
	if(DelayCounter >= Delay_Depth) DelayCounter = 0; 

    output_signal = (input_signal + (Echo_Buffer[DelayCounter]))>>1;
	
    //generate output PWM signal 6 bits
    bcm2835_pwm_set_data(1,output_signal & 0x3F);
    bcm2835_pwm_set_data(0,output_signal >> 6);

 }
 	//close all and exit
	bcm2835_spi_end();
    bcm2835_close();
    return 0;
}




