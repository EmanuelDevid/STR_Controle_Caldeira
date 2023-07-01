#ifndef SENSORES_H
#define SENSORES_H

void sensor_put(double temp, double nivel);
double sensor_get(char s[5]);
void sensor_alarmeT(double limite);

#endif



