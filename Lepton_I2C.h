#ifndef LEPTON_I2C
#define LEPTON_I2C

void lepton_perform_ffc();
int lepton_temperature();
float raw2Celsius(float);
void lepton_restart();
void lepton_disable_agc();
#endif
