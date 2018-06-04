/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file timer_controller.h
 *
 * \brief
 * Header of the module for timers.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#ifndef _TIMER_CONTROLLER_H_
#define _TIMER_CONTROLLER_H_

#include <time.h>

/****************************************************************************
 * @brief    Function for timer initialization.
 *
 * @param    timerId - [in] Pointer to timer variable.
 *           triggerSec - [in] Number of seconds after which timer should trigger.
 *           callback - [in] Pointer to function that should execute on timer trigger.
****************************************************************************/
void timerSetAndStart(timer_t *timerId, time_t triggerSec, void *callback);

/****************************************************************************
 * @brief    Function for timer deinitialization.
 *
 * @param    timerId - [in] Pointer to timer variable.
****************************************************************************/
void timerStopAndDelete(timer_t *timerId);

#endif