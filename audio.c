#include "audio.h"

#include <avr/eeprom.h>
#include <util/delay.h>

#include "ks0108.h"
#include "i2c.h"
#include "eeprom.h"
#include "input.h"

sndParam *params[10] = {
	&volume,
	&bass,
	&middle,
	&treble,
	&preamp,
	&balance,
	&gain[0],
	&gain[1],
	&gain[2],
	&gain[3],
};


void showParLabel(const uint8_t *parLabel)
{
	gdLoadFont(font_ks0066_ru_24, 1);
	gdSetXY(0, 0);
	gdWriteStringEeprom(parLabel);
	gdLoadFont(font_ks0066_ru_08, 1);
	gdSetXY(116, 7);
	gdWriteStringEeprom(dbLabel);
}

void showParValue(int8_t value)
{
	gdLoadFont(font_ks0066_ru_24, 1);
	gdSetXY(93, 4);
	gdWriteString(mkNumString(value, 3, ' ', 10));
	gdLoadFont(font_ks0066_ru_08, 1);
}

void showBoolParam(uint8_t value, const uint8_t *parLabel)
{
	gdLoadFont(font_ks0066_ru_24, 1);
	gdSetXY(0, 0);
	gdWriteStringEeprom(parLabel);
	gdSetXY(0, 4);
	if (value)
		gdWriteStringEeprom(onLabel);
	else
		gdWriteStringEeprom(offLabel);
	gdLoadFont(font_ks0066_ru_08, 1);
}

void showBar(int8_t min, int8_t max, int8_t value)
{
	uint8_t i, j, data;

	if (min + max) {
		value = (int16_t)85 * (value - min) / (max - min);
	} else {
		value = (int16_t)42 * value / max;
	}
	for (j = 5; j <= 6; j++) {
		gdSetXY(0, j);
		for (i = 0; i < 85; i++) {
			if (((min + max) && (value <= i)) || (!(min + max) &&
				(((value > 0) && ((i < 42) || (value + 42 < i))) ||
				((value <= 0) && ((i > 42) || (value + 42 > i)))))) {
				if (j == 5) {
					data = 0x80;
				} else {
					data = 0x01;
				}
			} else {
				data = 0xFF;
			}
			if (i & 0x01) {
				data = 0x00;
			}
			gdWriteData(data);
		}
	}
}

void showParam(sndParam *param)
{
	uint8_t mult = 8;

	if (audioProc == TDA7313_IC || audioProc == TDA7318_IC) {
		if (param->label == volumeLabel
		 || param->label == preampLabel
		 || param->label == balanceLabel)
		{
			mult = 10;
		}
		if (param->label == gainLabel0
		 || param->label == gainLabel1
		 || param->label == gainLabel2
		 || param->label == gainLabel3)
		{
			mult = 15;
		}
	}
	showBar(param->min, param->max, param->value);
	showParValue(((int16_t)(param->value) * param->step * mult + 4) >> 3);
	showParLabel(param->label);
}

void setVolume(int8_t val)
{
	int8_t spFrontLeft = 0;
	int8_t spFrontRight = 0;
	int8_t spRearLeft = 0;
	int8_t spRearRight = 0;

	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		I2CWrComm(TDA7313_ADDR, TDA7313_VOLUME | -val);
		if (balance.value > 0) {
			spFrontRight -= balance.value;
			spRearRight -= balance.value;
		} else {
			spFrontLeft += balance.value;
			spRearLeft += balance.value;
		}
		if (preamp.value > 0) {
			spRearLeft -= preamp.value;
			spRearRight -= preamp.value;
		} else {
			spFrontLeft += preamp.value;
			spFrontRight += preamp.value;
		}
		I2CWrComm(TDA7313_ADDR, TDA7313_SP_FRONT_LEFT | -spFrontLeft);
		I2CWrComm(TDA7313_ADDR, TDA7313_SP_FRONT_RIGHT | -spFrontRight);
		I2CWrComm(TDA7313_ADDR, TDA7313_SP_REAR_LEFT | -spRearLeft);
		I2CWrComm(TDA7313_ADDR, TDA7313_SP_REAR_RIGHT | -spRearRight);
		break;
	default:
		spFrontLeft = val;
		spFrontRight = val;
		if (balance.value > 0) {
			spFrontLeft -= balance.value;
			if (spFrontLeft < volume.min)
				spFrontLeft = volume.min;
		} else {
			spFrontRight += balance.value;
			if (spFrontRight < volume.min)
				spFrontRight = volume.min;
		}
		I2CWrite(TDA7439_ADDR, TDA7439_VOLUME_LEFT, -spFrontLeft);
		I2CWrite(TDA7439_ADDR, TDA7439_VOLUME_RIGHT, -spFrontRight);
		break;
	}
}

void setPreamp(int8_t val) /* For TDA7313 used as balance front/rear */
{
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		setVolume(volume.value);
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_PREAMP, -val);
		break;
	}
}

int8_t setBMT(int8_t val)
{
	if (val > 0)
		return 15 - val;
	return 7 + val;
}

void setBass(int8_t val)
{
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		I2CWrComm(TDA7313_ADDR, TDA7313_BASS | setBMT(val));
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_BASS, setBMT(val));
		break;
	}
}

void setMiddle(int8_t val)
{
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_MIDDLE, setBMT(val));
		break;
	}
}

void setTreble(int8_t val)
{
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		I2CWrComm(TDA7313_ADDR, TDA7313_TREBLE | setBMT(val));
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_TREBLE, setBMT(val));
		break;
	}
}

void setSwitch(int8_t gain)
{
	I2CWrComm(TDA7313_ADDR, TDA7313_SW | (3 - gain) << 3 | loud << 2 | chan);
}

void setBacklight(int8_t backlight)
{
	if (backlight)
		GD_BACKLIGHT_PORT |= GD_BCKL;
	else
		GD_BACKLIGHT_PORT &= ~GD_BCKL;
}

void setGain(int8_t val)
{
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		setSwitch(val);
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_INPUT_GAIN, val);
		break;
	}
}

void setChan(uint8_t ch)
{
	chan = ch;
	setGain(gain[ch].value);
	switch (audioProc) {
	case TDA7313_IC:
	case TDA7318_IC:
		setSwitch(gain[chan].value);
		break;
	default:
		I2CWrite(TDA7439_ADDR, TDA7439_INPUT_SELECT, chanCnt - 1 - ch);
		break;
	}
}

void setBalance(int8_t val)
{
	setVolume(volume.value);
}

void muteVolume(void)
{
	setVolume(volume.min);
	mute = MUTE_ON;
}

void unmuteVolume(void)
{
	setVolume(volume.value);
	mute = MUTE_OFF;
}

void switchMute(void)
{
	if (mute == MUTE_ON) {
		unmuteVolume();
	} else {
		muteVolume();
	}
}

void switchLoudness(void)
{
	if (loud == LOUDNESS_ON)
		loud = LOUDNESS_OFF;
	else
		loud = LOUDNESS_ON;
	setSwitch(gain[chan].value);
}

void switchBacklight(void)
{
	if (backlight == BACKLIGHT_ON)
		backlight = BACKLIGHT_OFF;
	else
		backlight = BACKLIGHT_ON;
	setBacklight(backlight);
}

void loadParams(void)
{
	uint8_t i;

	for (i = 0; i < 10; i++) {
		params[i]->value = eeprom_read_byte(eepromVolume + i);
		params[i]->label = volumeLabel + 16 * i;
		params[i]->min = eeprom_read_byte(eepromMinimums + i);
		params[i]->max = eeprom_read_byte(eepromMaximums + i);
		params[i]->step = eeprom_read_byte(eepromSteps + i);
	}

	chan = eeprom_read_byte(eepromChannel);
	loud = eeprom_read_byte(eepromLoudness);
	chanCnt = eeprom_read_byte(eepromChanCnt);
	audioProc = eeprom_read_byte(eepromICSelect);
	backlight = eeprom_read_byte(eepromBCKL);

	volume.set = setVolume;
	bass.set = setBass;
	middle.set = setMiddle;
	treble.set = setTreble;
	balance.set = setBalance;
	preamp.set = setPreamp;

	for (i = 0; i < 4; i++) {
		gain[i].set = setGain;
	}

	setChan(chan);
	setPreamp(preamp.value);
	setBass(bass.value);
	setMiddle(middle.value);
	setTreble(treble.value);
}

void saveParams(void)
{
	uint8_t i;

	for (i = 0; i < 10; i++) {
		eeprom_write_byte(eepromVolume + i, params[i]->value);
	}
	eeprom_write_byte(eepromChannel, chan);
	eeprom_write_byte(eepromLoudness, loud);
	eeprom_write_byte(eepromChanCnt, chanCnt);
	eeprom_write_byte(eepromBCKL, backlight);
}

void changeParam(sndParam *param, int8_t diff)
{
	param->value += diff;
	if (param->value > param->max)
		param->value = param->max;
	if (param->value < param->min)
		param->value = param->min;
	param->set(param->value);
}

void nextChan(void)
{
	chan++;
	if (chan >= chanCnt)
		chan = 0;
	setChan(chan);
}