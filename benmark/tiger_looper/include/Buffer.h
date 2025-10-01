/**
 * @copyright
 * Copyright (c) 2015 by LG Electronics Inc. @n
 * This program or software including the accompanying associated documentation @n
 * ("Software") is the proprietary software of LG Electronics Inc. and or its @n
 * licensors, and may only be used, duplicated, modified or distributed pursuant @n
 * to the terms and conditions of a separate written license agreement between you @n
 * and LG Electronics Inc. ("Authorized License"). Except as set forth in an @n
 * Authorized License, LG Electronics Inc. grants no license (express or implied), @n
 * rights to use, or waiver of any kind with respect to the Software, and LG @n
 * Electronics Inc. expressly reserves all rights in and to the Software and all @n
 * intellectual property therein. If you have no Authorized License, then you have @n
 * no rights to use the Software in any ways, and should immediately notify LG @n
 * Electronics Inc. and discontinue all use of the Software.
 *
 * @file
 * Buffer.h
 *
 * Declration of Buffer Class.
 */

/**
 * @defgroup Library Runtime Library
 */

/**
 * @defgroup Buffer buffer
 * @ingroup Library
 * All of input and output will contain into the Buffer. @n
 * You can call this function anywhere, using #include <utils/Buffer.h>
 */


#ifndef COMMUNICATION_BUFFER_H
#define COMMUNICATION_BUFFER_H

#include <utils/RefBase.h>

#include "Typedef.h"

/**
 * All of input and output will contain into the Buffer. @n
 * This inheit RefBase so that we can use sp for this.
 *
 * @author
 * sungwoo.oh
 * @date
 * 2015.08.04
 * @version
 * 1.00.000
 */
class Buffer: public android::RefBase
{

public:
    /**
    * @ingroup Buffer
    * Ctor of Buffer
    */
    Buffer();

    /**
    * Ctor of Buffer
    *
    * @param[in] other  other Buffer
    */
    Buffer(Buffer& other);
    virtual ~Buffer();

    /**
    * @ingroup Buffer
    * Fill block of memory. @n
    * Sets the first len bytes of the block of memory pointed by Buf to the specified NULL. @n
    * Buf is member variable in Buffer Class.
    *
    * @param[in] len  Number of bytes to be set to the value.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf->setSize(len);
      @endverbatim
    */
    void setSize(int32_t len);

    /**
    * @ingroup Buffer
    * The assignment operator. Copy other to mBuf. @n
    * Buf is member variable in Buffer Class.
    *
    * @param[in] other  other Buffer
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      int32_t len = ...;
      sp<Buffer> temp_buf = new Buffer();
      temp_buf->setSize(len);
      sp<Buffer> buf = new Buffer();
      buf = temp_buf;
      @endverbatim
    */
    Buffer& operator=(const Buffer& other);

    /**
    * @ingroup Buffer
    * The assignment operator. Copy other to Buf. @n
    * Buf is member variable in Buffer Class
    *
    * @param[in] other  other Buffer
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      int32_t len = ...;
      sp<Buffer> temp_buf = new Buffer();
      temp_buf->setSize(len);
      sp<Buffer> buf = new Buffer();
      buf-> setTo(temp_buf);
      @endverbatim
    */
    void setTo(const Buffer& buffer);

    /**
    * @ingroup Buffer
    * Fill block of memory. @n
    * Sets the first len bytes of the block of memory pointed by Buf to the specified buf. @n
    * Buf is member variable in Buffer Class.
    *
    * @param[in] buf  Value to be set.
    * @param[in] len  Number of bytes to be set to the value.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      uint8_t* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      @endverbatim
    */
    void setTo(uint8_t* buf, int32_t len);

    /**
    * @ingroup Buffer
    * Fill block of memory. @n
    * Sets the first len bytes of the block of memory pointed by Buf to the specified buf. @n
    * Buf is member variable in Buffer Class.
    *
    * @param[in] buf  Value to be set.
    * @param[in] len  Number of bytes to be set to the value.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      @endverbatim
    */
    void setTo(char* buf, int32_t len);

    /**
    * @ingroup Buffer
    * Extends the Buf by appending additional value as specified length.
    * Buf is member variable in Buffer Class.
    *
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      uint8_t* tmp_data = ...;
      int32_t tmp_len = ...;
      buf-> append(tmp_data, tmp_len);
      @endverbatim
    */
    void append(uint8_t* buf, int32_t len);

    /**
    * @ingroup Buffer
    * get a Buf @n
    * Buf is member variable in Buffer Class.
    *
    * @return Buf is that Pointer to the block of memory to fill.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      uint8_t* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      uint8_t* buf_data = buf->data();
      @endverbatim
    */
    uint8_t* data();

    /**
    * @ingroup Buffer
    * get a Buf's size
    * Buf is member variable in Buffer Class.
    *
    * @return Buf's size
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      int32_t buf_len = buf->size();
      @endverbatim
    */
    uint32_t size() const;

    /**
    * @ingroup Buffer
    * get whether the Buf is empty. @n
    * Buf is member variable in Buffer Class.
    *
    * @retval true  Buf is empty
    * @retval false  Buf is not empty
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      bool is_buf = buf->empty();
      @endverbatim
    */
    bool empty();

    /**
    * @ingroup Buffer
    * Show Buf contents in log. @n
    * Buf is member variable in Buffer Class.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      buf->dump();
      @endverbatim
    */
    void dump();

    /**
    * @ingroup Buffer
    * show buf contents for specified length in log.
    *
    * @param[in] s  specified buf
    * @param[in] len  specified length
    */
    void dump(uint8_t* s, int32_t len);

    /**
    * @ingroup Buffer
    * Remove Buf contents. @n
    * Buf is member variable in Buffer Class.
    * @note The following function can be used for this:
      @verbatim
      #include <utils/Buffer.h>

      char* data = ...;
      int32_t len = ...;
      sp<Buffer> buf = new Buffer();
      buf-> setTo(data, len);
      buf->clear();
      @endverbatim
    */
    void clear();

private:
    void _assign(uint32_t size);
    void _release();

private:
    uint8_t* mBuf;
    uint32_t mSize;
    bool mClear;
};
#endif // COMMUNICATION_BUFFER_H