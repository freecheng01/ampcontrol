#include "audioproc.h"

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "../eeprom.h"
#include "../display/icons.h"

static const sndGrid grid[] PROGMEM = {
	{  0,  0, 0.00 * 8},	/* 0: Not implemented */
	{-79,  0, 1.00 * 8},	/* 1: -79..0dB with 1dB step */
	{ -7,  7, 2.00 * 8},	/* 2: -14..14dB with 2dB step */
	{-47,  0, 1.00 * 8},	/* 3: -47..0dB with 1dB step */
	{-21, 21, 1.00 * 8},	/* 4: -21..21dB with 1dB step */
	{  0, 15, 2.00 * 8},	/* 5: 0..30dB with 2dB step */

	{-63,  0, 1.25 * 8},	/* 6: -78.75..0dB with 1.25dB step*/
	{-15, 15, 1.25 * 8},	/* 7:  -18.75..18.75dB with 1.25dB step */
	{  0,  3, 3.75 * 8},	/* 8: 0..11.25dB with 3.75dB step */
	{  0,  3, 6.25 * 8},	/* 9: 0..18.75dB with 6.25dB step */
};

static audioProc proc = AUDIOPROC_TDA7439;
static sndParam sndPar[MODE_SND_END];

static uint8_t _inCnt;
static uint8_t _input;
static uint8_t _mute;
static uint8_t _loudness;

static void setNothing(int8_t val)
{
	return;
}

void sndInit(uint8_t **txtLabels)
{
	uint8_t i;

	/* Load audio parameters stored in eeprom */
	for (i = 0; i < MODE_SND_END; i++) {
		sndPar[i].value = eeprom_read_byte(eepromVolume + i);
		sndPar[i].label = txtLabels[MODE_SND_VOLUME + i];
	}
	_input = eeprom_read_byte(eepromChannel);
	_loudness = eeprom_read_byte(eepromLoudness);

	/* Init grid an functions with empty values */
	for (i = 0; i < MODE_SND_END; i++) {
		sndPar[i].grid = &grid[0];
		sndPar[i].set = setNothing;
	}

	/* Setup audio parameter grid and functions */
	switch (proc) {
	case AUDIOPROC_TDA7439:
		sndPar[MODE_SND_VOLUME].grid = &grid[1];
		sndPar[MODE_SND_BASS].grid = &grid[2];
		sndPar[MODE_SND_MIDDLE].grid = &grid[2];
		sndPar[MODE_SND_TREBLE].grid = &grid[2];
		sndPar[MODE_SND_PREAMP].grid = &grid[3];
		sndPar[MODE_SND_BALANCE].grid = &grid[4];
		sndPar[MODE_SND_GAIN0].grid = &grid[5];
		sndPar[MODE_SND_GAIN1].grid = &grid[5];
		sndPar[MODE_SND_GAIN2].grid = &grid[5];
		sndPar[MODE_SND_GAIN3].grid = &grid[5];
		_inCnt = 4;
		break;
	case AUDIOPROC_TDA7312:
		sndPar[MODE_SND_VOLUME].grid = &grid[6];
		sndPar[MODE_SND_BASS].grid = &grid[2];
		sndPar[MODE_SND_TREBLE].grid = &grid[2];
		sndPar[MODE_SND_BALANCE].grid = &grid[7];
		_inCnt = 4;
		break;
	case AUDIOPROC_TDA7313:
		sndPar[MODE_SND_VOLUME].grid = &grid[6];
		sndPar[MODE_SND_BASS].grid = &grid[2];
		sndPar[MODE_SND_TREBLE].grid = &grid[2];
		sndPar[MODE_SND_FRONTREAR].grid = &grid[7];
		sndPar[MODE_SND_BALANCE].grid = &grid[7];
		sndPar[MODE_SND_GAIN0].grid = &grid[8];
		sndPar[MODE_SND_GAIN1].grid = &grid[8];
		sndPar[MODE_SND_GAIN2].grid = &grid[8];
		_inCnt = 3;
		break;
	case AUDIOPROC_TDA7314:
		sndPar[MODE_SND_VOLUME].grid = &grid[6];
		sndPar[MODE_SND_BASS].grid = &grid[2];
		sndPar[MODE_SND_TREBLE].grid = &grid[2];
		sndPar[MODE_SND_FRONTREAR].grid = &grid[7];
		sndPar[MODE_SND_BALANCE].grid = &grid[7];
		sndPar[MODE_SND_GAIN0].grid = &grid[9];
		_inCnt = 1;
		break;
	case AUDIOPROC_TDA7318:
		sndPar[MODE_SND_VOLUME].grid = &grid[6];
		sndPar[MODE_SND_BASS].grid = &grid[2];
		sndPar[MODE_SND_TREBLE].grid = &grid[2];
		sndPar[MODE_SND_FRONTREAR].grid = &grid[7];
		sndPar[MODE_SND_BALANCE].grid = &grid[7];
		sndPar[MODE_SND_GAIN0].grid = &grid[9];
		sndPar[MODE_SND_GAIN1].grid = &grid[9];
		sndPar[MODE_SND_GAIN2].grid = &grid[9];
		sndPar[MODE_SND_GAIN3].grid = &grid[9];
		_inCnt = 4;
		break;
	}

	/* Setup icons for audio parameters */
	sndPar[MODE_SND_VOLUME].icon = icons_24_volume;
	sndPar[MODE_SND_BASS].icon = icons_24_bass;
	sndPar[MODE_SND_MIDDLE].icon = icons_24_middle;
	sndPar[MODE_SND_TREBLE].icon = icons_24_treble;
	sndPar[MODE_SND_PREAMP].icon = icons_24_preamp;
	sndPar[MODE_SND_FRONTREAR].icon = icons_24_frontrear;
	sndPar[MODE_SND_BALANCE].icon = icons_24_balance;
	sndPar[MODE_SND_GAIN0].icon = icons_24_tuner;
	sndPar[MODE_SND_GAIN1].icon = icons_24_pc;
	sndPar[MODE_SND_GAIN2].icon = icons_24_tv;
	sndPar[MODE_SND_GAIN3].icon = icons_24_dvd;

	return;
}

sndParam *sndParAddr(uint8_t index)
{
	return &sndPar[index];
}

uint8_t sndInputCnt(void)
{
	return _inCnt;
}

void sndSetInput(uint8_t input)
{
	if (input >= _inCnt)
		input = 0;
	_input = input;

	return;
}

uint8_t sndGetInput(void)
{
	return _input;
}


void sndSetMute(uint8_t value)
{
	if (value == MUTE_ON) {
//		setVolume((int8_t)pgm_read_byte(&sndPar[MODE_SND_VOLUME].grid->min));
		_mute = MUTE_ON;
		PORT(STMU_MUTE) &= ~STMU_MUTE_LINE;
	} else {
//		setVolume(sndPar[MODE_SND_VOLUME].value);
		_mute = MUTE_OFF;
		PORT(STMU_MUTE) |= STMU_MUTE_LINE;
	}

	return;
}

uint8_t sndGetMute(void)
{
	return _mute;
}

void sndSetLoudness(uint8_t value)
{
	if (value == LOUDNESS_ON)
		_loudness = LOUDNESS_ON;
	else
		_loudness = LOUDNESS_OFF;

	return;
}

uint8_t sndGetLoudness(void)
{
	return _loudness;
}

void sndChangeParam(uint8_t dispMode, int8_t diff)
{
	sndParam *param = sndParAddr(dispMode);
	param->value += diff;
	if (param->value > (int8_t)pgm_read_byte(&param->grid->max))
		param->value = (int8_t)pgm_read_byte(&param->grid->max);
	if (param->value < (int8_t)pgm_read_byte(&param->grid->min))
		param->value = (int8_t)pgm_read_byte(&param->grid->min);
	param->set(param->value);

	return;
}

void sndPowerOn(void)
{
	sndSetMute(MUTE_OFF);
	sndSetInput(_input);
//	setBass(sndPar[MODE_SND_BASS].value);
//	setPreamp(sndPar[MODE_SND_PREAMP].value);
//	setMiddle(sndPar[MODE_SND_MIDDLE].value);
//	setTreble(sndPar[MODE_SND_TREBLE].value);
	sndSetMute(MUTE_OFF);

	return;
}

void sndPowerOff(void)
{
	uint8_t i;

	for (i = 0; i < MODE_SND_END; i++)
		eeprom_update_byte(eepromVolume + i, sndPar[i].value);

	eeprom_update_byte(eepromChannel, _input);
	eeprom_update_byte(eepromLoudness, _loudness);

	return;
}