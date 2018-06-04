/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file stream_controller.c
 *
 * \brief
 * Implementation of the module for handling tuner, player and channels.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#include "stream_controller.h"

#include "tables_parser.h"
#include "graphics_controller.h"

#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include "errno.h"

/* helper keywords needed only for stream controller module */
#define PAT_ID 0x00
#define PAT_PID 0x00

#define PMT_ID 0x02

#define EIT_ID 0x4E
#define EIT_PID 0x0012

#define VOLUME_MAX  INT_MAX
#define VOLUME_MIN  0
#define VOLUME_STEP 0.05 // increase volume by 5%

#define CHANNEL_RUNNING_STATUS 4

/* helper variables needed only for stream controller module */
static uint32_t playerHandle;
static uint32_t sourceHandle;
static uint32_t filterHandle;
static uint32_t videoHandle;
static uint32_t audioHandle;

static pthread_cond_t statusCondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t statusMutex = PTHREAD_MUTEX_INITIALIZER;

static patTable *pat;
static Channels channels;
static uint16_t channelCounter;
static uint16_t currentChannel;
static uint32_t currentVolume;
static uint8_t  volumeMuted;

/* helper functions needed only for stream controller module */
static streamControllerStatus setFilterAndRegister(uint32_t tableId, uint32_t tablePid);
static streamControllerStatus freeFilter(int32_t (*callback)(uint8_t* buffer));
static void pmtSaveChannel(pmtTable *pmt);
static void eitSaveChannel(eitTable *eit);
static streamControllerStatus streamTypeDVBtoTDP(uint32_t dvbStreamType);
static streamControllerStatus timedWaitForCondition(uint8_t seconds);
static streamControllerStatus threadMutexUnlock();

/* callback functions needed only for stream controller module */
static int32_t tunerStatusCallback(t_LockStatus status);
static int32_t patCallback(uint8_t *buffer);
static int32_t pmtCallback(uint8_t *buffer);
static int32_t eitCallback(uint8_t *buffer);

streamControllerStatus streamControllerInit(initialConfig *config)
{
    uint8_t result;

    /* Initialize tuner */
    result = Tuner_Init();
    ASSERT_TDP_RESULT(result, "streamControllerInit: Tuner_Init");

    /* Register tuner status callback */
    result = Tuner_Register_Status_Callback(tunerStatusCallback);
    ASSERT_TDP_RESULT(result, "streamControllerInit: Tuner_Register_Status_Callback");

    /* Lock to frequency */
    result = Tuner_Lock_To_Frequency(config->transponder.frequency * 1000000, config->transponder.bandwidth, config->transponder.module);
    ASSERT_TDP_RESULT(result, "streamControllerInit: Tuner_Lock_To_Frequency");

    /* set 10 second mutex lock until tunex is locked to frequency */
    timedWaitForCondition(10);

    /* Initialize player (demux is a part of player) */
    result = Player_Init(&playerHandle);
    ASSERT_TDP_RESULT(result, "streamControllerInit: Player_Init");

    /* Open source (open data flow between tuner and demux) */
    result = Player_Source_Open(playerHandle, &sourceHandle);
    ASSERT_TDP_RESULT(result, "streamControllerInit: Player_Source_Open");

    /* Get initial volume */
    result = Player_Volume_Get(playerHandle, &currentVolume);
    ASSERT_TDP_RESULT(result, "streamControllerInit: Player_Volume_Get");

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus streamControllerDeinit()
{
    uint8_t result;

    stopPlayerStream();

    /* Close previously opened source */
    result = Player_Source_Close(playerHandle, sourceHandle);
    ASSERT_TDP_RESULT(result, "streamControllerDeinit: Player_Source_Close");

    /* Deinit player */
    result = Player_Deinit(playerHandle);
    ASSERT_TDP_RESULT(result, "streamControllerDeinit: Player_Deinit");

    /* Deinit tuner */
    result = Tuner_Deinit();
    ASSERT_TDP_RESULT(result, "streamControllerDeinit: Tuner_Deinit");

    /* Free channels memory */
    free((channels.channel)->subtitles);
    free(channels.channel);

    (channels.channel)->subtitles = NULL;
    channels.channel = NULL;

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus startPlayerStream(startingChannelInit *channel)
{
    uint8_t result;

    stopPlayerStream();

    if (channel->videoPID != CONFIGURATION_PARSER_NOT_SET && channel->videoType != CONFIGURATION_PARSER_NOT_SET) {
        result = Player_Stream_Create(playerHandle, sourceHandle, channel->videoPID, channel->videoType, &videoHandle);
        ASSERT_TDP_RESULT(result, "startPlayerStream: Video Player_Stream_Create");
    }

    if (channel->audioPID != CONFIGURATION_PARSER_NOT_SET && channel->audioType != CONFIGURATION_PARSER_NOT_SET) {
        result = Player_Stream_Create(playerHandle, sourceHandle, channel->audioPID, channel->audioType, &audioHandle);
        ASSERT_TDP_RESULT(result, "startPlayerStream: Audio Player_Stream_Create");
    }

    if (!volumeMuted) {
        result = Player_Volume_Set(playerHandle, currentVolume);
        ASSERT_TDP_RESULT(result, "startPlayerStream: Player_Volume_Set");
    }

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus stopPlayerStream()
{
    uint8_t result;

    if (videoHandle) {
        result = Player_Stream_Remove(playerHandle, sourceHandle, videoHandle);
        ASSERT_TDP_RESULT(result, "stopPlayerStream: Video Player_Stream_Remove");
        videoHandle = 0;
    }

    if (audioHandle) {
        result = Player_Stream_Remove(playerHandle, sourceHandle, audioHandle);
        ASSERT_TDP_RESULT(result, "stopPlayerStream: Audio Player_Stream_Remove");
        audioHandle = 0;
    }

    return STREAM_CONTROLLER_NO_ERROR;
}

void *channelsSetup()
{
    uint8_t result;

    /* PMT table parsing setup */
    result = setFilterAndRegister(PAT_ID, PAT_PID);
    /* Wait for PAT table */
    timedWaitForCondition(3);

    channels.channelCount = pat->programCount;
    channels.channel = (channelData*) malloc(channels.channelCount * sizeof(channelData));

    int32_t i;
    /* PMT table parsing setup */
    for (i = 0; i < pat->sectionCount; i++) {
        if (pat->programInformation[i].programNumber) {
            result = setFilterAndRegister(PMT_ID, pat->programInformation[i].programMapPid);
            /* Wait for PMT table */
            timedWaitForCondition(3);
        }
    }

    free(pat->programInformation);
    free(pat);

    pat->programInformation = NULL;
    pat = NULL;

    /* EIT table parsing setup */
    if (filterHandle) {
        ASSERT_TDP_RESULT(STREAM_CONTROLLER_ERROR, "channelsSetup: EIT Filter already set.");
    }

    result = Demux_Set_Filter(playerHandle, EIT_PID, EIT_ID, &filterHandle);
    ASSERT_TDP_RESULT(result, "channelsSetup: EIT Demux_Set_Filter ");

    result = Demux_Register_Section_Filter_Callback(eitCallback);
    ASSERT_TDP_RESULT(result, "channelsSetup: EIT Demux_Register_Section_Filter_Callback");

    /* Wait for EIT table */
    timedWaitForCondition(3);

    return (void*) STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus playChannel(uint16_t channelNumber)
{
    int8_t result;

    if (channelNumber > channels.channelCount || channelNumber < 1) {
        showChannelNumberMessage(channelNumber);
        return STREAM_CONTROLLER_ERROR;
    }

    currentChannel = channelNumber - 1;
    result = startPlayerStream(&channels.channel[currentChannel].channelInit);
    ASSERT_TDP_RESULT(result, "playChannel: startPlayerStream");

    showChannelInfo();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus playNextChannel()
{
    uint8_t result;

    if (currentChannel == channels.channelCount - 1) {
        currentChannel = 0;
    }
    else {
        currentChannel++;
    }

    result = startPlayerStream(&channels.channel[currentChannel].channelInit);
    ASSERT_TDP_RESULT(result, "playNextChannel: startPlayerStream");

    showChannelInfo();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus playPreviousChannel()
{
    uint8_t result;

    if (currentChannel == 0) {
        currentChannel = channels.channelCount - 1;
    }
    else {
        currentChannel--;
    }

    result = startPlayerStream(&channels.channel[currentChannel].channelInit);
    ASSERT_TDP_RESULT(result, "playPreviousChannel: startPlayerStream");

    showChannelInfo();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus volumeMute()
{
    uint8_t result;

    if (volumeMuted) {
        result = Player_Volume_Set(playerHandle, currentVolume);
        ASSERT_TDP_RESULT(result, "volumeMute: Player_Volume_Set");
    } else {
        result = Player_Volume_Set(playerHandle, VOLUME_MIN);
        ASSERT_TDP_RESULT(result, "volumeMute: Player_Volume_Set");
    }

    volumeMuted = !volumeMuted;

    showVolumeInfo();
    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus volumeUp()
{
    uint8_t result;

    if (!volumeMuted) {
        result = Player_Volume_Get(playerHandle, &currentVolume);
        ASSERT_TDP_RESULT(result, "volumeUp: Player_Volume_Get");

        currentVolume += VOLUME_MAX * VOLUME_STEP;
        if (currentVolume >= VOLUME_MAX) {
            currentVolume = VOLUME_MAX;
        }
    }

    volumeMuted = 0;
    result = Player_Volume_Set(playerHandle, currentVolume);
    ASSERT_TDP_RESULT(result, "volumeUp: Player_Volume_Set");

    showVolumeInfo();
    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus volumeDown()
{
    uint8_t result;

    if (!volumeMuted) {
        result = Player_Volume_Get(playerHandle, &currentVolume);
        ASSERT_TDP_RESULT(result, "volumeDown: Player_Volume_Get");

        currentVolume -= VOLUME_MAX * VOLUME_STEP;

        if (currentVolume <= VOLUME_MIN) {
            currentVolume = VOLUME_MIN;
        }
    }

    volumeMuted = 0;
    result = Player_Volume_Set(playerHandle, currentVolume);
    ASSERT_TDP_RESULT(result, "volumeDown: Player_Volume_Set");

    showVolumeInfo();
    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus showChannelInfo()
{
    uint8_t result;

    result = drawChannelInfo(currentChannel + 1, channels.channel[currentChannel].subtitleCount, channels.channel[currentChannel].subtitles);
    ASSERT_TDP_RESULT(result, "showChannelInfo: drawChannelInfo");

    drawOnScreen();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus showVolumeInfo()
{
    uint8_t result;

    float volumePercent;

    if (volumeMuted) volumePercent = 0.0;
    else volumePercent = (float) currentVolume / VOLUME_MAX;

    result = drawVolumeInfo(volumePercent);
    ASSERT_TDP_RESULT(result, "showVolumeInfo: drawVolumeInfo");

    drawOnScreen();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus showMenuInfo(uint8_t channelFlag)
{
    uint8_t result;

    result = drawMenuInfo(channels.channel[currentChannel].presentShowStartTime, channels.channel[currentChannel].presentShowDuration,
                          channels.channel[currentChannel].presentShowName, channels.channel[currentChannel].presentShowDescription,
                          channels.channel[currentChannel].followingShowStartTime, channels.channel[currentChannel].followingShowDuration,
                          channels.channel[currentChannel].followingShowName, channels.channel[currentChannel].followingShowDescription,
                          channelFlag);
    ASSERT_TDP_RESULT(result, "showMenuInfo: drawMenuInfo");

    drawOnScreen();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus showChannelNumber(uint16_t channelNumberValue)
{
    uint8_t result;

    result = drawChannelNumber(channelNumberValue);
    ASSERT_TDP_RESULT(result, "showChannelNumber: drawChannelNumber");

    drawOnScreen();

    return STREAM_CONTROLLER_NO_ERROR;
}

streamControllerStatus showChannelNumberMessage(uint16_t channelNumberValue)
{
    clearScreen(COLOUR_BLACK);
    drawOnScreen();
    clearScreen(COLOUR_BLACK);

    uint8_t result;

    result = drawChannelNumberMessage(channelNumberValue);
    ASSERT_TDP_RESULT(result, "showChannelNumberMessage: drawChannelNumberMessage");

    drawOnScreen();

    return STREAM_CONTROLLER_NO_ERROR;
}

/* -------------------- HELPER FUNCTIONS -------------------- */
/****************************************************************************
 * @brief    Function for setting demux filter and registering corresponding callback.
 *
 * @param    tableId - [in] Table ID value.
 *           tablePid - [in] Table PID value.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static streamControllerStatus setFilterAndRegister(uint32_t tableId, uint32_t tablePid)
{
    uint8_t result;
    if (filterHandle) {
        ASSERT_TDP_RESULT(STREAM_CONTROLLER_ERROR, "setFilterAndRegister: Filter already set.");
    }

    /* Set filter to demux */
    result = Demux_Set_Filter(playerHandle, tablePid, tableId, &filterHandle);
    ASSERT_TDP_RESULT(result, "setFilterAndRegister: Demux_Set_Filter");

    /* Register section filter callback */
    switch(tableId) {
        case PAT_ID:
            result = Demux_Register_Section_Filter_Callback(patCallback);
            break;

        case PMT_ID:
            result = Demux_Register_Section_Filter_Callback(pmtCallback);
            break;
    }
    ASSERT_TDP_RESULT(result, "setFilterAndRegister: Demux_Register_Section_Filter_Callback");

    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Function for freeing demux filter and unregistering corresponding callback.
 *
 * @param    callback - [in] callback function to unregister.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static streamControllerStatus freeFilter(int32_t (*callback)(uint8_t* buffer))
{
    uint8_t result;

    /* Free demux filter */
    result = Demux_Free_Filter(playerHandle, filterHandle);
    ASSERT_TDP_RESULT(result, "freeFilter: Demux_Free_Filter");
    filterHandle = 0;

    /* Unregister callback */
    result = Demux_Unregister_Section_Filter_Callback(callback);
    ASSERT_TDP_RESULT(result, "freeFilter: Demux_Unregister_Section_Filter_Callback");

    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Function for saving channel read from PMT table.
 *
 * @param    pmt - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static void pmtSaveChannel(pmtTable *pmt)
{
    int32_t streamType;

    channels.channel[channelCounter].pmtProgramNumber = pmt->pmtHeader.programNumber;

    channels.channel[channelCounter].channelInit.audioType = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].channelInit.videoType = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].channelInit.audioPID = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].channelInit.videoPID = CONFIGURATION_PARSER_NOT_SET;

    channels.channel[channelCounter].presentShowStartTime = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].presentShowDuration = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].presentShowName = NULL;
    channels.channel[channelCounter].presentShowDescription = NULL;

    channels.channel[channelCounter].followingShowStartTime = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].followingShowDuration = CONFIGURATION_PARSER_NOT_SET;
    channels.channel[channelCounter].followingShowName = NULL;
    channels.channel[channelCounter].followingShowDescription = NULL;

    channels.channel[channelCounter].subtitleCount = 0;
    channels.channel[channelCounter].subtitles = NULL;

    int32_t i;
    for (i = 0; i < pmt->elementaryInformationCount; i++) {
        streamType = streamTypeDVBtoTDP(pmt->elementaryInformation[i].streamType);
        if (streamType >= AUDIO_TYPE_DOLBY_AC3 && streamType <= AUDIO_TYPE_UNSUPPORTED) {
            /* Audio stream type */
            if (channels.channel[channelCounter].channelInit.audioType == CONFIGURATION_PARSER_NOT_SET) {
                channels.channel[channelCounter].channelInit.audioType = streamType;
                channels.channel[channelCounter].channelInit.audioPID = pmt->elementaryInformation[i].elementaryPid;
            }
        }
        else if (streamType >= VIDEO_TYPE_H264 && streamType <= VIDEO_TYPE_VP6F) {
            /* Video stream type */
            channels.channel[channelCounter].channelInit.videoType = streamType;
            channels.channel[channelCounter].channelInit.videoPID = pmt->elementaryInformation[i].elementaryPid;
        }

        if (pmt->subtitleCount) {
            channels.channel[channelCounter].subtitleCount = pmt->subtitleCount;
            channels.channel[channelCounter].subtitles = pmt->subtitles;
        }
    }
    channelCounter++;
}

/****************************************************************************
 * @brief    Function for saving channel read from EIT table.
 *
 * @param    eit - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static void eitSaveChannel(eitTable *eit)
{
    int i;
    int j;
    for (i = 0; i < channels.channelCount; i++) {
        if (eit->eitHeader.serviceId == channels.channel[i].pmtProgramNumber) {
            for (j = 0; j < eit->eventInformationCount; j++) {
                if (eit->eventInformation[j].runningStatus == CHANNEL_RUNNING_STATUS) {
                    channels.channel[i].presentShowStartTime = eit->eventInformation[j].startTime;
                    channels.channel[i].presentShowDuration = eit->eventInformation[j].duration;

                    if (channels.channel[i].presentShowName != NULL) {
                        free(channels.channel[i].presentShowName);
                        channels.channel[i].presentShowName = NULL;
                    }
                    if (channels.channel[i].presentShowDescription != NULL) {
                        free(channels.channel[i].presentShowDescription);
                        channels.channel[i].presentShowDescription = NULL;
                    }

                    channels.channel[i].presentShowName = eit->eventInformation[j].eventNameChar;
                    channels.channel[i].presentShowDescription = eit->eventInformation[j].textChar;
                }
                else {
                    channels.channel[i].followingShowStartTime = eit->eventInformation[j].startTime;
                    channels.channel[i].followingShowDuration = eit->eventInformation[j].duration;

                    if (channels.channel[i].followingShowName != NULL) {
                        free(channels.channel[i].followingShowName);
                        channels.channel[i].followingShowName = NULL;
                    }
                    if (channels.channel[i].followingShowDescription != NULL) {
                        free(channels.channel[i].followingShowDescription);
                        channels.channel[i].followingShowDescription = NULL;
                    }

                    channels.channel[i].followingShowName = eit->eventInformation[j].eventNameChar;
                    channels.channel[i].followingShowDescription = eit->eventInformation[j].textChar;
                }
            } // eit->eventInformationCount for loop end
        } //if eit->eitHeader.serviceId == channels.channel[i].pmtProgramNumber end
    } // channels.channelCount for loop end


    for (i = 0; i < eit->eventInformationCount; i++) {
        eit->eventInformation[i].eventNameChar = NULL;
        eit->eventInformation[i].textChar = NULL;
    }
    free(eit->eventInformation);
    eit->eventInformation = NULL;
} // eitSaveChannel end

/****************************************************************************
 * @brief    Function for converting DVB stream type to TDP stream type.
 *
 * @param    dvbStreamType - [in] DVB standard stream type value.
 *
 * @return   TDP standard stream type value.
 *           CONFIGURATION_PARSER_NOT_SET, in case of an unknown value.
****************************************************************************/
static streamControllerStatus streamTypeDVBtoTDP(uint32_t dvbStreamType)
{
    switch(dvbStreamType) {
        case dvbVideoMPEG2:
            return VIDEO_TYPE_MPEG2;

        case dvbAudioMPEG:
            return AUDIO_TYPE_MPEG_AUDIO;
    }

    return CONFIGURATION_PARSER_NOT_SET;
}

/****************************************************************************
 * @brief    Function for locking mutex and waiting for condition.
 *
 * @param    seconds - [in] Mutex lock time in seconds.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static streamControllerStatus timedWaitForCondition(uint8_t seconds)
{
    struct timespec lockStatusWaitTime;
    struct timeval now;

    gettimeofday(&now,NULL);
    lockStatusWaitTime.tv_sec = now.tv_sec + seconds;

    ASSERT_TDP_RESULT(pthread_mutex_lock(&statusMutex), "threadMutexUnlock: pthread_mutex_lock");
    if (ETIMEDOUT == pthread_cond_timedwait(&statusCondition, &statusMutex, &lockStatusWaitTime)) {
        printf("\n\nLock timeout exceeded!\n\n");
        return STREAM_CONTROLLER_ERROR;
    }
    ASSERT_TDP_RESULT(pthread_mutex_unlock(&statusMutex), "timedWaitForCondition: pthread_mutex_unlock");

    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Function for signaling condition.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static streamControllerStatus threadMutexUnlock()
{
    ASSERT_TDP_RESULT(pthread_mutex_lock(&statusMutex), "threadMutexUnlock: pthread_mutex_lock");
    ASSERT_TDP_RESULT(pthread_cond_signal(&statusCondition), "threadMutexUnlock: pthread_cond_signal");
    ASSERT_TDP_RESULT(pthread_mutex_unlock(&statusMutex), "threadMutexUnlock: pthread_mutex_unlock");

    return STREAM_CONTROLLER_NO_ERROR;
}
/* -------------------- HELPER FUNCTIONS -------------------- */

/* -------------------- CALLBACK FUNCTIONS -------------------- */
/****************************************************************************
 * @brief    Callback function for setting and calling corresponding functions for PAT table parsing.
 *
 * @param    status - [in] Tuner locks status value.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static int32_t tunerStatusCallback(t_LockStatus status)
{
    if (status == STATUS_LOCKED) {
        threadMutexUnlock();
    }
    else {
        printf("\n\n\tCALLBACK NOT LOCKED\n\n");
    }
    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Callback function for setting and calling corresponding functions for PAT table parsing.
 *
 * @param    buffer - [in] Input buffer.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static int32_t patCallback(uint8_t *buffer)
{
    uint8_t result;

    pat = (patTable*) malloc(sizeof(patTable));

    result = parsePAT(buffer, pat);
    ASSERT_TDP_RESULT(result, "patCallback: parsePAT");

    result = freeFilter(patCallback);
    ASSERT_TDP_RESULT(result, "patCallback: freeFilter");

    threadMutexUnlock();

    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Callback function for setting and calling corresponding functions for PMT table parsing.
 *
 * @param    buffer - [in] Input buffer.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static int32_t pmtCallback(uint8_t *buffer)
{
    uint8_t result;
    pmtTable pmt;

    result = parsePMT(buffer, &pmt);
    ASSERT_TDP_RESULT(result, "pmtCallback: parsePMT");

    pmtSaveChannel(&pmt);
    free(pmt.elementaryInformation);

    pmt.elementaryInformation = NULL;
    pmt.subtitles = NULL;

    result = freeFilter(pmtCallback);
    ASSERT_TDP_RESULT(result, "pmtCallback: freeFilter");

    threadMutexUnlock();

    return STREAM_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Callback function for setting and calling corresponding functions for EIT table parsing.
 *
 * @param    buffer - [in] Input buffer.
 *
 * @return   STREAM_CONTROLLER_NO_ERROR, if there are no errors.
 *           STREAM_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static int32_t eitCallback(uint8_t *buffer)
{
    eitTable eit;

    uint8_t result;
    result = parseEIT(buffer, &eit);
    ASSERT_TDP_RESULT(result, "eitCallback: parseEIT");

    //printEIT(&eit);

    eitSaveChannel(&eit);

    threadMutexUnlock();

    return STREAM_CONTROLLER_NO_ERROR;
}
/* -------------------- CALLBACK FUNCTIONS -------------------- */