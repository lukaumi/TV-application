/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file tables_parser.h
 *
 * \brief
 * Header of the module for parsing transport stream tables.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#ifndef _TABLES_PARSER_H_
#define _TABLES_PARSER_H_

#include <stdint.h>

#define SUBTITLING_DESCRIPTOR_TAG 0x59
#define SHORT_EVENT_DESCRIPTOR_TAG 0x4D
#define SUBTITLE_CHARACTERS_COUNT 3

typedef enum _tablesParserStatus
{
    TABLES_PARSER_NOT_INITIALIZED = -1,
    TABLES_PARSER_NO_ERROR,
    TABLES_PARSER_ERROR
} tablesParserStatus;

/* ---- PAT table ---- */
typedef struct _patTableHeader
{
    uint8_t tableId;
    uint8_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t transportStreamId;
    uint8_t versionNumber;
    uint8_t currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
} patTableHeader;

typedef struct _patTableProgramInformation
{
    uint16_t programNumber;
    uint16_t programMapPid;
} patTableProgramInformation;

typedef struct _patTable
{
    patTableHeader patHeader;
    patTableProgramInformation *programInformation;
    uint8_t sectionCount;
    uint8_t programCount;
} patTable;
/* ---- PAT table ---- */

/* ---- PMT table ---- */
typedef struct _pmtTableHeader
{
    uint8_t tableId;
    uint8_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t programNumber;
    uint8_t versionNumber;
    uint8_t currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t pcrPid;
    uint16_t programInfoLength;
} pmtTableHeader;

typedef struct _pmtTableElementaryInformation
{
    uint8_t streamType;
    uint16_t elementaryPid;
    uint16_t esInfoLength;
} pmtTableElementaryInformation;

typedef struct _pmtTable
{
    pmtTableHeader pmtHeader;
    pmtTableElementaryInformation *elementaryInformation;
    uint8_t elementaryInformationCount;
    uint8_t subtitleCount;
    char *subtitles;
} pmtTable;
/* ---- PMT table ---- */

/* ---- EIT table ---- */
typedef struct _eitTableHeader
{
    uint8_t tableId;
    uint8_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t serviceId;
    uint8_t versionNumber;
    uint8_t currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t transportStreamId;
    uint16_t originalNetworkId;
    uint8_t segmentLastSectionNumber;
    uint8_t lastTableId;
} eitTableHeader;

typedef struct _eitTableEventInformation
{
    uint16_t eventId;
    uint32_t startTime;
    uint32_t duration;
    uint8_t runningStatus;
    uint8_t freeCAMode;
    uint16_t descriptorsLoopLength;

    uint8_t descriptorTag;
    uint8_t descriptorLength;
    uint8_t eventNameLength;
    char *eventNameChar;
    uint8_t textLength;
    char *textChar;

} eitTableEventInformation;

typedef struct _eitTable
{
    eitTableHeader eitHeader;
    eitTableEventInformation *eventInformation;
    uint8_t eventInformationCount;
    uint8_t rating;
} eitTable;
/* ---- EIT table ---- */

/****************************************************************************
 * @brief    Function for parsing PAT table from transport stream.
 *
 * @param    buffer - [in] Input buffer.
 *           pat - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus parsePAT(uint8_t *buffer, patTable *pat);

/****************************************************************************
 * @brief    Function for parsing PMT table from transport stream.
 *
 * @param    buffer - [in] Input buffer.
 *           pmt - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus parsePMT(uint8_t *buffer, pmtTable *pmt);

/****************************************************************************
 * @brief    Function for parsing EIT table from transport stream.
 *
 * @param    buffer - [in] Input buffer.
 *           eit - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus parseEIT(uint8_t *buffer, eitTable *eit);

/****************************************************************************
 * @brief    Function for printing PAT table variables values.
 *
 * @param    pat - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus printPAT(patTable *pat);

/****************************************************************************
 * @brief    Function for printing PMT table variables values.
 *
 * @param    pmt - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus printPMT(pmtTable *pmt);

/****************************************************************************
 * @brief    Function for printing EIT table variables values.
 *
 * @param    eit - [in] Pointer to structure variable in which loaded parameters are stored.
 *
 * @return   TABLES_PARSER_NO_ERROR, if there are no errors.
 *           TABLES_PARSER_ERROR, in case of an error.
****************************************************************************/
tablesParserStatus printEIT(eitTable *eit);

#endif // _TABLES_PARSER_H_
