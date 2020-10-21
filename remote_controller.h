/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file remote_controller.h
 *
 * \brief
 * Header of the module for remote controller events.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#ifndef _REMOTE_CONTROLLER_H_
#define _REMOTE_CONTROLLER_H_

#include "stream_controller.h"

typedef enum _remoteControllerStatus
{
    REMOTE_CONTROLLER_NO_ERROR = 0,
    REMOTE_CONTROLLER_ERROR
} remoteControllerStatus;

/****************************************************************************
 * @brief    Function for remote controller initialization.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
remoteControllerStatus remoteControllerInit();

/****************************************************************************
 * @brief    Function for executing functions on corresponding key press event.
****************************************************************************/
void *remoteControllerEvent();

#endif
