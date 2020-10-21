/***************************************************************************************
 * Faculty of Electrical Engineering, Computer Science and Information Technology Osijek
 *
 * -----------------------------------------------------
 * Project assignment from the course: DIGITAL IMAGE PROCESSING DAKR4I-01
 * -----------------------------------------------------
 * Assignment title: TV application (code: PPUTVIOS_20_2018_OS)
 * -----------------------------------------------------
 * \file tables_parser.c
 *
 * \brief
 * Implementation of the module for parsing transport stream tables.
 *
 * Last updated on 4 June 2018
 *
 * @Author Luka Umiljanović
 ***************************************************************************************/

#include "tables_parser.h"

#include <stdio.h>
#include <stdlib.h>

tablesParserStatus parsePAT(uint8_t *buffer, patTable *pat)
{
    pat->patHeader.tableId = (uint8_t)*buffer;

    pat->patHeader.sectionSyntaxIndicator = (uint8_t)(*(buffer + 1) >> 7) & 0x01;

    pat->patHeader.sectionLength = (uint16_t)((*(buffer + 1) << 8) + *(buffer + 2)) & 0x0FFF;

    pat->patHeader.transportStreamId = (uint16_t)(*(buffer + 3) << 8) + *(buffer + 4);

    pat->patHeader.versionNumber = (uint8_t)(*(buffer + 5) >> 1) & 0x001F;

    pat->patHeader.currentNextIndicator = (uint8_t)(*(buffer + 5) >> 7) & 0x01;

    pat->patHeader.sectionNumber = (uint8_t) * (buffer + 6);

    pat->patHeader.lastSectionNumber = (uint8_t) * (buffer + 7);

    pat->programCount = 0;
    pat->sectionCount = (uint8_t)(pat->patHeader.sectionLength - 9) / 4;

    pat->programInformation = (patTableProgramInformation *)malloc(pat->sectionCount * sizeof(patTableProgramInformation));

    int i;

    for (i = 0; i < pat->sectionCount; i++)
    {
        pat->programInformation[i].programNumber = (uint16_t)(*(buffer + 8 + i * 4) << 8) + *(buffer + 9 + i * 4);
        pat->programInformation[i].programMapPid = (uint16_t)((*(buffer + 10 + i * 4) << 8) + *(buffer + 11 + i * 4)) & 0x1FFF;

        if (pat->programInformation[i].programNumber)
        {
            pat->programCount++;
        }
    }

    //printPAT(pat);

    return TABLES_PARSER_NO_ERROR;
}

tablesParserStatus parsePMT(uint8_t *buffer, pmtTable *pmt)
{
    pmt->pmtHeader.tableId = (uint8_t)*buffer;

    pmt->pmtHeader.sectionSyntaxIndicator = (uint8_t)(*(buffer + 1) >> 7) & 0x01;

    pmt->pmtHeader.sectionLength = (uint16_t)(((*(buffer + 1) << 8) + *(buffer + 2)) & 0x0FFF);

    pmt->pmtHeader.programNumber = (uint16_t)(*(buffer + 3) << 8) + *(buffer + 4);

    pmt->pmtHeader.versionNumber = (uint8_t)(*(buffer + 5) >> 1) & 0x001F;

    pmt->pmtHeader.currentNextIndicator = (uint8_t)(*(buffer + 5) >> 7) & 0x01;

    pmt->pmtHeader.sectionNumber = (uint8_t) * (buffer + 6);

    pmt->pmtHeader.lastSectionNumber = (uint8_t) * (buffer + 7);

    pmt->pmtHeader.pcrPid = (uint16_t)((*(buffer + 8) << 8) + *(buffer + 9)) & 0x1FFF;

    pmt->pmtHeader.programInfoLength = (uint16_t)((*(buffer + 10) << 8) + *(buffer + 11)) & 0x0FFF;

    pmt->elementaryInformationCount = (uint8_t)(pmt->pmtHeader.sectionLength - 13) / 5;
    pmt->elementaryInformation = (pmtTableElementaryInformation *)malloc(pmt->elementaryInformationCount * sizeof(pmtTableElementaryInformation));

    pmt->subtitleCount = 0;
    pmt->subtitles = NULL;

    int offset = 0;
    int i;
    int j;

    for (i = 0; i < pmt->elementaryInformationCount; i++)
    {
        pmt->elementaryInformation[i].streamType = (uint8_t) * (buffer + 12 + i * 5 + offset);
        pmt->elementaryInformation[i].elementaryPid = (uint16_t)((*(buffer + 13 + i * 5 + offset) << 8) + *(buffer + 14 + i * 5 + offset)) & 0x1FFF;
        pmt->elementaryInformation[i].esInfoLength = (uint16_t)((*(buffer + 15 + i * 5 + offset) << 8) + *(buffer + 16 + i * 5 + offset)) & 0x0FFF;

        if (((uint8_t) * (buffer + 17 + i * 5 + offset)) == SUBTITLING_DESCRIPTOR_TAG)
        {

            pmt->subtitleCount = ((uint8_t) * (buffer + 18 + i * 5 + offset)) / 8;

            pmt->subtitles = (char *)malloc(sizeof(char) * pmt->subtitleCount * SUBTITLE_CHARACTERS_COUNT + 1);

            for (j = 0; j < pmt->subtitleCount; j++)
            {
                pmt->subtitles[j * SUBTITLE_CHARACTERS_COUNT] = (uint8_t) * (buffer + 19 + i * 5 + offset);
                pmt->subtitles[j * SUBTITLE_CHARACTERS_COUNT + 1] = (uint8_t) * (buffer + 20 + i * 5 + offset);
                pmt->subtitles[j * SUBTITLE_CHARACTERS_COUNT + 2] = (uint8_t) * (buffer + 21 + i * 5 + offset);
                pmt->subtitles[j * SUBTITLE_CHARACTERS_COUNT + 3] = '\0';
            }
        }

        offset += pmt->elementaryInformation[i].esInfoLength;
    }

    //printPMT(pmt);

    return TABLES_PARSER_NO_ERROR;
}

tablesParserStatus parseEIT(uint8_t *buffer, eitTable *eit)
{
    eit->eitHeader.tableId = (uint8_t) * (buffer);

    eit->eitHeader.sectionSyntaxIndicator = (uint8_t)(*(buffer + 1) >> 7) & 0x01;

    eit->eitHeader.sectionLength = (uint16_t)((*(buffer + 1) << 8) + *(buffer + 2)) & 0x0FFF;

    eit->eitHeader.serviceId = (uint16_t)((*(buffer + 3) << 8) + *(buffer + 4)) & 0xFFFF;

    eit->eitHeader.versionNumber = (uint8_t)(*(buffer + 5) >> 1) & 0x1F;

    eit->eitHeader.currentNextIndicator = (uint8_t) * (buffer + 5) & 0x1;

    eit->eitHeader.sectionNumber = (uint8_t) * (buffer + 6) & 0xFF;

    eit->eitHeader.lastSectionNumber = (uint8_t) * (buffer + 7) & 0xFF;

    eit->eitHeader.transportStreamId = (uint16_t)((*(buffer + 8) << 8) + *(buffer + 9)) & 0xFFFF;

    eit->eitHeader.originalNetworkId = (uint16_t)((*(buffer + 10) << 8) + *(buffer + 11)) & 0xFFFF;

    eit->eitHeader.segmentLastSectionNumber = (uint8_t) * (buffer + 12) & 0xFF;

    eit->eitHeader.lastTableId = (uint8_t) * (buffer + 13) & 0xFF;

    eit->eventInformationCount = 0;
    eit->eventInformation = NULL;

    int i = 0;
    while (i < (eit->eitHeader.sectionLength - 11 /*from section length to loop*/ - 4 /*CRC*/))
    {
        eit->eventInformationCount++;

        if (!eit->eventInformation)
        {
            eit->eventInformation = (eitTableEventInformation *)malloc(sizeof(eitTableEventInformation));
        }
        else
        {
            eit->eventInformation = (eitTableEventInformation *)realloc(eit->eventInformation, eit->eventInformationCount * sizeof(eitTableEventInformation));
        }

        eit->eventInformation[eit->eventInformationCount - 1].eventId = (uint16_t)((*(buffer + 14 + i) << 8) + *(buffer + 14 + i + 1));
        i += 2;

        eit->eventInformation[eit->eventInformationCount - 1].startTime = (uint32_t)((((*(buffer + 14 + i + 2) << 8) + *(buffer + 14 + i + 3)) << 8) + *(buffer + 14 + i + 4)); //saves only time
        i += 5;

        eit->eventInformation[eit->eventInformationCount - 1].duration = (uint32_t)((*(buffer + 14 + i) << 16) + (*(buffer + 14 + i + 1) << 8) + *(buffer + 14 + i + 2)) & 0xFFFFF;
        i += 3;

        eit->eventInformation[eit->eventInformationCount - 1].runningStatus = (uint8_t)(*(buffer + 14 + i) >> 5) & 0x07;

        eit->eventInformation[eit->eventInformationCount - 1].freeCAMode = (uint8_t)(*(buffer + 14 + i) >> 4) & 0x01;

        eit->eventInformation[eit->eventInformationCount - 1].descriptorsLoopLength = (uint16_t)((*(buffer + 14 + i) << 8) + *(buffer + 14 + i + 1)) & 0x0FFF;
        i += 2;

        int j = 0;
        int k = 0;

        eit->eventInformation[eit->eventInformationCount - 1].descriptorTag = TABLES_PARSER_NOT_INITIALIZED;
        eit->eventInformation[eit->eventInformationCount - 1].descriptorLength = TABLES_PARSER_NOT_INITIALIZED;

        eit->eventInformation[eit->eventInformationCount - 1].eventNameLength = TABLES_PARSER_NOT_INITIALIZED;
        eit->eventInformation[eit->eventInformationCount - 1].eventNameChar = NULL;

        eit->eventInformation[eit->eventInformationCount - 1].textLength = TABLES_PARSER_NOT_INITIALIZED;
        eit->eventInformation[eit->eventInformationCount - 1].textChar = NULL;

        while (j < eit->eventInformation[eit->eventInformationCount - 1].descriptorsLoopLength)
        {
            uint8_t tempDescriptorTag = (uint8_t) * (buffer + 14 + i + j);
            uint8_t tempDescriptorLength = (uint8_t) * (buffer + 14 + i + 1 + j);

            if (tempDescriptorTag == SHORT_EVENT_DESCRIPTOR_TAG)
            {
                eit->eventInformation[eit->eventInformationCount - 1].descriptorTag = tempDescriptorTag;
                eit->eventInformation[eit->eventInformationCount - 1].descriptorLength = tempDescriptorLength;

                /* getting event name */
                eit->eventInformation[eit->eventInformationCount - 1].eventNameLength = (uint8_t) * (buffer + 14 + i + 5 + j);
                eit->eventInformation[eit->eventInformationCount - 1].eventNameChar = (char *)malloc(eit->eventInformation[eit->eventInformationCount - 1].eventNameLength * sizeof(char));
                for (k = 0; k < eit->eventInformation[eit->eventInformationCount - 1].eventNameLength; k++)
                {
                    if (*(buffer + 14 + i + 6 + j + k + 1) < 0 || *(buffer + 14 + i + 6 + j + k + 1) > 127)
                        eit->eventInformation[eit->eventInformationCount - 1].eventNameChar[k] = ' ';
                    else
                        eit->eventInformation[eit->eventInformationCount - 1].eventNameChar[k] = *(buffer + 14 + i + 6 + j + k + 1);
                }
                eit->eventInformation[eit->eventInformationCount - 1].eventNameChar[eit->eventInformation[eit->eventInformationCount - 1].eventNameLength - 1] = '\0';

                /* getting event short description */
                eit->eventInformation[eit->eventInformationCount - 1].textLength = (uint8_t) * (buffer + 14 + i + 6 + eit->eventInformation[eit->eventInformationCount - 1].eventNameLength + j);
                eit->eventInformation[eit->eventInformationCount - 1].textChar = (char *)malloc(eit->eventInformation[eit->eventInformationCount - 1].textLength * sizeof(char));

                for (k = 0; k < eit->eventInformation[eit->eventInformationCount - 1].textLength; k++)
                {
                    if (*(buffer + 14 + i + 6 + eit->eventInformation[eit->eventInformationCount - 1].eventNameLength + 1 + j + k + 1) < 0 ||
                        *(buffer + 14 + i + 6 + eit->eventInformation[eit->eventInformationCount - 1].eventNameLength + 1 + j + k + 1) > 127)
                        eit->eventInformation[eit->eventInformationCount - 1].textChar[k] = ' ';
                    else
                        eit->eventInformation[eit->eventInformationCount - 1].textChar[k] = *(buffer + 14 + i + 6 + eit->eventInformation[eit->eventInformationCount - 1].eventNameLength + 1 + j + k + 1);
                }
                eit->eventInformation[eit->eventInformationCount - 1].textChar[eit->eventInformation[eit->eventInformationCount - 1].textLength - 1] = '\0';
            }
            j += tempDescriptorLength + 2;
        }
        i += eit->eventInformation[eit->eventInformationCount - 1].descriptorsLoopLength;
    }

    return TABLES_PARSER_NO_ERROR;
}

tablesParserStatus printPAT(patTable *pat)
{
    printf("\nPAT TABLE\n");
    printf("\tTable ID (hex): %#04x\n", pat->patHeader.tableId);
    printf("\tSection syntax indicator: %d\n", pat->patHeader.sectionSyntaxIndicator);
    printf("\tSection length: %d\n", pat->patHeader.sectionLength);
    printf("\tTransport stream id: %d\n", pat->patHeader.transportStreamId);
    printf("\tVersion number: %d\n", pat->patHeader.versionNumber);
    printf("\tSection number: %d\n", pat->patHeader.sectionNumber);
    printf("\tCurrent Next indicator: %d\n", pat->patHeader.currentNextIndicator);
    printf("\tLast section number: %d\n", pat->patHeader.lastSectionNumber);
    int i;
    for (i = 0; i < pat->sectionCount; i++)
    {
        if (pat->programInformation[i].programNumber)
        {
            printf("\tSection %d:\n", i);
            printf("\t\tProgram number: %d\n", pat->programInformation[i].programNumber);
            printf("\t\tPID: %d\n", pat->programInformation[i].programMapPid);
        }
    }

    return TABLES_PARSER_NO_ERROR;
}

tablesParserStatus printPMT(pmtTable *pmt)
{
    printf("\nPMT TABLE\n");
    printf("\tTable ID (hex): %#04x\n", pmt->pmtHeader.tableId);
    printf("\tSection syntax indicator: %d\n", pmt->pmtHeader.sectionSyntaxIndicator);
    printf("\tSection length: %d\n", pmt->pmtHeader.sectionLength);
    printf("\tProgram number id: %d\n", pmt->pmtHeader.programNumber);
    printf("\tVersion number: %d\n", pmt->pmtHeader.versionNumber);
    printf("\tSection number: %d\n", pmt->pmtHeader.sectionNumber);
    printf("\tCurrent Next indicator: %d\n", pmt->pmtHeader.currentNextIndicator);
    printf("\tLast section number: %d\n", pmt->pmtHeader.lastSectionNumber);
    printf("\tpcrPID: %d\n", pmt->pmtHeader.pcrPid);
    printf("\tProgram info length: %d\n", pmt->pmtHeader.programInfoLength);
    printf("\tSubtitle count: %d\n", pmt->subtitleCount);
    int i = 0;
    int characterCount = 0;
    if (pmt->subtitleCount)
    {
        while (pmt->subtitles[i] != '\0')
        {
            printf("%c", pmt->subtitles[i]);
            characterCount++;
            i++;

            if (characterCount == 3)
            {
                if (pmt->subtitles[i] == '\0')
                {
                    characterCount = 0;
                }
                else
                {
                    printf(", ");
                    characterCount = 0;
                }
            }
        }
        printf("\n");
    }
    for (i = 0; i < pmt->elementaryInformationCount; i++)
    {
        if (pmt->elementaryInformation[i].streamType)
        {
            printf("\tSection %d:\n", i);
            printf("\t\tStream type (hex): %#04x\n", pmt->elementaryInformation[i].streamType);
            printf("\t\tElementary PID: %d\n", pmt->elementaryInformation[i].elementaryPid);
        }
    }

    return TABLES_PARSER_NO_ERROR;
}

tablesParserStatus printEIT(eitTable *eit)
{
    printf("\nEIT TABLE\n");
    printf("\tTable ID (hex): %#04x\n", eit->eitHeader.tableId);
    printf("\tSection syntax indicator: %d\n", eit->eitHeader.sectionSyntaxIndicator);
    printf("\tSection length: %d\n", eit->eitHeader.sectionLength);
    printf("\tService ID: %d\n", eit->eitHeader.serviceId);
    printf("\tVersion number: %d\n", eit->eitHeader.versionNumber);
    printf("\tCurrent Next indicator: %d\n", eit->eitHeader.currentNextIndicator);
    printf("\tSection number: %d\n", eit->eitHeader.sectionNumber);
    printf("\tLast section number: %d\n", eit->eitHeader.lastSectionNumber);
    printf("\tTransport stream ID: %d\n", eit->eitHeader.transportStreamId);
    printf("\tOriginal network ID: %d\n", eit->eitHeader.originalNetworkId);
    printf("\tSegment last section number: %d\n", eit->eitHeader.segmentLastSectionNumber);
    printf("\tLast table ID (hex): %#04x\n", eit->eitHeader.lastTableId);
    printf("\tEvent information count: %d\n", eit->eventInformationCount);
    int i = 0;
    for (i = 0; i < eit->eventInformationCount; i++)
    {
        printf("\tEvent %d:\n", i);
        printf("\t\tEvent ID: %d\n", eit->eventInformation[i].eventId);
        printf("\t\tStart time (hex): %#04x\n", eit->eventInformation[i].startTime);
        printf("\t\tDuration (hex): %#04x\n", eit->eventInformation[i].duration);
        printf("\t\tRunning status: %d\n", eit->eventInformation[i].runningStatus);
        printf("\t\tFree CA mode: %d\n", eit->eventInformation[i].freeCAMode);
        printf("\t\tDescriptors loop length: %d\n", eit->eventInformation[i].descriptorsLoopLength);
        printf("\t\t\tShort event descriptor tag: %d\n", eit->eventInformation[i].descriptorTag);
        printf("\t\t\tShort event descriptor length: %d\n", eit->eventInformation[i].descriptorLength);
        printf("\t\t\tShort event descriptor event name length: %d\n", eit->eventInformation[i].eventNameLength);
        printf("\t\t\tShort event descriptor event name char: %s\n", eit->eventInformation[i].eventNameChar);
        printf("\t\t\tShort event descriptor text length: %d\n", eit->eventInformation[i].textLength);
        printf("\t\t\tShort event descriptor text char: %s\n", eit->eventInformation[i].textChar);
    }

    return TABLES_PARSER_NO_ERROR;
}
