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
#ifndef PN5180ISO15693_H
#define PN5180ISO15693_H

#include "PN5180.h"
#include "MyStd.h"

#define ISO15693_POLYCRC16 0x8408
#define ISO15693_MASKCRC16 0x0001
#define ISO15693_PRELOADCRC16 0xFFFF
#define RX_COLLISION_DETECTED (1<<18)

enum ISO15693ErrorCode
{
    EC_NO_CARD = -1,
    ISO15693_EC_OK = 0,
    ISO15693_EC_NOT_SUPPORTED = 0x01,
    ISO15693_EC_NOT_RECOGNIZED = 0x02,
    ISO15693_EC_OPTION_NOT_SUPPORTED = 0x03,
    ISO15693_EC_UNKNOWN_ERROR = 0x0f,
    ISO15693_EC_BLOCK_NOT_AVAILABLE = 0x10,
    ISO15693_EC_BLOCK_ALREADY_LOCKED = 0x11,
    ISO15693_EC_BLOCK_IS_LOCKED = 0x12,
    ISO15693_EC_BLOCK_NOT_PROGRAMMED = 0x13,
    ISO15693_EC_BLOCK_NOT_LOCKED = 0x14,
    ISO15693_EC_CUSTOM_CMD_ERROR = 0xA0
};

class PN5180ISO15693 : public PN5180
{

public:
    PN5180ISO15693(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin);

public:
    ISO15693ErrorCode getInventory(uint8_t *uid);

    ISO15693ErrorCode readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize);
    ISO15693ErrorCode writeSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *blockData, uint8_t blockSize);

    ISO15693ErrorCode getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks);

    int16_t ISO15693_CRC16(uint8_t *DataIn, int NbByte);

    bool setupRF();

    /*
    * 查找所有卡片 结果放入m_uidvec
    */
    void search_all();

    /*
    * @parm mask mask
    * @parm mask_length mask_length(bit)  掩码长度(Mask Length)指示了需要的比较的字节数，范围为[0x00，0x40]
    * @parm ret_val 0无卡，1 1张卡,其他 多张卡
    * @parm result_ptr ret_val==1 uid,其他为空
    * @return errno 
    * 
    */
    ISO15693ErrorCode search_once(const int64_t& mask,const uint8_t& mask_length,uint8_t& ret_val,int64_t& result_ptr);

    //quiet uid
    void quiet(const int64_t& uid);

/*


 */

    ISO15693ErrorCode getSystemInfo(const int64_t& uid, uint8_t& blockSize, uint8_t& numBlocks);

    /*
     *  
     *  @parm uid
     *  @parm blockNo
     *  @parm return blockData
     *  @parm blockSize blockData size
     */
    ISO15693ErrorCode readSingleBlock(const int64_t& uid, const uint8_t& blockNo, uint8_t* blockData, const uint8_t& blockSize);
    ISO15693ErrorCode readSingleBlock(const int64_t& uid, const uint8_t& blockNo, uint64_t& blockData);

    ISO15693ErrorCode writeSingleBlock(const int64_t& uid, const uint8_t& blockNo, uint8_t *blockData, const uint8_t& blockSize);
    ISO15693ErrorCode writeSingleBlock(const int64_t& uid, const uint8_t& blockNo, uint64_t& blockData);

    int32_t calc_point();
    int32_t calc_point_once();
    
    /*
    * Helper functions
    */
    char *formatHex(uint64_t val);
public:
    ISO15693ErrorCode issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr, int32_t* rx_status = nullptr);

public:
    
    const __FlashStringHelper *strerror(ISO15693ErrorCode errno);
    
    UidVec m_uidvec;
    
};




#endif /* PN5180ISO15693_H */
