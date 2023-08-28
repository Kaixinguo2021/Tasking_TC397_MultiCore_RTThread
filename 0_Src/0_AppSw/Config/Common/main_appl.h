/*
 * main_appl.h
 *
 *  Created on: 02.02.2013
 *      Author: voigtlae
 */

#ifndef MAIN_APPL_H_
#define MAIN_APPL_H_

//a structure to measure the runtime and to handle sdcard
typedef struct CONTROLMENU
{
    float32 cpuseconds;
    float32 cpusecondsdelta;
} TCONTROLMENU;

void graph_drawInfineonLogo(void);

#endif /* MAIN_APPL_H_ */
