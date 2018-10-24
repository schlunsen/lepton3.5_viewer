/*
*
* CREATE - Functions to create and display the thermal frameBuffer
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

#ifndef CREATE_H
#define CREATE_H

/*########################## PUBLIC PROCEDURES ################################*/

void boxFilter();
void calcFillPixel(uint16_t x, uint16_t y);
void calculatePointPos(int16_t* xpos, int16_t* ypos, uint16_t pixelIndex);
void clearTempPoints();
void convertColors(bool small = false);
void createThermalImg(bool small = false);
void createVisCombImg();
void fillEdges();
void gaussianFilter();
void getHotColdColors(byte* red, byte* green, byte* blue);
void getTouchPos(uint16_t* x, uint16_t* y);
void limitValues();
void refreshMinMax();
void refreshTempPoints();
void resizeImage();
void resizePixels(unsigned short* pixels, int w1, int h1, int w2, int h2);
void showTemperatures();
void smallToBigBuffer(bool trans = false);
void tempPointFunction(bool remove = false);

#endif /* CREATE_H */
