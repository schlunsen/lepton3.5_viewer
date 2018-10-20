#include <stdio.h>
#include "Lepton_I2C.h"
#include "leptonSDKEmb32PUB/LEPTON_SDK.h"
#include "leptonSDKEmb32PUB/LEPTON_SYS.h"
#include "leptonSDKEmb32PUB/LEPTON_Types.h"
#include "leptonSDKEmb32PUB/LEPTON_AGC.h"
#include "leptonSDKEmb32PUB/LEPTON_OEM.h"
bool _connected;

LEP_CAMERA_PORT_DESC_T _port;
LEP_SYS_FPA_TEMPERATURE_KELVIN_T fpa_temp_kelvin;
LEP_RESULT result;

int lepton_connect() {
	int res = (int)LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);
	_connected = true;
	return res;
}

int lepton_temperature(){
	if(!_connected)
		lepton_connect();
	result = ((LEP_GetSysFpaTemperatureKelvin(&_port, &fpa_temp_kelvin)));
	printf("FPA temp kelvin: %i, code %i\n", fpa_temp_kelvin, result);
	return (fpa_temp_kelvin/100);
}


float raw2Celsius(float raw){
	float ambientTemperature = 25.0;
	float slope = 0.0217;
	return slope*raw+ambientTemperature-177.77;
}

void lepton_perform_ffc() {
	printf("performing FFC...\n");
	if(!_connected) {
		int res = lepton_connect();
		if (res != 0) {
			//check SDA and SCL lines if you get this error
			printf("I2C could not connect\n");
			printf("error code: %d\n", res);
		}
	}

	int res = (int)LEP_RunSysFFCNormalization(&_port);
	if (res != 0) {
		printf("FFC not successful\n");
		printf("error code: %d\n", res);
	} else {
		printf("FFC successful\n");
	}

}

void lepton_restart() {
	if(!_connected) {
		int res = lepton_connect();
		if (res != 0) {
			//check SDA and SCL lines if you get this error
			printf("I2C could not connect\n");
			printf("error code: %d\n", res);
		}
	}
	printf("restarting...\n");
	int res = (int)LEP_RunOemReboot(&_port);

	if(res != 0) {
		printf("restart unsuccessful with error: %d\n", res);
	} else {
		printf("restart successful!\n");
	}

}

void lepton_disable_agc() {
	if(!_connected) {
		int res = lepton_connect();
		if (res != 0) {
			//check SDA and SCL lines if you get this error
			printf("I2C could not connect\n");
			printf("error code: %d\n", res);
		}
	}
	printf("Disable AGC...\n");
	
	int res = (int)LEP_SetAgcEnableState(&_port, LEP_AGC_ENABLE);

	if(res != 0) {
		printf("Disable AGC unsuccessful with error: %d\n", res);
	} else {
		printf("Disable AGC successful!\n");
	}
}

