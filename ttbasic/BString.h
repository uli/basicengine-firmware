// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 BString.h - String library for Wiring & Arduino
 ...mostly rewritten by Paul Stoffregen...
 Copyright (c) 2009-10 Hernando Barragan.  All right reserved.
 Copyright 2011, Paul Stoffregen, paul@pjrc.com
 Copyright 2018, 2019 Ulrich Hecht

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

#ifndef BString_class_h
#define BString_class_h
#ifdef __cplusplus

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pgmspace.h>
#include "ttconfig.h"


// An inherited class for holding the result of a concatenation.  These
// result objects are assumed to be writable by subsequent concatenations.
class BStringSumHelper;

// an abstract class used as a means to proide a unique pointer type
// but really has no body
// XXX: SdFat/src/SysCall.h messes with this! WTF??
#undef F
class __FlashStringHelper;
#if !defined(ARDUINO) || defined(HOSTED)
#define FPSTR(p) (p)
#define F(p) (p)
#else
#ifndef FPSTR
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif
#define F(string_literal) (FPSTR(PSTR(string_literal)))
#endif

// The string class
class BString {
        // use a function pointer to allow for "if (s)" without the
        // complications of an operator bool(). for more information, see:
        // http://www.artima.com/cppsource/safebool.html
        typedef void (BString::*BStringIfHelperType)() const;
        void BStringIfHelper() const {
        }

    public:
        // constructors
        // creates a copy of the initial value.
        // if the initial value is null or invalid, or if memory allocation
        // fails, the string will be marked as invalid (i.e. "if (s)" will
        // be false).
        BString(const char *cstr = "");
        BString(const BString &str);
        BString(const __FlashStringHelper *str);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        BString(BString &&rval);
        BString(BStringSumHelper &&rval);
#endif
        explicit BString(char c);
        explicit BString(unsigned char, unsigned char base = 10);
        explicit BString(int, unsigned char base = 10);
        explicit BString(unsigned int, unsigned char base = 10);
        explicit BString(long, unsigned char base = 10);
        explicit BString(unsigned long, unsigned char base = 10);
        explicit BString(float, unsigned char decimalPlaces = 2);
        explicit BString(double, unsigned char decimalPlaces = 2);
        ~BString(void);

        int fromBasic(TOKEN_TYPE *s) {
          len = *s++;
          if (!reserve(len)) {
            invalidate();
            return 0;
          }
          for (unsigned int i = 0; i < len; i++)
              buffer[i] = s[i];
          buffer[len] = 0;
          return len + 1;
        }

        // memory management
        // return true on success, false on failure (in which case, the string
        // is left unchanged).  reserve(0), if successful, will validate an
        // invalid string (i.e., "if (s)" will be true afterwards)
        unsigned char reserve(unsigned int size);
        inline unsigned int length(void) const {
            if(buffer) {
                return len;
            } else {
                return 0;
            }
        }
        inline void resetLength(unsigned int size) {
            len = size;
        }

        // creates a copy of the assigned value.  if the value is null or
        // invalid, or if the memory allocation fails, the string will be
        // marked as invalid ("if (s)" will be false).
        BString & operator =(const BString &rhs);
        BString & operator =(const char *cstr);
        BString & operator = (const __FlashStringHelper *str);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        BString & operator =(BString &&rval);
        BString & operator =(BStringSumHelper &&rval);
#endif

        // concatenate (works w/ built-in types)

        // returns true on success, false on failure (in which case, the string
        // is left unchanged).  if the argument is null or invalid, the
        // concatenation is considered unsucessful.
        unsigned char concat(const BString &str);
        unsigned char concat(const char *cstr);
        unsigned char concat(char c);
        unsigned char concat(unsigned char c);
        unsigned char concat(int num);
        unsigned char concat(unsigned int num);
        unsigned char concat(long num);
        unsigned char concat(unsigned long num);
        unsigned char concat(float num);
        unsigned char concat(double num);
        unsigned char concat(const __FlashStringHelper * str);

        // if there's not enough memory for the concatenated value, the string
        // will be left unchanged (but this isn't signalled in any way)
        BString & operator +=(const BString &rhs) {
            concat(rhs);
            return (*this);
        }
        BString & operator +=(const char *cstr) {
            concat(cstr);
            return (*this);
        }
        BString & operator +=(char c) {
            concat(c);
            return (*this);
        }
        BString & operator +=(unsigned char num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(int num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(unsigned int num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(long num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(unsigned long num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(float num) {
            concat(num);
            return (*this);
        }
        BString & operator +=(double num) {
            concat(num);
            return (*this);
        }
        BString & operator += (const __FlashStringHelper *str){
            concat(str);
            return (*this);
        }

        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, const BString &rhs);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, const char *cstr);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, char c);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned char num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, int num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned int num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, long num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, unsigned long num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, float num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, double num);
        friend BStringSumHelper & operator +(const BStringSumHelper &lhs, const __FlashStringHelper *rhs);

        // comparison (only works w/ BStrings and "strings")
        operator BStringIfHelperType() const {
            return buffer ? &BString::BStringIfHelper : 0;
        }
        int compareTo(const BString &s) const;
        unsigned char equals(const BString &s) const;
        unsigned char equals(const char *cstr) const;
        unsigned char operator ==(const BString &rhs) const {
            return equals(rhs);
        }
        unsigned char operator ==(const char *cstr) const {
            return equals(cstr);
        }
        unsigned char operator !=(const BString &rhs) const {
            return !equals(rhs);
        }
        unsigned char operator !=(const char *cstr) const {
            return !equals(cstr);
        }
        unsigned char operator <(const BString &rhs) const;
        unsigned char operator >(const BString &rhs) const;
        unsigned char operator <=(const BString &rhs) const;
        unsigned char operator >=(const BString &rhs) const;
        unsigned char equalsIgnoreCase(const BString &s) const;
        unsigned char startsWith(const BString &prefix) const;
        unsigned char startsWith(const BString &prefix, unsigned int offset) const;
        unsigned char endsWith(const BString &suffix) const;

        // character acccess
        char charAt(unsigned int index) const;
        void setCharAt(unsigned int index, char c);
        char operator [](unsigned int index) const;
        char& operator [](unsigned int index);
        void getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index = 0) const;
        void toCharArray(char *buf, unsigned int bufsize, unsigned int index = 0) const {
            getBytes((unsigned char *) buf, bufsize, index);
        }
        const char* c_str() const { return buffer; }
        char* begin() { return buffer; }
        char* end() { return buffer + length(); }
        const char* begin() const { return c_str(); }
        const char* end() const { return c_str() + length(); }

        // search
        int indexOf(char ch) const;
        int indexOf(char ch, unsigned int fromIndex) const;
        int indexOf(const BString &str) const;
        int indexOf(const BString &str, unsigned int fromIndex) const;
        int lastIndexOf(char ch) const;
        int lastIndexOf(char ch, unsigned int fromIndex) const;
        int lastIndexOf(const BString &str) const;
        int lastIndexOf(const BString &str, unsigned int fromIndex) const;
        BString substring(unsigned int beginIndex) const {
            return substring(beginIndex, len);
        }
        ;
        BString substring(unsigned int beginIndex, unsigned int endIndex) const;

        // modification
        void replace(char find, char replace);
        void replace(const BString& find, const BString& replace);
        void remove(unsigned int index);
        void remove(unsigned int index, unsigned int count);
        void toLowerCase(void);
        void toUpperCase(void);
        void trim(void);

        // parsing/conversion
        long toInt(void) const;
        float toFloat(void) const;

    protected:
        char *buffer;	        // the actual char array
        unsigned int capacity;  // the array length minus one (for the '\0')
        unsigned int len;       // the BString length (not counting the '\0')
    protected:
        void init(void);
        void invalidate(void);
        unsigned char changeBuffer(unsigned int maxStrLen);
        unsigned char concat(const char *cstr, unsigned int length);

        // copy and move
        BString & copy(const char *cstr, unsigned int length);
        BString & copy(const __FlashStringHelper *pstr, unsigned int length);
#ifdef __GXX_EXPERIMENTAL_CXX0X__
        void move(BString &rhs);
#endif
};

class BStringSumHelper: public BString {
    public:
        BStringSumHelper(const BString &s) :
                BString(s) {
        }
        BStringSumHelper(const char *p) :
                BString(p) {
        }
        BStringSumHelper(char c) :
                BString(c) {
        }
        BStringSumHelper(unsigned char num) :
                BString(num) {
        }
        BStringSumHelper(int num) :
                BString(num) {
        }
        BStringSumHelper(unsigned int num) :
                BString(num) {
        }
        BStringSumHelper(long num) :
                BString(num) {
        }
        BStringSumHelper(unsigned long num) :
                BString(num) {
        }
        BStringSumHelper(float num) :
                BString(num) {
        }
        BStringSumHelper(double num) :
                BString(num) {
        }
};

#endif  // __cplusplus
#endif  // BString_class_h
