// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 BString.cpp - String library for Wiring & Arduino
 ...mostly rewritten by Paul Stoffregen...
 Copyright (c) 2009-10 Hernando Barragan.  All rights reserved.
 Copyright 2011, Paul Stoffregen, paul@pjrc.com
 Modified by Ivan Grokhotkov, 2014 - esp8266 support
 Modified by Michael C. Miller, 2015 - esp8266 progmem support
 Modified by Ulrich Hecht - binary data support
 Copyright (c) 2018, 2019 Ulrich Hecht

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define __GNU_VISIBLE 1
#include <Arduino.h>
#include "BString.h"
#include "stdlib_noniso.h"

/*********************************************/
/*  Constructors                             */
/*********************************************/

BString::BString(const char *cstr) {
    init();
    if(cstr)
        copy(cstr, strlen(cstr));
}

BString::BString(const BString &value) {
    init();
    *this = value;
}

BString::BString(const __FlashStringHelper *pstr) {
    init();
    *this = pstr; // see operator =
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
BString::BString(BString &&rval) {
    init();
    move(rval);
}

BString::BString(BStringSumHelper &&rval) {
    init();
    move(rval);
}
#endif

BString::BString(char c) {
    init();
    if (!reserve(1))
      return;
    buffer[0] = c;
    buffer[1] = 0;
    len = 1;
}

BString::BString(unsigned char value, unsigned char base) {
    init();
    char buf[1 + 8 * sizeof(unsigned char)];
    utoa(value, buf, base);
    *this = buf;
}

BString::BString(int value, unsigned char base) {
    init();
    char buf[2 + 8 * sizeof(int)];
    itoa(value, buf, base);
    *this = buf;
}

BString::BString(unsigned int value, unsigned char base) {
    init();
    char buf[1 + 8 * sizeof(unsigned int)];
    utoa(value, buf, base);
    *this = buf;
}

BString::BString(long value, unsigned char base) {
    init();
    char buf[2 + 8 * sizeof(long)];
    ltoa(value, buf, base);
    *this = buf;
}

BString::BString(unsigned long value, unsigned char base) {
    init();
    char buf[1 + 8 * sizeof(unsigned long)];
    ultoa(value, buf, base);
    *this = buf;
}

BString::BString(float value, unsigned char decimalPlaces) {
    init();
    char buf[33];
    *this = dtostrf(value, (decimalPlaces + 2), decimalPlaces, buf);
}

BString::BString(double value, unsigned char decimalPlaces) {
    init();
    char buf[33];
    *this = dtostrf(value, (decimalPlaces + 2), decimalPlaces, buf);
}

BString::~BString() {
    if(buffer) {
        free(buffer);
    }
    init();
}

// /*********************************************/
// /*  Memory Management                        */
// /*********************************************/

inline void BString::init(void) {
    buffer = NULL;
    capacity = 0;
    len = 0;
}

void BString::invalidate(void) {
    if(buffer)
        free(buffer);
    init();
}

unsigned char BString::reserve(unsigned int size) {
    if(buffer && capacity >= size)
        return 1;
    if(changeBuffer(size)) {
        if(len == 0)
            buffer[0] = 0;
        return 1;
    }
    return 0;
}

unsigned char BString::changeBuffer(unsigned int maxStrLen) {
    size_t newSize = (maxStrLen + 16) & (~0xf);
    char *newbuffer = NULL;
#ifdef ESP8266
    if (newSize <= 262144)
      // umm seems to crap out instead of reporting an error if you allocate
      // anything larger than about 9MB for some reason...
      // Weirdly enough, this does not seem to happen in variable.h, for
      // example.
#endif
    newbuffer = (char *) realloc(buffer, newSize);
    if(newbuffer) {
        size_t oldSize = capacity + 1; // include NULL.
        if (newSize > oldSize)
        {
            memset(newbuffer + oldSize, 0, newSize - oldSize);
        }
        capacity = newSize - 1;
        buffer = newbuffer;
        return 1;
    }
    // realloc() failed so buffer remains
    // untouched/valid
    return 0;
}

// /*********************************************/
// /*  Copy and Move                            */
// /*********************************************/

BString & BString::copy(const char *cstr, unsigned int length) {
    if(!reserve(length)) {
        invalidate();
        return *this;
    }
    len = length;
    memcpy(buffer, cstr, length);
    buffer[length] = 0;
    return *this;
}

BString & BString::copy(const __FlashStringHelper *pstr, unsigned int length) {
    if (!reserve(length)) {
        invalidate();
        return *this;
    }
    len = length;
    memcpy_P(buffer, (PGM_P)pstr, length);
    buffer[length] = 0;
    return *this;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
void BString::move(BString &rhs) {
    if(buffer) {
        if(capacity >= rhs.len) {
            memcpy(buffer, rhs.buffer, rhs.len);
            buffer[rhs.len] = 0;
            len = rhs.len;
            rhs.len = 0;
            return;
        } else {
            free(buffer);
        }
    }
    buffer = rhs.buffer;
    capacity = rhs.capacity;
    len = rhs.len;
    rhs.buffer = NULL;
    rhs.capacity = 0;
    rhs.len = 0;
}
#endif

BString & BString::operator =(const BString &rhs) {
    if(this == &rhs)
        return *this;

    if(rhs.buffer)
        copy(rhs.buffer, rhs.len);
    else
        invalidate();

    return *this;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
BString & BString::operator =(BString &&rval) {
    if(this != &rval)
        move(rval);
    return *this;
}

BString & BString::operator =(BStringSumHelper &&rval) {
    if(this != &rval)
        move(rval);
    return *this;
}
#endif

BString & BString::operator =(const char *cstr) {
    if(cstr)
        copy(cstr, strlen(cstr));
    else
        invalidate();

    return *this;
}

BString & BString::operator = (const __FlashStringHelper *pstr)
{
    if (pstr) copy(pstr, strlen_P((PGM_P)pstr));
    else invalidate();

    return *this;
}

// /*********************************************/
// /*  concat                                   */
// /*********************************************/

unsigned char BString::concat(const BString &s) {
    return concat(s.buffer, s.len);
}

unsigned char BString::concat(const char *cstr, unsigned int length) {
    unsigned int newlen = len + length;
    if(!cstr)
        return 0;
    if(length == 0)
        return 1;
    if(!reserve(newlen))
        return 0;
    memcpy(buffer + len, cstr, length);
    buffer[newlen] = 0;
    len = newlen;
    return 1;
}

unsigned char BString::concat(const char *cstr) {
    if(!cstr)
        return 0;
    return concat(cstr, strlen(cstr));
}

unsigned char BString::concat(char c) {
    char buf[2];
    buf[0] = c;
    buf[1] = 0;
    return concat(buf, 1);
}

unsigned char BString::concat(unsigned char num) {
    char buf[1 + 3 * sizeof(unsigned char)];
    itoa(num, buf, 10);
    return concat(buf, strlen(buf));
}

unsigned char BString::concat(int num) {
    char buf[2 + 3 * sizeof(int)];
    itoa(num, buf, 10);
    return concat(buf, strlen(buf));
}

unsigned char BString::concat(unsigned int num) {
    char buf[1 + 3 * sizeof(unsigned int)];
    utoa(num, buf, 10);
    return concat(buf, strlen(buf));
}

unsigned char BString::concat(long num) {
    char buf[2 + 3 * sizeof(long)];
    ltoa(num, buf, 10);
    return concat(buf, strlen(buf));
}

unsigned char BString::concat(unsigned long num) {
    char buf[1 + 3 * sizeof(unsigned long)];
    ultoa(num, buf, 10);
    return concat(buf, strlen(buf));
}

unsigned char BString::concat(float num) {
    char buf[20];
    char* string = dtostrf(num, 4, 2, buf);
    return concat(string, strlen(string));
}

unsigned char BString::concat(double num) {
    char buf[20];
    char* string = dtostrf(num, 4, 2, buf);
    return concat(string, strlen(string));
}

unsigned char BString::concat(const __FlashStringHelper * str) {
    if (!str) return 0;
    int length = strlen_P((PGM_P)str);
    if (length == 0) return 1;
    unsigned int newlen = len + length;
    if (!reserve(newlen)) return 0;
    strcpy_P(buffer + len, (PGM_P)str);
    len = newlen;
    return 1;
}

/*********************************************/
/*  Concatenate                              */
/*********************************************/

BStringSumHelper & operator +(const BStringSumHelper &lhs, const BString &rhs) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(rhs.buffer, rhs.len))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, const char *cstr) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!cstr || !a.concat(cstr, strlen(cstr)))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, char c) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(c))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned char num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, int num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned int num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, long num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned long num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, float num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator +(const BStringSumHelper &lhs, double num) {
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if(!a.concat(num))
        a.invalidate();
    return a;
}

BStringSumHelper & operator + (const BStringSumHelper &lhs, const __FlashStringHelper *rhs)
{
    BStringSumHelper &a = const_cast<BStringSumHelper&>(lhs);
    if (!a.concat(rhs))	a.invalidate();
    return a;
}

// /*********************************************/
// /*  Comparison                               */
// /*********************************************/

int BString::compareTo(const BString &s) const {
    if(!buffer || !s.buffer) {
        if(s.buffer && s.len > 0)
            return 0 - *(unsigned char *) s.buffer;
        if(buffer && len > 0)
            return *(unsigned char *) buffer;
        return 0;
    }

    if (len > s.len)
      return buffer[s.len];
    else if (s.len > len)
      return -s.buffer[len];

    return memcmp(buffer, s.buffer, len);
}

unsigned char BString::equals(const BString &s2) const {
    return (len == s2.len && compareTo(s2) == 0);
}

unsigned char BString::equals(const char *cstr) const {
    if(len == 0)
        return (cstr == NULL || *cstr == 0);
    if(cstr == NULL)
        return buffer[0] == 0;
    return strcmp(buffer, cstr) == 0;
}

unsigned char BString::operator<(const BString &rhs) const {
    return compareTo(rhs) < 0;
}

unsigned char BString::operator>(const BString &rhs) const {
    return compareTo(rhs) > 0;
}

unsigned char BString::operator<=(const BString &rhs) const {
    return compareTo(rhs) <= 0;
}

unsigned char BString::operator>=(const BString &rhs) const {
    return compareTo(rhs) >= 0;
}

unsigned char BString::equalsIgnoreCase(const BString &s2) const {
    if(this == &s2)
        return 1;
    if(len != s2.len)
        return 0;
    if(len == 0)
        return 1;
    const char *p1 = buffer;
    const char *p2 = s2.buffer;
    int c = len;
    while(c--) {
        if(tolower(*p1++) != tolower(*p2++))
            return 0;
    }
    return 1;
}

unsigned char BString::startsWith(const BString &s2) const {
    if(len < s2.len)
        return 0;
    return startsWith(s2, 0);
}

unsigned char BString::startsWith(const BString &s2, unsigned int offset) const {
    if(offset > len - s2.len || !buffer || !s2.buffer)
        return 0;
    return memcmp(&buffer[offset], s2.buffer, s2.len) == 0;
}

unsigned char BString::endsWith(const BString &s2) const {
    if(len < s2.len || !buffer || !s2.buffer)
        return 0;
    return memcmp(&buffer[len - s2.len], s2.buffer, s2.len) == 0;
}

// /*********************************************/
// /*  Character Access                         */
// /*********************************************/

char BString::charAt(unsigned int loc) const {
    return operator[](loc);
}

void BString::setCharAt(unsigned int loc, char c) {
    if(loc < len)
        buffer[loc] = c;
}

char & BString::operator[](unsigned int index) {
    static char dummy_writable_char;
    if(index >= len || !buffer) {
        // XXX: extend buffer instead
        dummy_writable_char = 0;
        return dummy_writable_char;
    }
    return buffer[index];
}

char BString::operator[](unsigned int index) const {
    if(index >= len || !buffer)
        return 0;
    return buffer[index];
}

utf8_int32_t BString::codepointAt(unsigned int index) const {
    char *start = buffer;
    while (start && *start && index--) {
        start += utf8codepointcalcsize(start);
    }
    utf8_int32_t c;
    utf8codepoint(start, &c);
    return c;
}

void BString::getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index) const {
    if(!bufsize || !buf)
        return;
    if(index >= len) {
        buf[0] = 0;
        return;
    }
    unsigned int n = bufsize - 1;
    if(n > len - index)
        n = len - index;
    memcpy((char *) buf, buffer + index, n);
    buf[n] = 0;
}

// /*********************************************/
// /*  Search                                   */
// /*********************************************/

int BString::indexOf(char c) const {
    return indexOf(c, 0);
}

int BString::indexOf(char ch, unsigned int fromIndex) const {
    if(fromIndex >= len)
        return -1;
    const char* temp = (const char *)memchr(buffer + fromIndex, ch, len - fromIndex);
    if(temp == NULL)
        return -1;
    return temp - buffer;
}

int BString::indexOf(const BString &s2) const {
    return indexOf(s2, 0);
}

int BString::indexOf(const BString &s2, unsigned int fromIndex) const {
    if(fromIndex >= len)
        return -1;
    // At least for NOWIFI, memmem() does not exist, but memmem_P() does...
    const char *found = (const char *)memmem_P(buffer + fromIndex, len - fromIndex, s2.buffer, s2.len);
    if(found == NULL)
        return -1;
    return found - buffer;
}

int BString::lastIndexOf(char theChar) const {
    return lastIndexOf(theChar, len - 1);
}

int BString::lastIndexOf(char ch, unsigned int fromIndex) const {
    if(fromIndex >= len)
        return -1;
    char* temp = (char *)memrchr(buffer, ch, fromIndex);
    if(temp == NULL)
        return -1;
    return temp - buffer;
}

int BString::lastIndexOf(const BString &s2) const {
    return lastIndexOf(s2, len - s2.len);
}

int BString::lastIndexOf(const BString &s2, unsigned int fromIndex) const {
    if(s2.len == 0 || len == 0 || s2.len > len)
        return -1;
    if(fromIndex >= len)
        fromIndex = len - 1;
    int found = -1;
    // XXX: make binary-safe
    for(char *p = buffer; p <= buffer + fromIndex; p++) {
        p = strstr(p, s2.buffer);
        if(!p)
            break;
        if((unsigned int) (p - buffer) <= fromIndex)
            found = p - buffer;
    }
    return found;
}

BString BString::substring(unsigned int left, unsigned int right) const {
    if(left > right) {
        unsigned int temp = right;
        right = left;
        left = temp;
    }
    BString out;
    if(left >= len)
        return out;
    if(right > len)
        right = len;
    out.copy(buffer + left, right - left);
    return out;
}

BString BString::substringMB(unsigned int left, unsigned int right) const {
    if(left > right) {
        unsigned int temp = right;
        right = left;
        left = temp;
    }
    BString out;
    if(left >= utf8len(buffer))
        return out;
    if(right > utf8len(buffer))
        right = utf8len(buffer);

    char *start = buffer;
    for (int i = 0; i < left && start && *start; ++i) {
        start += utf8codepointcalcsize(start);
    }
    char *end = start;
    for (int i = 0; i < right - left && end && *end; ++i) {
        end += utf8codepointcalcsize(end);
    }
    out.copy(start, end - start);
    return out;
}

// /*********************************************/
// /*  Modification                             */
// /*********************************************/

void BString::replace(char find, char replace) {
    if(!buffer)
        return;
    unsigned int c = 0;
    for(char *p = buffer; c < len; p++, c++) {
        if(*p == find)
            *p = replace;
    }
}

void BString::replace(const BString& find, const BString& replace) {
    if(len == 0 || find.len == 0)
        return;
    int diff = replace.len - find.len;
    char *readFrom = buffer;
    char *foundAt;
    // XXX: make binary-safe
    if(diff == 0) {
        while((foundAt = strstr(readFrom, find.buffer)) != NULL) {
            memcpy(foundAt, replace.buffer, replace.len);
            readFrom = foundAt + replace.len;
        }
    } else if(diff < 0) {
        char *writeTo = buffer;
        while((foundAt = strstr(readFrom, find.buffer)) != NULL) {
            unsigned int n = foundAt - readFrom;
            memcpy(writeTo, readFrom, n);
            writeTo += n;
            memcpy(writeTo, replace.buffer, replace.len);
            writeTo += replace.len;
            readFrom = foundAt + find.len;
            len += diff;
        }
        strcpy(writeTo, readFrom);
    } else {
        unsigned int size = len; // compute size needed for result
        while((foundAt = strstr(readFrom, find.buffer)) != NULL) {
            readFrom = foundAt + find.len;
            size += diff;
        }
        if(size == len)
            return;
        if(size > capacity && !changeBuffer(size))
            return; // XXX: tell user!
        int index = len - 1;
        while(index >= 0 && (index = lastIndexOf(find, index)) >= 0) {
            readFrom = buffer + index + find.len;
            memmove(readFrom + diff, readFrom, len - (readFrom - buffer));
            len += diff;
            buffer[len] = 0;
            memcpy(buffer + index, replace.buffer, replace.len);
            index--;
        }
    }
}

void BString::remove(unsigned int index) {
    // Pass the biggest integer as the count. The remove method
    // below will take care of truncating it at the end of the
    // string.
    remove(index, (unsigned int) -1);
}

void BString::remove(unsigned int index, unsigned int count) {
    if(index >= len) {
        return;
    }
    if(count <= 0) {
        return;
    }
    if(count > len - index) {
        count = len - index;
    }
    char *writeTo = buffer + index;
    len = len - count;
    memmove(writeTo, buffer + index + count, len - index);
    buffer[len] = 0;
}

void BString::toLowerCase(void) {
    if(!buffer)
        return;
    char *p;
    unsigned int c = 0;
    for(p = buffer; c < len; p++, c++) {
        *p = tolower(*p);
    }
}

void BString::toUpperCase(void) {
    if(!buffer)
        return;
    char *p;
    unsigned int c = 0;
    for(p = buffer; c < len; p++, c++) {
        *p = toupper(*p);
    }
}

void BString::trim(void) {
    if(!buffer || len == 0)
        return;
    char *begin = buffer;
    while(isspace(*begin))
        begin++;
    char *end = buffer + len - 1;
    while(isspace(*end) && end >= begin)
        end--;
    len = end + 1 - begin;
    if(begin > buffer)
        memcpy(buffer, begin, len);
    buffer[len] = 0;
}

// /*********************************************/
// /*  Parsing / Conversion                     */
// /*********************************************/

long BString::toInt(void) const {
    if(buffer)
        return atol(buffer);
    return 0;
}

float BString::toFloat(void) const {
    if(buffer)
        return atof(buffer);
    return 0;
}
