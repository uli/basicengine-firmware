// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"

BString BASIC_INT Basic::ilrstr(bool right, bool bytewise) {
  BString value;
  num_t nlen;
  int32_t len;

  if (checkOpen())
    goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(nlen, I_CLOSE))
    goto out;

  if (ceil(nlen) < 0) {
    E_ERR(VALUE, _("negative substring length"));
    goto out;
  }

  len = nlen;

  if (bytewise) {
    if (right) {
      value = value.substring(_max(0, (int)value.length() - len), value.length());
    } else
      value = value.substring(0, len);
  } else {
    if (right) {
      value = value.substringMB(_max(0, (int)value.lengthMB() - len), value.lengthMB());
    } else
      value = value.substringMB(0, len);
  }

out:
  return value;
}

/***bf bas LEFT$
Returns a specified number of leftmost characters in a string.
\usage s$ = LEFT$(l$, num)
\args
@l	any string expression
@num	number of characters to return [min `0`]
\ret Substring of `num` characters or less.
\note
If `l$` is shorter than `num` characters, the return value is `l$`.
\ref MID$() RIGHT$()
***/
BString BASIC_INT Basic::sleft() {
  return ilrstr(false);
}
/***bf bas RIGHT$
Returns a specified number of rightmost characters in a string.
\usage s$ = RIGHT$(r$, num)
\args
@r	any string expression
@num	number of characters to return [min `0`]
\ret Substring of `num` characters or less.
\note
If `r$` is shorter than `num` characters, the return value is `r$`.
\ref LEFT$() MID$()
***/
BString BASIC_INT Basic::sright() {
  return ilrstr(true);
}

/***bf bas BLEFT$
Returns a specified number of leftmost bytes in a byte string.
\usage s$ = BLEFT$(l$, num)
\args
@l	any string expression
@num	number of bytes to return [min `0`]
\ret Substring of `num` bytes or less.
\note
If `l$` is shorter than `num` bytes, the return value is `l$`.
\ref BMID$() BRIGHT$()
***/
BString BASIC_INT Basic::sbleft() {
  return ilrstr(false, true);
}
/***bf bas BRIGHT$
Returns a specified number of rightmost bytes in a byte string.
\usage s$ = RIGHT$(r$, num)
\args
@r	any string expression
@num	number of bytes to return [min `0`]
\ret Substring of `num` bytes or less.
\note
If `r$` is shorter than `num` bytes, the return value is `r$`.
\ref BLEFT$() BMID$()
***/
BString BASIC_INT Basic::sbright() {
  return ilrstr(true, true);
}

/***bf bas MID$
Returns part of a string (a substring).
\usage s$ = MID$(m$, start[, len])
\args
@m$	any string expression
@start	position of the first character in the substring being returned
@len	number of characters in the substring [default: `LEN(m$)-start`]
\ret Substring of `len` characters or less.
\note
* If `m$` is shorter than `len` characters, the return value is `m$`.
* Unlike with other BASIC implementations, `start` is zero-based, i.e. the
  first character is 0, not 1.
\bugs
You cannot assign values to `MID$()`, which is possible in some other BASIC
implementations.
\ref LEFT$() LEN() RIGHT$()
***/
BString BASIC_INT Basic::smid() {
  BString value;
  num_t nstart;
  int32_t start;
  num_t nlen;
  int32_t len;

  if (checkOpen())
    goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(nstart, I_NONE))
    goto out;
  if (ceil(nstart) < 0) {
    E_ERR(VALUE, _("negative string offset"));
    goto out;
  }
  start = nstart;

  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(nlen, I_NONE))
      goto out;
    if (nlen > INT_MAX)
      len = INT_MAX;
    else
      len = nlen;
  } else {
    len = value.lengthMB() - start;
  }
  if (checkClose())
    goto out;

  value = value.substringMB(start, start + len);

out:
  return value;
}

/***bf bas BMID$
Returns part of a byte string.
\usage s$ = BMID$(m$, start[, len])
\args
@m$	any string expression
@start	position of the first byte in the substring being returned
@len	number of bytes in the substring [default: `BLEN(m$)-start`]
\ret Substring of `len` bytes or less.
\note
* If `m$` is shorter than `len` bytes, the return value is `m$`.
* Unlike with other BASIC implementations, `start` is zero-based, i.e. the
  first byte is 0, not 1.
\ref BLEFT$() BLEN() BRIGHT$()
***/
BString BASIC_INT Basic::sbmid() {
  BString value;
  int32_t start;
  int32_t len;

  if (checkOpen())
    goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(start, I_NONE))
    goto out;
  if (start < 0) {
    E_ERR(VALUE, _("negative string offset"));
    goto out;
  }
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(len, I_NONE))
      goto out;
  } else {
    len = value.length() - start;
  }
  if (checkClose())
    goto out;

  value = value.substring(start, start + len);

out:
  return value;
}

/***bf bas CHR$
Returns a string containing the UTF-8 encoding of the specified Unicode
codepoint.
\usage char = CHR$(val)
\args
@val	Unicode codepoint
\ret Single-character string.
\note
* To construct string from a byte value without UTF-8 encoding, use the
  `a$ = [byte]` syntax.
\ref ASC()
***/
BString Basic::schr() {
  int32_t nv;
  BString value;
  if (checkOpen())
    return value;
  if (getParam(nv, 0, 0x10ffff, I_NONE))
    return value;
  checkClose();
  if (err)
    return value;

  if (nv == 0)
    return BString((char)0);
  else {
    char nvstr[5];

    *(char *)utf8catcodepoint(nvstr, nv, 4) = 0;
    value = BString(nvstr);

    return value;
  }
}

/***bf bas STR$
Returns a string representation of a number.
\usage s$ = STR$(num)
\args
@num	any numeric expression
\ret String representation of `num`.
\ref VAL()
***/
BString Basic::sstr() {
  BString value;
  if (checkOpen())
    return value;
  // The BString ctor for doubles is not helpful because it uses dtostrf()
  // which can only do a fixed number of decimal places. That is not
  // the BASIC Way(tm).
  sprintf(lbuf, "%0g", iexp());
  value = lbuf;
  checkClose();
  return value;
}

/***bf bas RET$
Returns one of the string return values returned by the last function
call.
\usage rval$ = RET$(num)
\args
@num	number of the string return value [`0` to `{MAX_RETVALS_m1}`]
\ret String return value requested.
\ref RET() RETURN
***/
BString BASIC_INT Basic::sret() {
  int32_t n = getparam();
  if (n < 0 || n >= MAX_RETVALS) {
    E_VALUE(0, MAX_RETVALS - 1);
    return BString(F(""));
  }
  return retstr[n];
}

/***bf bas STRING$
Returns a string of a specified length made up of a repeating character.
\usage s$ = STRING$(count, char$)
\args
@count	number of characters [at least `0`]
@char$	any non-empty string expression
\note
* If `char$` contains more than one character, only the first will be considered.
* If `count` is `0`, an empty string will be returned.
\example
====
----
PRINT STRING$(5, "-");
PRINT "Hello";
PRINT STRING$(5, "-")
----
====
***/
BString Basic::sstring() {
  BString out;
  int32_t count;
  int32_t c;
  if (checkOpen())
    return out;
  if (getParam(count, I_COMMA))
    return out;
  if (count < 0) {
    E_ERR(VALUE, _("negative length"));
    return out;
  }
  if (is_strexp()) {
    BString cs = istrexp();
    if (err)
      return cs;
    if (cs.length() < 1) {
      E_ERR(VALUE, _("need min 1 character"));
      return cs;
    }
    c = cs[0];
    if (checkClose())
      return cs;
  } else {
    if (getParam(c, 0, 255, I_CLOSE))
      return out;
  }
  if (!out.reserve(count)) {
    err = ERR_OOM;
    return out;
  }
  memset(out.begin(), c, count);
  out.begin()[count] = 0;
  out.resetLength(count);

  return out;
}

/***bf bas INSTR
Get the position of the first occurrence of a string in another string.
\usage p = INSTR(haystack$, needle$)
\args
@haystack$	string in which to search
@needle$	string to search for
\ret
Position of first occurence of `needle$` (starting with `0`), or `-1` if it
could not be found.

NOTE: The meaning of the return value differs from other BASIC implementations,
in which `0` usually indicates that the string has not been found, and locations
found start at `1`.
***/
num_t BASIC_INT Basic::ninstr() {
  BString haystack, needle;
  if (checkOpen())
    return 0;
  haystack = istrexp();
  if (err)
    return 0;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return 0;
  }
  needle = istrexp();
  if (checkClose())
    return 0;
  const char *res = strstr(haystack.c_str(), needle.c_str());
  if (!res)
    return -1;
  else
    return res - haystack.c_str();
}

// XXX: 40 byte jump table
num_t BASIC_INT NOJUMP Basic::irel_string() {
  BString lhs = istrexp();
  BString rhs;
  switch (*cip++) {
/***bo op = (strings)
String equality operator.
\usage a$ = b$
\res
`-1` if the value of `a$` is identical to the value of `b$`, `0` otherwise.
\prec 5
***/
  case I_EQ:
    rhs = istrexp();
    return basic_bool(lhs == rhs);
/***bo op <> (strings)
String inequality operator.
\usage
a$ <> b$

a$ >< b$
\res
`-1` if the value of `a$` is different from the value of `b$`, `0` otherwise.
\prec 5
***/
  case I_NEQ:
  case I_NEQ2:
    rhs = istrexp();
    return basic_bool(lhs != rhs);
/***bo op < (strings)
String less-than operator.
\usage a$ < b$
\res
`-1` if the value of `a$` precedes the value of `b$` when sorted
alphabetically, `0` otherwise.
\prec 5
***/
  case I_LT:
    rhs = istrexp();
    return basic_bool(lhs < rhs);
/***bo op > (strings)
String greater-than operator.
\usage a$ > b$
\res
`-1` if the value of `a$` succeeds the value of `b$` when sorted
alphabetically, `0` otherwise.
\prec 5
***/
  case I_GT:
    rhs = istrexp();
    return basic_bool(lhs > rhs);
  case I_SQOPEN: {
    int32_t i = iexp();
    if (*cip++ != I_SQCLOSE) {
      E_SYNTAX(I_SQCLOSE);
      return -1;
    }
    return lhs[i];
  }
  default:
    err = ERR_TYPE;
    return -1;
  }
}

/***bf bas LCASE$
Converts string to all-lowercase letters.
\usage s$ = LCASE$(s$)
\args
@s	string to convert
\ret
Value of `s$` with all letters replaced with their lowercase equivalents.
\ref UCASE$
***/
BString Basic::slcase() {
  BString s;
  if (checkOpen())
    return s;
  s = istrexp();
  if (err || checkClose())
    return s;
  utf8lwr(s.begin());
  return s;
}

/***bf bas UCASE$
Converts string to all-uppercase letters.
\usage s$ = UCASE$(s$)
\args
@s	string to convert
\ret
Value of `s$` with all letters replaced with their uppercase equivalents.
\ref LCASE$
***/
BString Basic::sucase() {
  BString s;
  if (checkOpen())
    return s;
  s = istrexp();
  if (err || checkClose())
    return s;
  utf8upr(s.begin());
  return s;
}
