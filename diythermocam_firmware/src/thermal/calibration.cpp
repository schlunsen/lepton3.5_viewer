/*
 *
 * CALIBRATION - Functions to convert Lepton raw values to absolute values
 *
 * DIY-Thermocam Firmware
 *
 * GNU General Public License v3.0
 *
 * Copyright by Max Ritter
 *
 * http://www.diy-thermocam.net
 * https://github.com/maxritter/DIY-Thermocam
 *
 */

/*################################# INCLUDES ##################################*/

#include <Arduino.h>
#include <globaldefines.h>
#include <globalvariables.h>
#include <mlx90614.h>
#include <lepton.h>
#include <hardware.h>
#include <connection.h>
#include <mainmenu.h>
#include <display.h>
#include <touchscreen.h>
#include <buttons.h>
#include <gui.h>
#include <fonts.h>
#include <calibration.h>

/*###################### PRIVATE FUNCTION BODIES ##############################*/

/* Help function for least suqare fit */
inline static double sqr(double x) {
	return x*x;
}

/*######################## PUBLIC FUNCTION BODIES #############################*/

/* Converts a given Temperature in Celcius to Fahrenheit */
float celciusToFahrenheit(float Tc) {
	float Tf = ((float) 9.0 / 5.0) * Tc + 32.0;
	return (Tf);
}

/* Converts a given temperature in Fahrenheit to Celcius */
float fahrenheitToCelcius(float Tf) {
	float Tc = (Tf - 32.0) * ((float) 5.0 / 9.0);
	return Tc;
}

/* Function to calculate temperature out of Lepton value */
float calFunction(uint16_t rawValue) {
	//For radiometric Lepton, set offset to fixed value
	if ((leptonVersion == leptonVersion_2_5_shutter) || (leptonVersion == leptonVersion_3_5_shutter))
		calOffset = -273.15;

	//For non-radiometric Lepton
	else {
		//Refresh MLX90614 ambient temp
		ambTemp = mlx90614_getAmb();

		//Calculate offset out of ambient temp and compensation factor
		calOffset = ambTemp - (calSlope * 8192) + calComp;
	}

	//Calculate the temperature in Celcius out of the coefficients
	float temp = (calSlope * rawValue) + calOffset;

	//Convert to Fahrenheit if needed
	if (tempFormat == tempFormat_fahrenheit)
		temp = celciusToFahrenheit(temp);
	return temp;
}

/* Calculate the lepton value out of an absolute temperature */
uint16_t tempToRaw(float temp) {
	//Convert to Celcius if needed
	if (tempFormat == tempFormat_fahrenheit)
		temp = fahrenheitToCelcius(temp);

	//For radiometric Lepton, set offset to fixed value
	if ((leptonVersion == leptonVersion_2_5_shutter) || (leptonVersion == leptonVersion_3_5_shutter))
		calOffset = -273.15;

	//For non-radiometric Lepton
	else {
		//Refresh MLX90614 ambient temp
		ambTemp = mlx90614_getAmb();

		//Calculate offset out of ambient temp and compensation factor
		calOffset = ambTemp - (calSlope * 8192) + calComp;
	}

	//Calcualte the raw value
	uint16_t rawValue = (temp - calOffset) / calSlope;
	return rawValue;
}

/* Calculates the average of the 196 (14x14) pixels in the middle */
uint16_t calcAverage() {
	int sum = 0;
	uint16_t val;
	for (byte vert = 52; vert < 66; vert++) {
		for (byte horiz = 72; horiz < 86; horiz++) {
			val = smallBuffer[(vert * 160) + horiz];
			sum += val;
		}
	}
	uint16_t avg = (uint16_t)(sum / 196.0);
	return avg;
}

/* Compensate the calibration with MLX90614 values */
void compensateCalib() {
	//Calculate compensation if auto mode enabled and no limits locked
	if ((autoMode) && (!limitsLocked)) {
		//Calculate min & max
		int16_t min = round(calFunction(minValue));
		int16_t max = round(calFunction(maxValue));

		//If spot temp is lower than current minimum by one degree, lower minimum
		if (spotTemp < (min - 1))
			calComp = spotTemp - min;

		//If spot temp is higher than current maximum by one degree, raise maximum
		else if (spotTemp > (max + 1))
			calComp = spotTemp - max;
	}
}

/* Checks if the calibration warmup is done */
void checkWarmup() {
	//Activate the calibration after a warmup time of 30s
	if (((calStatus == cal_warmup) && (millis() - calTimer > 30000))) {
		//Set calibration status to standard
		calStatus = cal_standard;

		//Disable auto FFC when isotherm mode
		if (hotColdMode != hotColdMode_disabled)
			lepton_ffcMode(false);

		//Read temperature limits
		readTempLimits();

		//If we loaded one
		if (!autoMode)
		{
			//Switch to manual FFC mode
			if (leptonShutter == leptonShutter_auto)
				lepton_ffcMode(false);

			//Disable limits locked
			limitsLocked = false;
		}
	}
}

/* Least square fit */
int linreg(int n, const uint16_t x[], const float y[], float* m, float* b, float* r)
{
	double   sumx = 0.0;
	double   sumx2 = 0.0;
	double   sumxy = 0.0;
	double   sumy = 0.0;
	double   sumy2 = 0.0;
	for (int i = 0; i < n; i++)
	{
		sumx += x[i];
		sumx2 += sqr(x[i]);
		sumxy += x[i] * y[i];
		sumy += y[i];
		sumy2 += sqr(y[i]);
	}
	double denom = (n * sumx2 - sqr(sumx));
	if (denom == 0) {
		//singular matrix. can't solve the problem.
		*m = 0;
		*b = 0;
		*r = 0;
		return 1;
	}
	*m = (n * sumxy - sumx * sumy) / denom;
	*b = (sumy * sumx2 - sumx * sumxy) / denom;
	if (r != NULL) {
		*r = (sumxy - sumx * sumy / n) /
				sqrt((sumx2 - sqr(sumx) / n) *
						(sumy2 - sqr(sumy) / n));
	}
	return 0;
}

/* Run the calibration process */
void calibrationProcess(bool serial, bool firstStart) {
	//When in serial mode and using radiometric Lepton, return invalid command
	if (serial && ((leptonVersion == leptonVersion_2_5_shutter) || (leptonVersion == leptonVersion_3_5_shutter))) {
		Serial.write(CMD_INVALID);
		return;
	}

	//Variables
	float calMLX90614[100];
	uint16_t calLepton[100];
	float calCorrelation;
	char result[30];
	uint16_t average;
	uint16_t average_old = 0;
	float temp;
	float temp_old = 0;
	maxValue = 0;
	minValue = 65535;

	//Repeat as long as there is a good calibration
	do {
		//Show the screen when not using serial mode
		if (!serial)
			calibrationScreen(firstStart);

		//Reset counter to zero
		int counter = 0;

		//Get 100 different calibration samples
		while (counter < 100) {
			//Store time elapsed
			long timeElapsed = millis();

			//Repeat measurement as long as there is no new average
			do {
				//Safe delay for bad PCB routing
				delay(10);
				//Get temperatures
				lepton_getRawValues();
				//Calculate the average
				average = calcAverage();
			} while ((average == average_old) || (average == 0));

			//Store old average
			average_old = average;

			//Measure new value from MLX90614
			temp = mlx90614_getTemp();

			//If the temperature changes too much, do not take that measurement
			if (abs(temp - temp_old) < 10) {
				//Store values
				calLepton[counter] = average;
				calMLX90614[counter] = temp;

				//Find minimum and maximum value
				if (average > maxValue)
					maxValue = average;
				if (average < minValue)
					minValue = average;

				//Refresh status on screen in steps of 10, not for serial
				if (((counter % 10) == 0) && !serial) {
					char buffer[20];
					sprintf(buffer, "Status: %2d%%", counter);
					display_print(buffer, CENTER, 140);
				}

				//When doing this in serial mode, print counter
				if (serial)
					Serial.write(counter);

				//Raise counter
				counter++;
			}

			//Store old spot temperature
			temp_old = temp;

			//Wait at least 111ms between two measurements (9Hz)
			while ((millis() - timeElapsed) < 111);

			//If the user wants to abort and is not in first start or serial mode
			if (touch_touched() && !firstStart && !serial) {
				int pressedButton = buttons_checkButtons(true);
				//Abort
				if (pressedButton == 0)
					return;
			}
		}

		//Calculate the calibration formula with least square fit
		linreg(100, calLepton, calMLX90614, &calSlope, &calOffset, &calCorrelation);

		//Refresh MLX90614 ambient temp
		ambTemp = mlx90614_getAmb();

		//Calculate compensation
		calComp = calOffset - ambTemp + (calSlope * 8192);

		//When in serial mode, store and send ACK
		if (serial)
		{
			//Set shutter mode back to auto
			lepton_ffcMode(true);

			//Save calibration to EEPROM
			storeCalibration();

			//Send ACK
			Serial.write(CMD_SET_CALIBRATION);

			//Leave
			return;
		}

		//In case the calibration was not good, ask to repeat
		if (calCorrelation < 0.5) {

			//When in first start mode
			if (firstStart) {
				showFullMessage((char*) "Bad calibration, try again", true);
				delay(1000);
			}

			//If the user does not want to repeat, discard
			else if (!calibrationRepeat()) {
				calSlope = cal_stdSlope;
				calStatus = cal_standard;
				break;
			}
		}
	} while (calCorrelation < 0.5);

	//Show the result
	sprintf(result, "Slope: %1.4f, offset: %.1f", calSlope, calOffset);
	showFullMessage(result);
	delay(2000);

	//Save calibration to EEPROM
	storeCalibration();

	//Show message if not in first start menu
	if (firstStart == false) {
		showFullMessage((char*) "Calibration written to EEPROM", true);
		delay(1000);
	}

	//Restore old font
	display_setFont(smallFont);
}

/* Calibration */
bool calibration() {
	//Still in warmup
	if (calStatus == cal_warmup) {
		showFullMessage((char*) "Please wait for sensor warmup", true);
		delay(1000);
		return false;
	}

	//Radiometric Lepton, no calibration needed
	if ((leptonVersion == leptonVersion_2_5_shutter) || (leptonVersion == leptonVersion_3_5_shutter)) {
		showFullMessage((char*) "Not required for radiometric Lepton", true);
		delay(1000);
		return false;
	}

	//Do a new one
	calibrationProcess();

	return true;
}
