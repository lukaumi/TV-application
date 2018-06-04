/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file stream_controller.h
 *
 * \brief
 * Header of the module for handling tuner, player and channels.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#ifndef _STREAM_CONTROLLER_H_
#define _STREAM_CONTROLLER_H_

#include "configuration_parser.h"

typedef enum _streamControllerStatus {
    STREAM_CONTROLLER_NO_ERROR = 0,
    STREAM_CONTROLLER_ERROR
} streamControllerStatus;

#define ASSERT_TDP_RESULT(x,y)            \
{                                         \
    if (STREAM_CONTROLLER_NO_ERROR == x)   \
        printf("%s success\n", y);        \
    else {                                \
        textColor(1,1,0);                 \
        printf("%s fail\n", y);           \
        textColor(0,7,0);                 \
        return STREAM_CONTROLLER_ERROR;   \
    }                                     \
}

typedef struct _channelData {
    uint16_t pmtProgramNumber;

    startingChannelInit channelInit;

    uint32_t presentShowStartTime;
    uint32_t presentShowDuration;
    char *presentShowName;
    char *presentShowDescription;

    uint32_t followingShowStartTime;
    uint32_t followingShowDuration;
    char *followingShowName;
    char *followingShowDescription;

    uint8_t subtitleCount;
    char *subtitles;
} channelData;

typedef struct _channels {
    channelData *channel;
    uint32_t channelCount;
} Channels;

typedef enum _dvbStreamType {
    dvbVideo      = 0x01,
    dvbVideoMPEG2 = 0x02,
    dvbAudioMPEG  = 0x03
} dvbStreamType;

/****************************************************************************
 * @brief    Function for tuner and player initialization.
 *
 * @param    config - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus streamControllerInit(initialConfig *config);

/****************************************************************************
 * @brief    Function for tuner and player deinitialization.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus streamControllerDeinit();

/****************************************************************************
 * @brief    Function for creating player stream and setting volume.
 *
 * @param    config - [in] Pointer to structure variable in which channel parameters are stored.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus startPlayerStream(startingChannelInit *channel);

/****************************************************************************
 * @brief    Function for removing player stream.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus stopPlayerStream();

/****************************************************************************
 * @brief    Function for setting up channels based on information from PAT, PMT and EIT tables.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
void *channelsSetup();

/****************************************************************************
 * @brief    Function for starting player stream.
 *
 * @param    channelNumber - [in] Number of channel to play.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus playChannel(uint16_t channelNumber);

/****************************************************************************
 * @brief    Function for starting player stream for next channel.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus playNextChannel();

/****************************************************************************
 * @brief    Function for starting player stream for previous channel.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus playPreviousChannel();

/****************************************************************************
 * @brief    Function for muting or unmuting volume.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus volumeMute();

/****************************************************************************
 * @brief    Function for increasing volume.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus volumeUp();

/****************************************************************************
 * @brief    Function for decreasing volume.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus volumeDown();

/****************************************************************************
 * @brief    Function for calling functions for drawing and showing channel information banner.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus showChannelInfo();

/****************************************************************************
 * @brief    Function for calling functions for drawing and showing channel volume banner.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus showVolumeInfo();

/****************************************************************************
 * @brief    Function for calling functions for drawing and showing channel menu banner.
 *
 * @param    channelFlag - [in] Value that signals which channel menu should be drawn.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus showMenuInfo(uint8_t channelFlag);

/****************************************************************************
 * @brief    Function for calling functions for drawing and showing input channel number value.
 *
 * @param    channelNumberValue - [in] Entered channel number to draw.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus showChannelNumber(uint16_t channelNumberValue);

/****************************************************************************
 * @brief    Function for calling functions for drawing and showing input channel number message.
 *
 * @param    channelNumberValue - [in] Entered channel number to draw.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
streamControllerStatus showChannelNumberMessage(uint16_t channelNumberValue);

#endif