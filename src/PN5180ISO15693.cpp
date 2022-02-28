// NAME: PN5180ISO15693.h
//
// DESC: ISO15693 protocol on NXP Semiconductors PN5180 module for Arduino.
//
// Copyright (c) 2018 by Andreas Trappmann. All rights reserved.
//
// This file is part of the PN5180 library for the Arduino environment.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
//#define DEBUG 1

#include <Arduino.h>
#include "PN5180ISO15693.h"
#include "PN5180Debug.h"

PN5180ISO15693::PN5180ISO15693(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin)
    : PN5180(SSpin, BUSYpin, RSTpin)
{
}

int16_t PN5180ISO15693::ISO15693_CRC16(uint8_t *DataIn, int NbByte)
{
    int8_t i, j;
    int32_t ResCrc = ISO15693_PRELOADCRC16;

    for (i = 0; i < NbByte; i++)
    {
        ResCrc = ResCrc ^ DataIn[i];
        for (j = 8; j > 0; j--)
        {
            ResCrc = (ResCrc & ISO15693_MASKCRC16) ? (ResCrc >> 1) ^ ISO15693_POLYCRC16 : (ResCrc >> 1);
        }
    }
    int res = ((~ResCrc) & 0xFFFF);
    //Serial.println(res);
    return res;
}

/*
 * Inventory, code=01
 *
 * Request format: SOF, Req.Flags, Inventory, AFI (opt.), Mask len, Mask value, CRC16, EOF
 * Response format: SOF, Resp.Flags, DSFID, UID, CRC16, EOF
 *
 */
ISO15693ErrorCode PN5180ISO15693::getInventory(uint8_t *uid)
{
    char buf[32];
    //                     Flags,  CMD, maskLen
    //uint8_t inventory[] = {0x26, 0x01, 0x00};
    uint8_t inventory[] = {0x26, 0x01, 0x08, 0x74};
    //int16_t crc16 = ISO15693_CRC16(inventory,4);

    //inventory[6] = crc16>>8&0xff;
    //inventory[7] = crc16&0xff;

    //                        |\- inventory flag + high data rate
    //                        \-- 1 slot: only one card, no AFI field present
    PN5180DEBUG(F("Get Inventory...\n"));

    for (int i = 0; i < 8; i++)
    {
        uid[i] = 0;
    }

    uint8_t *readBuffer;
    ISO15693ErrorCode rc = issueISO15693Command(inventory, sizeof(inventory), &readBuffer);

    if (ISO15693_EC_OK != rc)
    {
        return rc;
    }

    char a[7] = {0};

    //Serial.println(F("Response flags: "));
    sprintf(a, "0x%02X", readBuffer[0]);
    //Serial.println(a);
    //Serial.println(F(", Data Storage Format ID: "));
    sprintf(a, "0x%02X", readBuffer[1]);
    //Serial.println(a);
    ////Serial.println(formatHex(readBuffer[1]));
    //Serial.println(F(", UID: "));

    //Serial.println("DATA:");
    for (int i = 0; i < 8; i++)
    {
        uid[i] = readBuffer[2 + i];
        sprintf(a, "0x%02X ", uid[i]);
        //Serial.print(a);
#ifdef DEBUG
        PN5180DEBUG(formatHex(uid[7 - i])); // LSB comes first
        if (i < 2)
            PN5180DEBUG(":");
#endif
    }
    //Serial.println("");
    sprintf(a, "CRC16: 0x%02X%02X", readBuffer[10], readBuffer[11]);
    //Serial.println(a);
    PN5180DEBUG("\n");
    // delay(1000000);

    return ISO15693_EC_OK;
}

/*
 * Read single block, code=20
 *
 * Request format: SOF, Req.Flags, ReadSingleBlock, UID (opt.), BlockNumber, CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occured.
  *    03 = The option is not suppored.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
 *
 *  when ERROR flag is NOT set:
 *    SOF, Flags, BlockData (len=blockLength), CRC16, EOF
 */
ISO15693ErrorCode PN5180ISO15693::readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize)
{
    //                            flags, cmd, uid,             blockNo
    uint8_t readSingleBlock[] = {0x62, 0x20, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    //                              |\- high data rate
    //                              \-- options, addressed by UID
    for (int i = 0; i < 8; i++)
    {
        readSingleBlock[2 + i] = uid[i];
    }

#ifdef DEBUG
    PN5180DEBUG("Read Single Block #");
    PN5180DEBUG(blockNo);
    PN5180DEBUG(", size=");
    PN5180DEBUG(blockSize);
    PN5180DEBUG(": ");
    for (int i = 0; i < sizeof(readSingleBlock); i++)
    {
        PN5180DEBUG(" ");
        PN5180DEBUG(formatHex(readSingleBlock[i]));
    }
    PN5180DEBUG("\n");
#endif

    uint8_t *resultPtr;
    ISO15693ErrorCode rc = issueISO15693Command(readSingleBlock, sizeof(readSingleBlock), &resultPtr);
    if (ISO15693_EC_OK != rc)
    {
        return rc;
    }

    PN5180DEBUG("Value=");

    for (int i = 0; i < blockSize; i++)
    {
        blockData[i] = resultPtr[2 + i];
#ifdef DEBUG
        PN5180DEBUG(formatHex(blockData[i]));
        PN5180DEBUG(" ");
#endif
    }

#ifdef DEBUG
    PN5180DEBUG(" ");
    for (int i = 0; i < blockSize; i++)
    {
        char c = blockData[i];
        if (isPrintable(c))
        {
            PN5180DEBUG(c);
        }
        else
            PN5180DEBUG(".");
    }
    PN5180DEBUG("\n");
#endif

    return ISO15693_EC_OK;
}

/*
 * Write single block, code=21
 *
 * Request format: SOF, Requ.Flags, WriteSingleBlock, UID (opt.), BlockNumber, BlockData (len=blcokLength), CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occured.
  *    03 = The option is not suppored.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
 *
 *  when ERROR flag is NOT set:
 *    SOF, Resp.Flags, CRC16, EOF
 */
ISO15693ErrorCode PN5180ISO15693::writeSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize)
{
    //                            flags, cmd, uid,             blockNo
    //uint8_t writeSingleBlock[] = {0x62, 0x21, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    uint8_t writeSingleBlock[] = {0x23, 0x21, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    //                               |\- high data rate
    //                               \-- options, addressed by UID

    uint8_t writeCmdSize = sizeof(writeSingleBlock) + blockSize;
    uint8_t *writeCmd = (uint8_t *)malloc(writeCmdSize);
    uint8_t pos = 0;
    writeCmd[pos++] = writeSingleBlock[0];
    writeCmd[pos++] = writeSingleBlock[1];
    for (int i = 0; i < 8; i++)
    {
        writeCmd[pos++] = uid[i];
    }
    writeCmd[pos++] = blockNo;
    for (int i = 0; i < blockSize; i++)
    {
        writeCmd[pos++] = blockData[i];
    }

#ifdef DEBUG
    PN5180DEBUG("Write Single Block #");
    PN5180DEBUG(blockNo);
    PN5180DEBUG(", size=");
    PN5180DEBUG(blockSize);
    PN5180DEBUG(":");
    for (int i = 0; i < writeCmdSize; i++)
    {
        PN5180DEBUG(" ");
        PN5180DEBUG(formatHex(writeCmd[i]));
    }
    PN5180DEBUG("\n");
#endif

    uint8_t *resultPtr;
    ISO15693ErrorCode rc = issueISO15693Command(writeCmd, writeCmdSize, &resultPtr);
    if (ISO15693_EC_OK != rc)
    {
        free(writeCmd);
        return rc;
    }

    free(writeCmd);
    return ISO15693_EC_OK;
}

/*
 * Get System Information, code=2B
 *
 * Request format: SOF, Req.Flags, GetSysInfo, UID (opt.), CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occured.
  *    03 = The option is not suppored.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
  *
 *  when ERROR flag is NOT set:
 *    SOF, Flags, InfoFlags, UID, DSFID (opt.), AFI (opt.), Other fields (opt.), CRC16, EOF
 *
 *    InfoFlags:
 *    xxxx.3210
 *         |||\_ DSFID: 0=DSFID not supported, DSFID field NOT present; 1=DSFID supported, DSFID field present
 *         ||\__ AFI: 0=AFI not supported, AFI field not present; 1=AFI supported, AFI field present
 *         |\___ VICC memory size:
 *         |        0=Information on VICC memory size is not supported. Memory size field is present. ???
 *         |        1=Information on VICC memory size is supported. Memory size field is present.
 *         \____ IC reference:
 *                  0=Information on IC reference is not supported. IC reference field is not present.
 *                  1=Information on IC reference is supported. IC reference field is not present.
 *
 *    VICC memory size:
 *      xxxb.bbbb nnnn.nnnn
 *        bbbbb - Block size is expressed in number of bytes, on 5 bits, allowing to specify up to 32 bytes i.e. 256 bits.
 *        nnnn.nnnn - Number of blocks is on 8 bits, allowing to specify up to 256 blocks.
 *
 *    IC reference: The IC reference is on 8 bits and its meaning is defined by the IC manufacturer.
 */
ISO15693ErrorCode PN5180ISO15693::getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks)
{
    uint8_t sysInfo[] = {0x22, 0x2b, 1, 2, 3, 4, 5, 6, 7, 8}; // UID has LSB first!
    for (int i = 0; i < 8; i++)
    {
        sysInfo[2 + i] = uid[i];
    }

#ifdef DEBUG
    PN5180DEBUG("Get System Information");
    for (int i = 0; i < sizeof(sysInfo); i++)
    {
        PN5180DEBUG(" ");
        PN5180DEBUG(formatHex(sysInfo[i]));
    }
    PN5180DEBUG("\n");
#endif

    uint8_t *readBuffer;
    ISO15693ErrorCode rc = issueISO15693Command(sysInfo, sizeof(sysInfo), &readBuffer);
    if (ISO15693_EC_OK != rc)
    {
        return rc;
    }

    for (int i = 0; i < 8; i++)
    {
        uid[i] = readBuffer[2 + i];
    }

#ifdef DEBUG
    PN5180DEBUG("UID=");
    for (int i = 0; i < 8; i++)
    {
        PN5180DEBUG(formatHex(readBuffer[9 - i])); // UID has LSB first!
        if (i < 2)
            PN5180DEBUG(":");
    }
    PN5180DEBUG("\n");
#endif

    uint8_t *p = &readBuffer[10];

    uint8_t infoFlags = readBuffer[1];
    if (infoFlags & 0x01)
    { // DSFID flag
        uint8_t dsfid = *p++;
        PN5180DEBUG("DSFID="); // Data storage format identifier
        PN5180DEBUG(formatHex(dsfid));
        PN5180DEBUG("\n");
    }
#ifdef DEBUG
    else
        PN5180DEBUG(F("No DSFID\n"));
#endif

    if (infoFlags & 0x02)
    { // AFI flag
        uint8_t afi = *p++;
        PN5180DEBUG(F("AFI=")); // Application family identifier
        PN5180DEBUG(formatHex(afi));
        PN5180DEBUG(F(" - "));
        switch (afi >> 4)
        {
        case 0:
            PN5180DEBUG(F("All families"));
            break;
        case 1:
            PN5180DEBUG(F("Transport"));
            break;
        case 2:
            PN5180DEBUG(F("Financial"));
            break;
        case 3:
            PN5180DEBUG(F("Identification"));
            break;
        case 4:
            PN5180DEBUG(F("Telecommunication"));
            break;
        case 5:
            PN5180DEBUG(F("Medical"));
            break;
        case 6:
            PN5180DEBUG(F("Multimedia"));
            break;
        case 7:
            PN5180DEBUG(F("Gaming"));
            break;
        case 8:
            PN5180DEBUG(F("Data storage"));
            break;
        case 9:
            PN5180DEBUG(F("Item management"));
            break;
        case 10:
            PN5180DEBUG(F("Express parcels"));
            break;
        case 11:
            PN5180DEBUG(F("Postal services"));
            break;
        case 12:
            PN5180DEBUG(F("Airline bags"));
            break;
        default:
            PN5180DEBUG(F("Unknown"));
            break;
        }
        PN5180DEBUG("\n");
    }
#ifdef DEBUG
    else
        PN5180DEBUG(F("No AFI\n"));
#endif

    if (infoFlags & 0x04)
    { // VICC Memory size
        *numBlocks = *p++;
        *blockSize = *p++;
        *blockSize = (*blockSize) & 0x1f;

        *blockSize = *blockSize + 1; // range: 1-32
        *numBlocks = *numBlocks + 1; // range: 1-256
        uint16_t viccMemSize = (*blockSize) * (*numBlocks);

        PN5180DEBUG("VICC MemSize=");
        PN5180DEBUG(viccMemSize);
        PN5180DEBUG(" BlockSize=");
        PN5180DEBUG(*blockSize);
        PN5180DEBUG(" NumBlocks=");
        PN5180DEBUG(*numBlocks);
        PN5180DEBUG("\n");
    }
#ifdef DEBUG
    else
        PN5180DEBUG(F("No VICC memory size\n"));
#endif

    if (infoFlags & 0x08)
    { // IC reference
        uint8_t icRef = *p++;
        PN5180DEBUG("IC Ref=");
        PN5180DEBUG(formatHex(icRef));
        PN5180DEBUG("\n");
    }
#ifdef DEBUG
    else
        PN5180DEBUG(F("No IC ref\n"));
#endif

    return ISO15693_EC_OK;
}

/*
 * ISO 15693 - Protocol
 *
 * General Request Format:
 *  SOF, Req.Flags, Command code, Parameters, Data, CRC16, EOF
 *
 *  Request Flags:
 *    xxxx.3210
 *         |||\_ Subcarrier flag: 0=single sub-carrier, 1=two sub-carrier
 *         ||\__ Datarate flag: 0=low data rate, 1=high data rate
 *         |\___ Inventory flag: 0=no inventory, 1=inventory
 *         \____ Protocol extension flag: 0=no extension, 1=protocol format is extended
 *
 *  If Inventory flag is set:
 *    7654.xxxx
 *     ||\_ AFI flag: 0=no AFI field present, 1=AFI field is present
 *     |\__ Number of slots flag: 0=16 slots, 1=1 slot
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *
 *  If Inventory flag is NOT set:
 *    7654.xxxx
 *     ||\_ Select flag: 0=request shall be executed by any VICC according to Address_flag
 *     ||                1=request shall be executed only by VICC in selected state
 *     |\__ Address flag: 0=request is not addressed. UID field is not present.
 *     |                  1=request is addressed. UID field is present. Only VICC with UID shall answer
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *
 * General Response Format:
 *  SOF, Resp.Flags, Parameters, Data, CRC16, EOF
 *
 *  Response Flags:
 *    xxxx.3210
 *         |||\_ Error flag: 0=no error, 1=error detected, see error field
 *         ||\__ RFU: 0
 *         |\___ RFU: 0
 *         \____ Extension flag: 0=no extension, 1=protocol format is extended
 *
 *  If Error flag is set, the following error codes are defined:
 *    01 = The command is not supported, i.e. the request code is not recognized.
 *    02 = The command is not recognized, i.e. a format error occured.
 *    03 = The option is not suppored.
 *    0F = Unknown error.
 *    10 = The specific block is not available.
 *    11 = The specific block is already locked and cannot be locked again.
 *    12 = The specific block is locked and cannot be changed.
 *    13 = The specific block was not successfully programmed.
 *    14 = The specific block was not successfully locked.
 *    A0-DF = Custom command error codes
 *
 *  Function return values:
 *    0 = OK
 *   -1 = No card detected
 *   >0 = Error code
 */
ISO15693ErrorCode PN5180ISO15693::issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr, int32_t *rx_status)
{
#ifdef DEBUG
    PN5180DEBUG(F("Issue Command 0x"));
    PN5180DEBUG(formatHex(cmd[1]));
    PN5180DEBUG("...\n");
#endif

    sendData(cmd, cmdLen);
    delay(10);

    if (0 == (getIRQStatus() & RX_SOF_DET_IRQ_STAT))
    {
        return EC_NO_CARD;
    }

    uint32_t rxStatus;
    readRegister(RX_STATUS, &rxStatus);

    PN5180DEBUG(F("RX-Status="));
    PN5180DEBUG(formatHex(rxStatus));

    uint16_t len = (uint16_t)(rxStatus & 0x000001ff);

    if (rx_status)
    {
        *rx_status = rxStatus;
    }

    PN5180DEBUG(", len=");
    PN5180DEBUG(len);
    PN5180DEBUG("\n");

    *resultPtr = readData(len);
    if (0L == *resultPtr)
    {
        PN5180DEBUG(F("*** ERROR in readData!\n"));
        return ISO15693_EC_UNKNOWN_ERROR;
    }

    // #ifdef DEBUG
    //     //Serial.print("Read=");
    //     for (int i = 0; i < len; i++)
    //     {
    //         //Serial.print(formatHex((*resultPtr)[i]));
    //         if (i < len - 1)
    //             //Serial.print(":");
    //     }
    //     //Serial.println();
    // #endif

    uint32_t irqStatus = getIRQStatus();
    if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus))
    { // no card detected
        clearIRQStatus(TX_IRQ_STAT | IDLE_IRQ_STAT);
        return EC_NO_CARD;
    }

    uint8_t responseFlags = (*resultPtr)[0];
    if (responseFlags & (1 << 0))
    { // error flag
        uint8_t errorCode = (*resultPtr)[1];

        PN5180DEBUG("ERROR code=");
        PN5180DEBUG(formatHex(errorCode));
        PN5180DEBUG(" - ");
        // PN5180DEBUG(strerror(errorCode));
        PN5180DEBUG("\n");

        if (errorCode >= 0xA0)
        { // custom command error codes
            return ISO15693_EC_CUSTOM_CMD_ERROR;
        }
        else
            return (ISO15693ErrorCode)errorCode;
    }

#ifdef DEBUG
    if (responseFlags & (1 << 3))
    { // extendsion flag
        PN5180DEBUG("Extension flag is set!\n");
    }
#endif

    clearIRQStatus(RX_SOF_DET_IRQ_STAT | IDLE_IRQ_STAT | TX_IRQ_STAT | RX_IRQ_STAT);
    return ISO15693_EC_OK;
}

bool PN5180ISO15693::setupRF()
{
    PN5180DEBUG(F("Loading RF-Configuration...\n"));

    if (loadRFConfig(0x00, 0x80))
    { // ISO15693 parameters
        PN5180DEBUG(F("done.\n"));
    }
    else
        return false;

    PN5180DEBUG(F("Turning ON RF field...\n"));
    if (setRF_on())
    {
        PN5180DEBUG(F("done.\n"));
    }
    else
        return false;

    writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8); // Idle/StopCom Command
    writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);  // Transceive Command

    return true;
}



const __FlashStringHelper *PN5180ISO15693::strerror(ISO15693ErrorCode errno)
{
    PN5180DEBUG(F("ISO15693ErrorCode="));
    PN5180DEBUG(errno);
    PN5180DEBUG("\n");

    switch (errno)
    {
    case EC_NO_CARD:
        return F("No card detected!");
    case ISO15693_EC_OK:
        return F("OK!");
    case ISO15693_EC_NOT_SUPPORTED:
        return F("Command is not supported!");
    case ISO15693_EC_NOT_RECOGNIZED:
        return F("Command is not recognized!");
    case ISO15693_EC_OPTION_NOT_SUPPORTED:
        return F("Option is not suppored!");
    case ISO15693_EC_UNKNOWN_ERROR:
        return F("Unknown error!");
    case ISO15693_EC_BLOCK_NOT_AVAILABLE:
        return F("Specified block is not available!");
    case ISO15693_EC_BLOCK_ALREADY_LOCKED:
        return F("Specified block is already locked!");
    case ISO15693_EC_BLOCK_IS_LOCKED:
        return F("Specified block is locked and cannot be changed!");
    case ISO15693_EC_BLOCK_NOT_PROGRAMMED:
        return F("Specified block was not successfully programmed!");
    case ISO15693_EC_BLOCK_NOT_LOCKED:
        return F("Specified block was not successfully locked!");
    default:
        if ((errno >= 0xA0) && (errno <= 0xDF))
        {
            return F("Custom command error code!");
        }
        else
            return F("Undefined error code in ISO15693!");
    }
}

void PN5180ISO15693::search_all()
{
    MyStack stack(100);
    SearchData start(0, 0, 0);
    stack.push(start);
    //Serial.println(stack.size());

    // delay(1000);
    SearchData tmp;
    SearchData to_push;
    uint8_t ret;
    int64_t uid;

    int64_t mask_to_search;
    uint8_t mask_len_to_search;

    while (!stack.empty())
    {

        stack.pop(tmp);

        if (tmp.current_bit_value)
        {
            (tmp.mask) |= (tmp.current_bit_value << tmp.position);
        }
        else
        {
            (tmp.mask) &= ~(tmp.current_bit_value << tmp.position);
        }
        mask_len_to_search = tmp.position + 1;

        //ISO15693ErrorCode ec = search_once(tmp.mask, mask_len_to_search, &ret, &uid);
        ISO15693ErrorCode ec = search_once(tmp.mask, mask_len_to_search, ret, uid);
        ////Serial.print("RET= ");
        ////Serial.println(ret);
        if (tmp.current_bit_value == 0)
        {
            to_push.mask = tmp.mask;
            to_push.position = tmp.position;
            to_push.current_bit_value = (uint8_t)1;
            stack.push(to_push);
        }

        if (ret == 0)
        {
        }
        else if (ret == 1)
        {
            m_uidvec.insert(uid);
            quiet(uid);
            //Serial.println("FOUND");
        }
        else
        {

            //SearchData test(tmp.mask, tmp.position + (uint8_t)1, (uint8_t)0);
            to_push.mask = tmp.mask;
            to_push.position = tmp.position + (uint8_t)1;
            to_push.current_bit_value = (uint8_t)0;
            stack.push(to_push);
        }

        // delay(1000);
        // this->reset();
        // this->setupRF();

        //Serial.print("STACK SIZE = ");
        //Serial.println(stack.size());
    }

    // Serial.print("count: ");
    // Serial.println(m_uidvec.size());
    // for (int i = 0; i < m_uidvec.size(); i++)
    // {
    //    Serial.println(formatHex(m_uidvec[i]));
    //    Serial.println(*(uint8_t*)&m_uidvec[i]);
    // }

    // m_uidvec.clear();
}

/*
 * Get System Information, code=2B
 *
 * Request format: SOF, Req.Flags, GetSysInfo, UID (opt.), CRC16, EOF
 * Response format:
 *  when ERROR flag is set:
 *    SOF, Resp.Flags, ErrorCode, CRC16, EOF
 *
 *     Response Flags:
  *    xxxx.3xx0
  *         |||\_ Error flag: 0=no error, 1=error detected, see error field
  *         \____ Extension flag: 0=no extension, 1=protocol format is extended
  *
  *  If Error flag is set, the following error codes are defined:
  *    01 = The command is not supported, i.e. the request code is not recognized.
  *    02 = The command is not recognized, i.e. a format error occured.
  *    03 = The option is not suppored.
  *    0F = Unknown error.
  *    10 = The specific block is not available.
  *    11 = The specific block is already locked and cannot be locked again.
  *    12 = The specific block is locked and cannot be changed.
  *    13 = The specific block was not successfully programmed.
  *    14 = The specific block was not successfully locked.
  *    A0-DF = Custom command error codes
  *
 *  when ERROR flag is NOT set:
 *    SOF, Flags, InfoFlags, UID, DSFID (opt.), AFI (opt.), Other fields (opt.), CRC16, EOF
 *
 *    InfoFlags:
 *    xxxx.3210
 *         |||\_ DSFID: 0=DSFID not supported, DSFID field NOT present; 1=DSFID supported, DSFID field present
 *         ||\__ AFI: 0=AFI not supported, AFI field not present; 1=AFI supported, AFI field present
 *         |\___ VICC memory size:
 *         |        0=Information on VICC memory size is not supported. Memory size field is present. ???
 *         |        1=Information on VICC memory size is supported. Memory size field is present.
 *         \____ IC reference:
 *                  0=Information on IC reference is not supported. IC reference field is not present.
 *                  1=Information on IC reference is supported. IC reference field is not present.
 *
 *    VICC memory size:
 *      xxxb.bbbb nnnn.nnnn
 *        bbbbb - Block size is expressed in number of bytes, on 5 bits, allowing to specify up to 32 bytes i.e. 256 bits.
 *        nnnn.nnnn - Number of blocks is on 8 bits, allowing to specify up to 256 blocks.
 *
 *    IC reference: The IC reference is on 8 bits and its meaning is defined by the IC manufacturer.
 */
ISO15693ErrorCode PN5180ISO15693::getSystemInfo(const int64_t &uid, uint8_t &blockSize, uint8_t &numBlocks)
{
    uint8_t sysInfo[] = {0x22, 0x2b, 1, 2, 3, 4, 5, 6, 7, 8}; // UID has LSB first!
    for (int i = 0; i < 8; i++)
    {
        sysInfo[2 + i] = ((uint8_t *)&uid)[i];
    }

    uint8_t *readBuffer;
    ISO15693ErrorCode rc = issueISO15693Command(sysInfo, sizeof(sysInfo), &readBuffer);
    if (ISO15693_EC_OK != rc)
    {
        return rc;
    }

    // for (int i = 0; i < 8; i++)
    // {
    //     uid[i] = readBuffer[2 + i];
    // }

    uint8_t *p = &readBuffer[10];

    uint8_t infoFlags = readBuffer[1];
    if (infoFlags & 0x01)
    { // DSFID flag
        uint8_t dsfid = *p++;
    }

    if (infoFlags & 0x02)
    { // AFI flag
        uint8_t afi = *p++;
    }

    if (infoFlags & 0x04)
    { // VICC Memory size
        numBlocks = *p++;
        blockSize = *p++;
        blockSize = (blockSize)&0x1f;

        blockSize = blockSize + 1; // range: 1-32
        numBlocks = numBlocks + 1; // range: 1-256
        //uint16_t viccMemSize = (blockSize) * (numBlocks);
    }

    if (infoFlags & 0x08)
    { // IC reference
        uint8_t icRef = *p++;
    }

    return ISO15693_EC_OK;
}

ISO15693ErrorCode PN5180ISO15693::readSingleBlock(const int64_t &uid, const uint8_t &blockNo, uint8_t *blockData, const uint8_t &blockSize)
{
    //                            flags, cmd, uid,             blockNo
    uint8_t sendbuf[] = {0x62, 0x20, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    //                              |\- high data rate
    //                              \-- options, addressed by UID
    for (int i = 0; i < 8; i++)
    {
        sendbuf[2 + i] = ((uint8_t *)&uid)[i];
    }
    int32_t len;

    uint8_t *resultPtr;
    ISO15693ErrorCode rc = issueISO15693Command(sendbuf, sizeof(sendbuf), &resultPtr, &len);
    if (ISO15693_EC_OK != rc)
    {
        return rc;
    }

    for (int i = 0; i < blockSize; i++)
    {
        blockData[i] = resultPtr[2 + i];
    }

    Serial.print("resultPtr = ");

    char buf[256];
    sprintf(buf, "resultlen: %d, 0:%02X,1:%02X,data:%02X%02X%02X%02X", len, resultPtr[0], resultPtr[1], resultPtr[2], resultPtr[3], resultPtr[4], resultPtr[5]);

    //Serial.print(resultPtr[7]);
    Serial.println(buf);

    return ISO15693_EC_OK;
}

ISO15693ErrorCode PN5180ISO15693::readSingleBlock(const int64_t &uid, const uint8_t &blockNo, uint64_t &blockData)
{
    readSingleBlock(uid, blockNo, (uint8_t *)&blockData, 4);
}

ISO15693ErrorCode PN5180ISO15693::writeSingleBlock(const int64_t &uid, const uint8_t &blockNo, uint8_t *blockData, const uint8_t &blockSize)
{
    //                            flags, cmd, uid,             blockNo
    //uint8_t writeSingleBlock[] = {0x62, 0x21, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    uint8_t head[] = {0x23, 0x21, 1, 2, 3, 4, 5, 6, 7, 8, blockNo}; // UID has LSB first!
    //                               |\- high data rate
    //                               \-- options, addressed by UID

    uint8_t buffersize = sizeof(head) + blockSize;
    //uint8_t *writeCmd = (uint8_t *)malloc(writeCmdSize);
    uint8_t *sendbuf = new uint8_t[buffersize];

    uint8_t pos = 0;
    sendbuf[pos++] = head[0];
    sendbuf[pos++] = head[1];
    for (int i = 0; i < 8; i++)
    {
        sendbuf[pos++] = ((uint8_t *)&uid)[i];
    }
    sendbuf[pos++] = blockNo;
    for (int i = 0; i < blockSize; i++)
    {
        sendbuf[pos++] = blockData[i];
    }

    uint8_t *resultPtr;
    ISO15693ErrorCode rc = issueISO15693Command(sendbuf, buffersize, &resultPtr);
    if (ISO15693_EC_OK != rc)
    {
        delete[] sendbuf;
        return rc;
    }

    delete[] sendbuf;
    return ISO15693_EC_OK;
}

ISO15693ErrorCode PN5180ISO15693::writeSingleBlock(const int64_t &uid, const uint8_t &blockNo, uint64_t &blockData)
{
    writeSingleBlock(uid, blockNo, (uint8_t *)&blockData, 4);
}

/*
    * @parm mask mask
    * @parm mask_length mask_length(bit)  掩码长度(Mask Length)指示了需要的比较的字节数，范围为[0x00，0x40]
    * @parm return ret_val 0无卡，1 1张卡,其他 多张卡
    * @parm return result_ptr ret_val==1 uid,其他为空
    * @return errno 
    * 
     */
ISO15693ErrorCode PN5180ISO15693::search_once(const int64_t &mask, const uint8_t &mask_length, uint8_t &ret_val, int64_t &result_ptr)
{
    delay(1);
    //Serial.print("start search_once :");
    //Serial.print("mask:");
    //Serial.print(mask);
    //Serial.print(" ,mask_length:");
    //Serial.println(mask_length);

    uint8_t inventory[11];
    inventory[0] = 0x26;
    inventory[1] = 0x01;
    inventory[2] = mask_length;

    uint8_t mask_byte_length = (mask_length + 7) / 8;
    for (int i = 0; i < mask_byte_length; i++)
    {
        inventory[3 + i] = ((uint8_t *)&mask)[i];
    }

    uint8_t send_len = mask_byte_length + 3;
    uint8_t *readBuffer;
    int32_t rx_status;

    ISO15693ErrorCode rc = issueISO15693Command(inventory, send_len, &readBuffer, &rx_status);

    ////Serial.println(rx_status);

    if (ISO15693_EC_OK != rc)
    {
        //Serial.println("nocard return 0");
        ret_val = 0;
        return rc;
    }

    if (rx_status & RX_COLLISION_DETECTED)
    {
        //Serial.println("multicard return 2");
        ret_val = 2;
        return rc;
    }

    //Serial.print("DATA:");
    for (int i = 0; i < 8; i++)
    {
        //result_ptr[i] = readBuffer[2 + i];
        // sprintf(a, "0x%02X ", uid[i]);
        // //Serial.print(a);
    }

    for (int i = 0; i < 8; i++)
    {
        ((uint8_t *)&result_ptr)[i] = readBuffer[2 + i];
    }
    //Serial.println(result_ptr);

    ret_val = 1;
    //Serial.println("one card");
    return ISO15693_EC_OK;
}
//quiet之后继续readblock可以读出数据。quiet之后继续搜卡无法搜到，需要reset。
void PN5180ISO15693::quiet(const int64_t &uid)
{
    uint8_t sendbuf[10] = {0b00100010, 0x02};
    for (int i = 0; i < 8; i++)
    {
        sendbuf[2 + i] = ((uint8_t *)&uid)[i];
    }
    uint8_t *readBuffer;
    issueISO15693Command(sendbuf, sizeof(sendbuf), &readBuffer);
}

int32_t PN5180ISO15693::calc_point_once()
{
    int32_t res = 0;
    uint8_t data[4];
    ISO15693ErrorCode ec;
    for (int i = 0; i < m_uidvec.size(); i++)
    {
        //ec = readSingleBlock(m_uidvec[i], 1, data);
        ec = readSingleBlock(m_uidvec[i], 1, data, 4);
        // Serial.print("data: ");
        // Serial.println(data[0]);
        // Serial.println(data[1]);
        // Serial.println(data[2]);
        // Serial.println(data[3]);
        if (ec == 0)
        {
            res += data[0] * 65536 + data[1] * 4096 + data[2] * 256 + data[3];
        }
        else
        {
            Serial.println("read failed");
        }
    }
    return res;
}

int32_t PN5180ISO15693::calc_point()
{
    int showValue = 0;
    int showTimes = 0;
    int count;
    for (;;)
    {
        if (showTimes >= 2)
        {
            return showValue;
        }

        count = calc_point_once();
        if (showValue == count)
        {
            showTimes++;
        }
        else
        {
            showTimes = 0;
            showValue = count;
        }
    }
    //return -1;
}

char *PN5180ISO15693::formatHex(uint64_t val)
{
    static const char hexChar[] = "0123456789ABCDEF";
    static char hexBuffer[17];
    for (int i = 15; i >= 0; i--)
    {
        hexBuffer[i] = hexChar[val & 0x0f];
        val = val >> 4;
    }
    hexBuffer[16] = '\0';
    return hexBuffer;
}
