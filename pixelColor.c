#include "pixelColor.h"

unsigned char stepsToRed (int steps){
	unsigned char intensity;
	if (steps == 255){
		intensity = 0;
	} else {
		intensity = steps*10;
	}
	return intensity;
}

unsigned char stepsToBlue (int steps){
	unsigned char intensity;
	if (steps == 255){
		intensity = 0;
	} else {
		intensity = steps*10;
	}
	return intensity;
}
unsigned char stepsToGreen (int steps){
	unsigned char intensity;
	if (steps == 255){
		intensity = 0;
	} else {
		intensity = steps*10;
	}
	return intensity;
}



