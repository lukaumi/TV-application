/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file graphics_controller.c
 *
 * \brief
 * Implementation of the module for handling graphics.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#include "graphics_controller.h"

#include <stdio.h>
#include <directfb.h>
#include "math.h"

#include "timer_controller.h"

/* helper macro functions needed only for graphics controller module */
#define DEGREES_TO_RADIANS(deg) ((deg) * M_PI / 180.0)
// https://stackoverflow.com/questions/2570934/how-to-round-floating-point-numbers-to-the-nearest-integer-in-c
#define roundNumber(x) ( (int) ((x) < 0.0 ? (x) - 0.5 : (x) + 0.5) )

#define DFBCHECK(x...)                                          \
{                                                               \
    DFBResult err = x;                                          \
    if (err != DFB_OK) {                                        \
        fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );   \
        DirectFBErrorFatal( #x, err );                          \
        return GRAPHICS_CONTROLLER_ERROR;                       \
    }                                                           \
}

/* helper variables needed only for graphics controller module */
static IDirectFBSurface *primary = NULL;
static IDirectFB *dfbInterface = NULL;
static int screenWidth = 0;
static int screenHeight = 0;
static DFBSurfaceDescription surfaceDesc;

static IDirectFBFont *fontInterface = NULL;
static DFBFontDescription fontDesc;

static timer_t timerChannelInfo;
static timer_t timerChannelNumberMessage;
static timer_t timerVolumeInfo;

static uint8_t showingChannelInfo;
static uint8_t showingVolumeInfo;

/* helper functions needed only for graphics controller module */
static void removeChannelInfo();
static void removeVolumeInfo();
static void removeMenuInfo();
static void removeChannelNumberMessage();

static graphicsControllerStatus formatAndDrawMenuShowName(const char* status, char *showname);
static graphicsControllerStatus formatAndDrawMenuShowTimes(uint32_t startTime, uint32_t duration);
static graphicsControllerStatus formatAndDrawShowDescription(char *source);

graphicsControllerStatus graphicsControllerInit()
{
    /* initialize DirectFB */
    DFBCHECK(DirectFBInit(NULL, NULL));

    /* fetch the DirectFB interface */
    DFBCHECK(DirectFBCreate(&dfbInterface));

    /* tell the DirectFB to take the full screen for this application */
    DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));

    /* create primary surface with double buffering enabled */
    surfaceDesc.flags = DSDESC_CAPS;
    surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
    DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));

    /* fetch the screen size */
    DFBCHECK(primary->GetSize(primary, &screenWidth, &screenHeight));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus graphicsControllerDeinit()
{
    DFBCHECK(primary->Release(primary));
    DFBCHECK(dfbInterface->Release(dfbInterface));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawChannelNumber(uint16_t channelNumberValue)
{
    if (timerChannelInfo) timerStopAndDelete(&timerChannelInfo);
    if (timerVolumeInfo) timerStopAndDelete(&timerVolumeInfo);
    if (timerChannelNumberMessage) timerStopAndDelete(&timerChannelNumberMessage);

    char channelNumberString[4];
    sprintf(channelNumberString, "%d", channelNumberValue);

    clearScreen(COLOUR_BLACK);

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 100;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    /* draw yellow #FFA500 channel number */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, channelNumberString, -1, screenHeight/7, screenHeight/7, DSTF_LEFT));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawChannelNumberMessage(uint16_t channelNumberValue)
{
    if (timerChannelInfo) timerStopAndDelete(&timerChannelInfo);
    if (timerVolumeInfo) timerStopAndDelete(&timerVolumeInfo);
    if (timerChannelNumberMessage) timerStopAndDelete(&timerChannelNumberMessage);

    char message[27];
    sprintf(message, "Invalid channel number %d", channelNumberValue);

    clearScreen(COLOUR_BLACK);

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 70;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    /* draw yellow #FFA500 channel number */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, message, -1, screenHeight/7, screenHeight/7, DSTF_LEFT));

    /* timer setup */
    timerSetAndStart(&timerChannelNumberMessage, 2, removeChannelNumberMessage);

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawChannelInfo(uint16_t channelNumberValue, uint8_t subtitleCount, char *subtitles)
{
    if (timerChannelInfo) timerStopAndDelete(&timerChannelInfo);
    if (timerVolumeInfo) timerStopAndDelete(&timerVolumeInfo);
    if (timerChannelNumberMessage) timerStopAndDelete(&timerChannelNumberMessage);

    char channelNumber[12];

    if (channelNumberValue) {
        sprintf(channelNumber, "Channel %d", channelNumberValue);
    }

    int subtitlesArraySize = 5 * subtitleCount - 2; // (subtitleCount * 3) characters + (subtitleCount - 1) spaces + (subtitleCount - 1) commas + (1) '\0' - (1) index
    char channelSubtitles[subtitlesArraySize];

    if (subtitleCount) {
        int i = 0;
        int j = 0;
        int character_count = 0;
        while (subtitles[i] != '\0') {
            channelSubtitles[j] = subtitles[i];
            character_count++;
            j++;

            if (character_count == 3) {
                if (subtitles[i+1] == '\0') {
                    character_count = 0;
                } else {
                    channelSubtitles[j++] = ',';
                    channelSubtitles[j++] = ' ';
                    character_count = 0;
                }
            }
            i++;
        }
        channelSubtitles[subtitlesArraySize] = '\0';
    }

    clearScreen(COLOUR_BLACK);

    /* draw yellow #FFA500 info rectangle */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/4, (5.3*screenHeight)/6.5, screenWidth/2, screenHeight/6));

    /* draw grey #383838 info rectangle */
    DFBCHECK(primary->SetColor(primary, 0x38, 0x38, 0x38, COLOUR_WHITE));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/4 + 5, (5.3*screenHeight)/6.5 + 5, screenWidth/2 - 10, screenHeight/6 - 10));

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 68;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    /* draw yellow #FFA500 channel string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, channelNumber, -1, screenWidth/4 + 20, (5.3*screenHeight)/6.5 + 80, DSTF_LEFT));

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 48;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    if (subtitleCount) {
        DFBCHECK(primary->DrawString(primary, "Subtitles: ", -1, screenWidth/4 + 20 , (5.3*screenHeight)/6.5 + 140, DSTF_LEFT));
        DFBCHECK(primary->DrawString(primary, channelSubtitles, -1, screenWidth/4 + 260 , (5.3*screenHeight)/6.5 + 140, DSTF_LEFT));
    } else {
        DFBCHECK(primary->DrawString(primary, "No available subtitles", -1, screenWidth/4 + 20 , (5.3*screenHeight)/6.5 + 140, DSTF_LEFT));
    }

    /* timer setup */
    if (showingChannelInfo) {
        timerStopAndDelete(&timerChannelInfo);
    }
    timerSetAndStart(&timerChannelInfo, 4, removeChannelInfo);
    showingChannelInfo = 1;

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawVolumeInfo(float volumePercent)
{
    if (timerChannelInfo) timerStopAndDelete(&timerChannelInfo);
    if (timerVolumeInfo) timerStopAndDelete(&timerVolumeInfo);
    if (timerChannelNumberMessage) timerStopAndDelete(&timerChannelNumberMessage);

    char volume[4]; // 3 digits + 1 % sign + 1 '\0' - 1 index
    uint8_t volumePercentInt = roundNumber(volumePercent * 100);
    uint16_t volumeDeg = roundNumber(volumePercent * 360);

    clearScreen(COLOUR_BLACK);

    sprintf(volume, "%d%%", volumePercentInt);

    int x = screenWidth - screenWidth / 10 ;
    int y = 150;

    int radius = 120;
    int16_t i;
    uint16_t x1;
    uint16_t x2;
    uint16_t y1;
    uint16_t y2;

    int16_t startDeg = -90;
    int16_t endDeg = volumeDeg;

    /* background circle */
    endDeg -= 90;
    x1 = x + radius * cos(DEGREES_TO_RADIANS(startDeg));
    y1 = y + radius * sin(DEGREES_TO_RADIANS(startDeg));
    for (i = startDeg + 1; i <= 360; i++) {
        x2 = x + radius * cos(DEGREES_TO_RADIANS(i));
        y2 = y + radius * sin(DEGREES_TO_RADIANS(i));

        DFBCHECK(primary->SetColor(primary, 0x38, 0x38, 0x38, 0xFF));
        DFBCHECK(primary->FillTriangle(primary, x, y, x1, y1, x2, y2));

        x1 = x2;
        y1 = y2;
    }

    /* yellow #FFA500 volume circle */
    startDeg = -90;

    x1 = x + radius * cos(DEGREES_TO_RADIANS(startDeg));
    y1 = y + radius * sin(DEGREES_TO_RADIANS(startDeg));

    for (i = startDeg + 1; i <= endDeg; i++) {
        x2 = x + radius * cos(DEGREES_TO_RADIANS(i));
        y2 = y + radius * sin(DEGREES_TO_RADIANS(i));

        DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
        DFBCHECK(primary->FillTriangle(primary, x, y, x1, y1, x2, y2));

        x1 = x2;
        y1 = y2;
    }

    /* foreground volume percent circle */
    x1 = x + radius/2 * cos(DEGREES_TO_RADIANS(startDeg));
    y1 = y + radius/2 * sin(DEGREES_TO_RADIANS(startDeg));
    for (i = startDeg + 1; i <= 360; i++) {
        x2 = x + radius/2 * cos(DEGREES_TO_RADIANS(i));
        y2 = y + radius/2 * sin(DEGREES_TO_RADIANS(i));

        DFBCHECK(primary->SetColor(primary, 0x38, 0x38, 0x38, 0xFF));
        DFBCHECK(primary->FillTriangle(primary, x, y, x1, y1, x2, y2));

        x1 = x2;
        y1 = y2;
    }

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 38;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    /* draw yellow #FFA500 volume string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, volume, -1, x - radius/4 + 80, y + radius/4 -20, DSTF_RIGHT));

    /* timer setup */
    if (showingVolumeInfo) {
        timerStopAndDelete(&timerVolumeInfo);
    }
    timerSetAndStart(&timerVolumeInfo, 2, removeVolumeInfo);
    showingVolumeInfo = 1;

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawMenuInfo(uint32_t presentShowStartTime, uint32_t presentShowDuration, char *presentShowName, char *presentShowDescription,
                                      uint32_t followingShowStartTime, uint32_t followingShowDuration, char *followingShowName, char *followingShowDescription,
                                      uint8_t channelFlag)
{
    if (timerChannelInfo) timerStopAndDelete(&timerChannelInfo);
    if (timerVolumeInfo) timerStopAndDelete(&timerVolumeInfo);
    if (timerChannelNumberMessage) timerStopAndDelete(&timerChannelNumberMessage);

    if (channelFlag == 0) {
        removeMenuInfo();
        return GRAPHICS_CONTROLLER_NO_ERROR;
    }

    clearScreen(COLOUR_BLACK);

    /* draw yellow #FFA500 menu background rectangle */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/10, screenHeight/6, (8*screenWidth)/10 , (4*screenHeight)/6));

    /* draw grey #383838 menu foreground rectangle */
    DFBCHECK(primary->SetColor(primary, 0x38, 0x38, 0x38, COLOUR_WHITE));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/10 + 5, screenHeight/6 + 5, (8*screenWidth)/10 - 10, (4*screenHeight)/6 - 10));

    /* draw yellow #FFA500 menu line rectangle */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/10, screenHeight/6 + 100, (8*screenWidth)/10 , 5));

    /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 70;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    if (channelFlag == 1) {
        if ((presentShowStartTime != -1)  && (presentShowDuration != -1)) { // CONFIGURATION_PARSER_NOT_SET == -1
            if (presentShowName != NULL) formatAndDrawMenuShowName("Now:", presentShowName);

            formatAndDrawMenuShowTimes(presentShowStartTime, presentShowDuration);

            if (presentShowDescription != NULL) formatAndDrawShowDescription(presentShowDescription);

            if (followingShowStartTime && followingShowDuration) {
                /* draw yellow #FFA500 right arrow */
                DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
                DFBCHECK(primary->FillRectangle(primary, (4*screenWidth)/6 + 100, (5*screenHeight)/6 - 100, 100 , 50));

                DFBCHECK(primary->FillTriangle(primary, (5*screenWidth)/6 - 125, (5*screenHeight)/6 - 125,
                                                        (5*screenWidth)/6 - 125, (5*screenHeight)/6 - 25,
                                                        (5*screenWidth)/6 - 25, (5*screenHeight)/6 - 75));
            }
        }
        else {
            /* draw yellow #FFA500 show name string information */
            DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
            DFBCHECK(primary->DrawString(primary, "Information Not Available!", -1, screenWidth/10 + 20, screenHeight/6 + 300, DSTF_LEFT));
        }
    }

    if (channelFlag == 2) {
        if ((followingShowStartTime != -1)  && (followingShowStartTime != -1)) { // CONFIGURATION_PARSER_NOT_SET == -1
            if (followingShowName != NULL) formatAndDrawMenuShowName("Next:", followingShowName);

            formatAndDrawMenuShowTimes(followingShowStartTime, followingShowDuration);

            if (followingShowDescription != NULL) formatAndDrawShowDescription(followingShowDescription);

            if (presentShowStartTime && presentShowDuration) {
            /* draw yellow #FFA500 left arrow */
                DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
                DFBCHECK(primary->FillRectangle(primary, screenWidth/6 + 125, (5*screenHeight)/6 - 100, 100 , 50));

                DFBCHECK(primary->FillTriangle(primary, (screenWidth)/6 + 125, (5*screenHeight)/6 - 125,
                                                        (screenWidth)/6 + 125, (5*screenHeight)/6 - 25,
                                                        (screenWidth)/6 + 25, (5*screenHeight)/6 - 75));
            }
        }

        else {
            /* draw yellow #FFA500 show name string information */
            DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
            DFBCHECK(primary->DrawString(primary, "Information Not Available!", -1, screenWidth/10 + 20, screenHeight/6 + 300, DSTF_LEFT));
        }
    }

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus drawOnScreen()
{
    /* switch between the displayed and the work buffer (update the display) */
    DFBCHECK(primary->Flip(primary, NULL, 0));
    return GRAPHICS_CONTROLLER_NO_ERROR;
}

graphicsControllerStatus clearScreen(uint8_t alpha)
{
    DFBCHECK(primary->SetColor(primary, COLOUR_BLACK, COLOUR_BLACK, COLOUR_BLACK, alpha));
    DFBCHECK(primary->FillRectangle(primary, 0, 0, screenWidth, screenHeight));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

/* -------------------- HELPER FUNCTIONS -------------------- */
/****************************************************************************
 * @brief    Function for removing channel information banner from screen at timer trigger.
****************************************************************************/
static void removeChannelInfo()
{
    showingChannelInfo = 0;
    clearScreen(COLOUR_BLACK);
    drawOnScreen();
    clearScreen(COLOUR_BLACK);
}

/****************************************************************************
 * @brief    Function for removing volume information banner from screen at timer trigger.
****************************************************************************/
static void removeVolumeInfo()
{
    if (!showingChannelInfo) {
        showingVolumeInfo = 0;
        clearScreen(COLOUR_BLACK);
        drawOnScreen();
        clearScreen(COLOUR_BLACK);
    }
}

/****************************************************************************
 * @brief    Function for removing menu information banner from screen at timer trigger.
****************************************************************************/
static void removeMenuInfo()
{
    clearScreen(COLOUR_BLACK);
    drawOnScreen();
    clearScreen(COLOUR_BLACK);
}

/****************************************************************************
 * @brief    Function for removing channel number message from screen at timer trigger.
****************************************************************************/
static void removeChannelNumberMessage()
{
    clearScreen(COLOUR_BLACK);
    drawOnScreen();
    clearScreen(COLOUR_BLACK);
}

/****************************************************************************
 * @brief    Function for drawing menu information banner show name.
 *
 * @param    status - [in] String marking show running status, "Now" or "Next".
 *           showName - [in] Show name string.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static graphicsControllerStatus formatAndDrawMenuShowName(const char* status, char *showName)
{
    /* draw yellow #FFA500 Now or Next string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, status, -1, screenWidth/10 + 20, (screenHeight)/6 + 85, DSTF_LEFT));

    /* draw yellow #FFA500 show name string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, showName, -1, screenWidth/10 + 200, (screenHeight)/6 + 85, DSTF_LEFT));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Function for drawing menu information banner show times.
 *
 * @param    startTime - [in] Show start time value.
 *           duration - [in] Show duration value.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static graphicsControllerStatus formatAndDrawMenuShowTimes(uint32_t startTime, uint32_t duration)
{
    char startTimeString[6];
    char durationString[8];

    uint16_t hrs;
    uint16_t min;

    hrs = startTime >> 16;
    min = (startTime >> 8) & 0x00FF;
    sprintf(startTimeString, "%d%d:%d%d", hrs >> 4, hrs & 0x0F, min >> 4, min & 0x0F);

    hrs = duration >> 16;
    min = (duration >> 8) & 0x00FF;
    sprintf(durationString, "%d min", (hrs * 60) + ((min >> 4) * 10) + (min & 0x0f));

    /* draw yellow #FFA500 show start time string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, startTimeString, -1, screenWidth/10 + 20, screenHeight/6 + 200, DSTF_LEFT));

    /* draw yellow #FFA500 show run time string information */
    DFBCHECK(primary->SetColor(primary, 0xff, 0xa5, 0x00, COLOUR_WHITE));
    DFBCHECK(primary->DrawString(primary, durationString, -1, (5*screenWidth)/6 - 150, screenHeight/6 + 200, DSTF_LEFT));

    return GRAPHICS_CONTROLLER_NO_ERROR;
}

/****************************************************************************
 * @brief    Function for drawing menu information banner show description.
 *
 * @param    source - [in] Show description string.
 *
 * @return   GRAPHICS_CONTROLLER_NO_ERROR, if there are no errors.
 *           GRAPHICS_CONTROLLER_ERROR, in case of an error.
****************************************************************************/
static graphicsControllerStatus formatAndDrawShowDescription(char *source)
{
     /* specify the height of the font by raising the appropriate flag and setting the height value */
    fontDesc.flags = DFDESC_HEIGHT;
    fontDesc.height = 50;

    /* create the font and set the created font for primary surface text drawing */
    DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
    DFBCHECK(primary->SetFont(primary, fontInterface));

    int i = 0;
    int j;
    char buffer[255];

    while (source[i] != '\0') {
        buffer[i] = source[i];
        i++;
    }
    buffer[i] = '\0';

    char* temp;
    temp = buffer;

    for (i = 0; i < 5; i++) { // maximum 5 rows
        j = 0;
        while (temp[j] != '\0' && j < 54 ) {
            j++; // maximum 54 characters per row
        }
        if (j == 54) {
            while (temp[j] != ' ' && j > 0) {
                j--;
            }
        }

        DFBCHECK(primary->DrawString(primary, temp, j, screenWidth/10 + 20, screenHeight/6 + 300 + i * 100, DSTF_LEFT));

        if (temp[j] == '\0') break;
        temp = (char*)(temp + j);
    }

    return  GRAPHICS_CONTROLLER_NO_ERROR;
}
/* -------------------- HELPER FUNCTIONS -------------------- */