/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file graphics_controller.h
 *
 * \brief
 * Header of the module for handling graphics.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#ifndef _GRAPHICS_CONTROLLER_H_
#define _GRAPHICS_CONTROLLER_H_

#ifndef _TDP_API_H_
#define _TDP_API_H_

#include "tdp_api.h"

#endif // _TDP_API_H_

#define COLOUR_BLACK 0x00
#define COLOUR_WHITE 0xff

typedef enum _graphicsControllerStatus
{
    GRAPHICS_CONTROLLER_NO_ERROR = 0,
    GRAPHICS_CONTROLLER_ERROR
} graphicsControllerStatus;

/****************************************************************************
 * @brief    Function for DirectFB initialization.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus graphicsControllerInit();

/****************************************************************************
 * @brief    Function for DirectFB deinitialization.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus graphicsControllerDeinit();

/****************************************************************************
 * @brief    Function for drawing input channel number value.
 *
 * @param    channelNumberValue - [in] Entered channel number to draw.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawChannelNumber(uint16_t channelNumberValue);

/****************************************************************************
 * @brief    Function for drawing input channel number value.
 *
 * @param    channelNumberValue - [in] Entered channel number for which message is drawn.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawChannelNumberMessage(uint16_t channelNumberValue);

/****************************************************************************
 * @brief    Function for drawing channel information banner.
 *
 * @param    channelNumberValue - [in] Channel number to draw.
 *           subtitleCount - [in] Channel number of subtitles.
 *           subtitles - [in] String with characters representing subtitle languages.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawChannelInfo(uint16_t channelNumberValue, uint8_t subtitleCount, char *subtitles);

/****************************************************************************
 * @brief    Function for drawing volume information banner.
 *
 * @param    volumePercent - [in] Channel volume percentage number to draw.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawVolumeInfo(float volumePercent);

/****************************************************************************
 * @brief    Function for drawing menu information banner.
 *
 * @param    presentShowStartTime - [in] Present show start time value.
 *           presentShowDuration - [in] Present show duration time value.
 *           presentShowName - [in] Present show name string.
 *           presentShowDescription - [in] Present show description string.
 *
 *           followingShowStartTime - [in] Following show start time value.
 *           followingShowDuration - [in] Following show duration time value.
 *           followingShowName - [in] Following show name string.
 *           followingShowDescription - [in]  Following show description string.
 *
 *           channelFlag - [in] Value that signals which channel menu should be drawn.
 *                              0 Removes menu banner.
 *                              1 Draws present show menu banner.
 *                              2 Draws following show menu banner.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawMenuInfo(uint32_t presentShowStartTime, uint32_t presentShowDuration, char *presentShowName, char *presentShowDescription,
                                      uint32_t followingShowStartTime, uint32_t followingShowDuration, char *followingShowName, char *followingShowDescription,
                                      uint8_t channelFlag);

/****************************************************************************
 * @brief    Function for showing drawn graphics to screen.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus drawOnScreen();

/****************************************************************************
 * @brief    Function for removing graphics from screen. Black rectangle with passed transparency is drawn.
 *
 * @param    alpha - [in] Transparency colour value.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
graphicsControllerStatus clearScreen(uint8_t alpha);

#endif // _GRAPHICS_CONTROLLER_H_
