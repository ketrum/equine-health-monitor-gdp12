// University of Southampton, 2012
// EMECS Group Design Project

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "efm32.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "trace.h"
#include "fatfs.h"
#include "messagestorage.h"
#include "audio.h"
#include "accelerationsensor.h"
#include "temperaturesensor.h"
#include "port_config.h"


MessageStorage * msgStore;

void saveAudioSample(uint16_t audioLenS)
{
//	printf("start acquiring audio sample, duration %d seconds \n", audioLenS);
	const uint16_t bufferSize = 1000;
	Audio * mic = new Audio(Fs_8khz, bufferSize);
	mic->init();

	msgStore->startAudioSample();	
	mic->startRecording(audioLenS);
	
	int buffer_count =0;

	audioStatus_typedef micStatus;
	micStatus = mic->getStatus();

	uint16_t *new_buffer;

	while (micStatus == recording) 
	{
		new_buffer = (uint16_t *)mic->getBuffer();

		if (new_buffer != NULL)
		{
			buffer_count++;
			
			msgStore->flushAudioSample((char *) new_buffer, // pointer to buffer
									   bufferSize * sizeof(uint16_t),	// total size of buffer
									   true // remove the right channel assuming 16 bits per sample
									   );
		}
		EMU_EnterEM1();
		micStatus = mic->getStatus();
	}

	new_buffer = (uint16_t *)mic->getBuffer();
	
	if(new_buffer)
	{
		printf("got last buffer! \n");
		msgStore->flushAudioSample((char *) new_buffer, bufferSize * sizeof(uint16_t));
	}
//	else
//		printf("no last buffer! \n");
	
	msgStore->endAudioSample();
	
//	printf("end acquiring audio sample \n");
}


/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
int main(void)
{
	CHIP_Init();
	TRACE_SWOSetup();
	
	SystemHFXOClockSet(48000000);   
  	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	CMU_ClockEnable(cmuClock_USART1, false);
	
	// turn off power to everything else
	// only needed for the PCB where we cannot cut the power to these sensors:
	GPIO_PinModeSet(GPIO_ANT_VCC, gpioModePushPull, 1);
	AccelerationSensor::getInstance()->setSleepState(true);
	TemperatureSensor::getInstance()->setSleepState(true);
	

	msgStore = MessageStorage::getInstance();
	msgStore->initialize("");
	
//	FATFS_speedTest(2);
	
	saveAudioSample(2);
	
	msgStore->deinitialize();
	printf("SD card now safe to remove! \n");

	// deinitialize used clocks, TODO this should be done in their respective classes
	TIMER_Enable(TIMER0, false);
	CMU_ClockEnable(cmuClock_TIMER0, false);
	
	while (1)
	{
         EMU_EnterEM2(true);
//		  printf("Woke up! \n");
	}

}
