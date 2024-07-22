/*

 Package: dyncall
 Library: test
 File: test/plain_c++/test_main.cc
 Description:
 License:

   Copyright (c) 2007-2022 Daniel Adler <dadler@uni-goettingen.de>,
                           Tassilo Philipp <tphilipp@potion-studios.com>

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/




#include "../../dyncall/dyncall.h"
#include "../common/platformInit.h"
#include "../common/platformInit.c" /* Impl. for functions only used in this translation unit */

#include "../../dyncall/dyncall_aggregate.h"

#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>

jmp_buf jbuf;


void segv_handler(int sig)
{
  longjmp(jbuf, 1);
}


/* -------------------------------------------------------------------------
 * test: identity this calls
 * ------------------------------------------------------------------------- */

union ValueUnion
{
  DCbool     B;
  DCint      i;
  DClong     j;
  DClonglong l;
  DCfloat    f;
  DCdouble   d;
  DCpointer  p;
};


/*
 * the layout of the VTable is non-standard and it is not clear what is the
 * initial real first method index.
 * so far it turns out, *iff* dtor is defined, that:
 * on msvc/x86  : 1
 * on msvc/x64  : 1
 * on gcc/x86   : 2
 * on gcc/x64   : 2
 * on clang/x86 : 2
 * on clang/x64 : 2
 */

// vtable offset to first func of class Value and class ValueMS, skipping dtor
#if defined DC__C_MSVC
#define VTBI_BASE 1
#else
#define VTBI_BASE 2
#endif

#define VTBI_SET_BOOL VTBI_BASE+0
#define VTBI_GET_BOOL VTBI_BASE+1
#define VTBI_SET_INT  VTBI_BASE+2
#define VTBI_GET_INT  VTBI_BASE+3
#define VTBI_SET_LONG VTBI_BASE+4
#define VTBI_GET_LONG VTBI_BASE+5
#define VTBI_SET_LONG_LONG VTBI_BASE+6
#define VTBI_GET_LONG_LONG VTBI_BASE+7
#define VTBI_SET_FLOAT VTBI_BASE+8
#define VTBI_GET_FLOAT VTBI_BASE+9
#define VTBI_SET_DOUBLE VTBI_BASE+10
#define VTBI_GET_DOUBLE VTBI_BASE+11
#define VTBI_SET_POINTER VTBI_BASE+12
#define VTBI_GET_POINTER VTBI_BASE+13
#define VTBI_SUM_3_INTS VTBI_BASE+14

#define TEST_CLASS(NAME, CCONV) \
class NAME \
{ \
public: \
  virtual                  ~NAME()                   { } \
 \
  virtual void       CCONV setBool(DCbool x)         { mValue.B = x; } \
  virtual DCbool     CCONV getBool()                 { return mValue.B; } \
  virtual void       CCONV setInt(DCint x)           { mValue.i = x; } \
  virtual DCint      CCONV getInt()                  { return mValue.i; } \
  virtual void       CCONV setLong(DClong x)         { mValue.j = x; } \
  virtual DClong     CCONV getLong()                 { return mValue.j; } \
  virtual void       CCONV setLongLong(DClonglong x) { mValue.l = x; } \
  virtual DClonglong CCONV getLongLong()             { return mValue.l; } \
  virtual void       CCONV setFloat(DCfloat x)       { mValue.f = x; } \
  virtual DCfloat    CCONV getFloat()                { return mValue.f; } \
  virtual void       CCONV setDouble(DCdouble x)     { mValue.d = x; } \
  virtual DCdouble   CCONV getDouble()               { return mValue.d; } \
  virtual void       CCONV setPtr(DCpointer x)       { mValue.p = x; } \
  virtual DCpointer  CCONV getPtr()                  { return mValue.p; } \
 \
  /* ellipsis test w/ this ptr */ \
  virtual int        CCONV sum3Ints(DCint x, ...)    { va_list va; va_start(va,x); x += va_arg(va,int); x += va_arg(va,int); va_end(va); return x; } \
 \
private: \
  ValueUnion mValue; \
};


#if defined(DC__Arch_Intel_x86) && !defined(DC__C_MSVC) && !defined(__cdecl)
#  define __cdecl
#endif


TEST_CLASS(ValueThisDef, /*empty/default*/) /* default */
#if defined(DC__Arch_Intel_x86)
TEST_CLASS(ValueThisCdecl, __cdecl)         /* methods explicitly declared as cdecl */
#if defined(DC__OS_Win32) && defined(DC__C_MSVC)
TEST_CLASS(ValueThisMS, /*empty/default*/)  /* microsoft this call */
#endif
#endif




template<typename T>
bool testCallValue(DCCallVM* pc, const char* name)
{
  bool r = true, b;
  T o;
  T* pThis = &o;
  DCpointer* vtbl =  *( (DCpointer**) pThis ); /* vtbl is located at beginning of class */

  /* set/get bool (TRUE) */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgBool(pc,DC_TRUE);
  dcCallVoid(pc, vtbl[VTBI_SET_BOOL] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallBool(pc, vtbl[VTBI_GET_BOOL] ) == DC_TRUE );
  printf("bt (%s): %d\n", name, b);
  r = r && b;

  /* set/get bool (FALSE) */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgBool(pc,DC_FALSE);
  dcCallVoid(pc, vtbl[VTBI_SET_BOOL] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallBool(pc, vtbl[VTBI_GET_BOOL] ) == DC_FALSE );
  printf("bf (%s): %d\n", name, b);
  r = r && b;

  /* set/get int */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgInt(pc,1234);
  dcCallVoid(pc, vtbl[VTBI_SET_INT] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallInt(pc, vtbl[VTBI_GET_INT] ) == 1234 );
  printf("i  (%s): %d\n", name, b);
  r = r && b;

  /* set/get long */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgLong(pc,0xCAFEBABEUL);
  dcCallVoid(pc, vtbl[VTBI_SET_LONG] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallLong(pc, vtbl[VTBI_GET_LONG] ) == (DClong)0xCAFEBABEUL );
  printf("l  (%s): %d\n", name, b);
  r = r && b;

  /* set/get long long */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgLongLong(pc,0xCAFEBABEDEADC0DELL);
  dcCallVoid(pc, vtbl[VTBI_SET_LONG_LONG] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallLongLong(pc, vtbl[VTBI_GET_LONG_LONG] ) == (DClonglong)0xCAFEBABEDEADC0DELL );
  printf("ll (%s): %d\n", name, b);
  r = r && b;

  /* set/get float */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgFloat(pc,1.2345f);
  dcCallVoid(pc, vtbl[VTBI_SET_FLOAT] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallFloat(pc, vtbl[VTBI_GET_FLOAT] ) == 1.2345f );
  printf("f  (%s): %d\n", name, b);
  r = r && b;

  /* set/get double */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgDouble(pc,1.23456789);
  dcCallVoid(pc, vtbl[VTBI_SET_DOUBLE] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallDouble(pc, vtbl[VTBI_GET_DOUBLE] ) == 1.23456789 );
  printf("d  (%s): %d\n", name, b);
  r = r && b;

  /* set/get pointer */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcArgPointer(pc, (DCpointer) 0xCAFEBABE );
  dcCallVoid(pc, vtbl[VTBI_SET_POINTER] );
  dcReset(pc);
  dcArgPointer(pc, pThis);
  b = ( dcCallPointer(pc, vtbl[VTBI_GET_POINTER] ) == ( (DCpointer) 0xCAFEBABE ) );
  printf("p  (%s): %d\n", name, b);
  r = r && b;

  /* ellipsis test w/ this pointer */

  dcReset(pc);
  dcArgPointer(pc, pThis);
  dcMode(pc, DC_CALL_C_ELLIPSIS);
  dcArgInt(pc, 23);
  dcMode(pc, DC_CALL_C_ELLIPSIS_VARARGS);
  dcArgInt(pc, -223);
  dcArgInt(pc, 888);
  int r_ = dcCallInt(pc, vtbl[VTBI_SUM_3_INTS]);
  b = (r_ == 688);
  printf("...  (%s): %d\n", name, b);
  r = r && b;

  return r;
}


template<class T>
static bool testCallThis(DCint mode, const char* str)
{
  bool r = false;
  DCCallVM* pc = dcNewCallVM(4096);
  dcMode(pc, mode);
  dcReset(pc);
  if(setjmp(jbuf) != 0)
    printf("sigsegv\n"), r=false;
  else
    r = testCallValue<T>(pc, str);
  dcFree(pc);
  return r;
}


#if defined(DC__Feature_AggrByVal)

#define TEST_CLASS_AGGR(NAME, CCONV) \
class NAME \
{ \
public: \
  struct S { int i, j, k, l, m; }; \
 \
  virtual             ~NAME()       { } \
 \
  virtual void  CCONV setAggr(S x)  { mS.i = x.i; mS.j = x.j; mS.k = x.k; mS.l = x.l; mS.m = x.m; } \
  virtual S     CCONV getAggr()     { return mS; } \
 \
  /* ellipsis test w/ this ptr and big (!) aggregate return */ \
  struct Big { int sum; long long dummy[50]; /*dummy to make it not fit in any regs*/ }; \
  virtual struct Big  CCONV sum3RetAggr(DCint x, ...) { va_list va; va_start(va,x); struct Big r = { x + va_arg(va,int) + va_arg(va,int) }; va_end(va); return r; } \
 \
  /* non-trivial aggregate */ \
  struct NonTriv { \
    int i, j; \
    NonTriv(int a, int b) : i(a),j(b) { } \
    NonTriv(const NonTriv& rhs) { static int a=13, b=37; i = a++; j = b++; } \
  }; \
  /* by value, so on first invocation a = 13,37, b = 14,38 and retval = 13*14,37*38, no matter the contents of the instances as copy ctor is called */ \
  /* NOTE: copy of return value is subject to C++ "copy elision", so it is *not* calling the copy ctor for the return value */ \
  virtual struct NonTriv  CCONV squareFields(NonTriv a, NonTriv b) { return NonTriv(a.i*b.i, a.j*b.j); } \
 \
private: \
  struct S mS; \
};

TEST_CLASS_AGGR(ValueAggrThisDef, /*empty/default*/) /* default */
#if defined(DC__Arch_Intel_x86)
TEST_CLASS_AGGR(ValueAggrThisCdecl, __cdecl)         /* methods explicitly declared as cdecl */
#if defined(DC__OS_Win32) && defined(DC__C_MSVC)
TEST_CLASS_AGGR(ValueAggrThisMS, /*empty/default*/)  /* microsoft this call */
#endif
#endif


#if (__cplusplus >= 201103L)
#  include <type_traits>
#endif

/* special case w/ e.g. MS x64 C++ calling cconf: struct return ptr is passed as *2nd* arg */
template<class T>
static bool testCallThisAggr(DCint mode, const char* str)
{
  bool r = false;
  DCCallVM* pc = dcNewCallVM(4096);
  dcMode(pc, mode);

  if(setjmp(jbuf) != 0)
    printf("sigsegv\n"), r=false;
  else
  {
    T o;

    DCpointer* vtbl =  *( (DCpointer**) &o ); /* vtbl is located at beginning of class */
    typename T::S st = { 124, -12, 434, 20202, -99999 }, returned;

#if (__cplusplus >= 201103L)
    bool istriv = std::is_trivial<typename T::S>::value;
#else
    bool istriv = true; /* own deduction as no type trait */
#endif
    DCaggr *s = dcNewAggr(5, sizeof(typename T::S));
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(typename T::S, i), 1);
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(typename T::S, j), 1);
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(typename T::S, k), 1);
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(typename T::S, l), 1);
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(typename T::S, m), 1);
    dcCloseAggr(s);

    // set S::mS
    dcReset(pc);
    dcArgPointer(pc, &o); // this ptr
    dcArgAggr(pc, s, &st);
    dcCallVoid(pc, vtbl[VTBI_BASE+0]);

    // get it back
    dcReset(pc);
    dcBeginCallAggr(pc, s);
    dcArgPointer(pc, &o); // this ptr
    dcCallAggr(pc, vtbl[VTBI_BASE+1], s, &returned);

    dcFreeAggr(s);

    r = returned.i == st.i && returned.j == st.j && returned.k == st.k && returned.l == st.l && returned.m == st.m && istriv;
    printf("r:{iiiii}  (%s/trivial): %d\n", str, r);



    /* ellipsis test w/ this pointer returning big aggregate (quite an edge
     * case) by value (won't fit in regs, so hidden pointer is is used to write
     * return values to), showing the need to use the DC_CALL_C_DEFAULT_THIS
     * mode first, for the this ptr alone, then DC_CALL_C_ELLIPSIS, then
     * DC_CALL_C_ELLIPSIS_VARARGS (test is useful on win64 where thisptr is
     * passed *after* return aggregate's hidden ptr) */
#if (__cplusplus >= 201103L)
    istriv = std::is_trivial<typename T::Big>::value;
#else
    istriv = true; /* own deduction as no type trait */
#endif
    s = dcNewAggr(2, sizeof(struct T::Big));
    dcAggrField(s, DC_SIGCHAR_INT, offsetof(struct T::Big, sum), 1);
    dcAggrField(s, DC_SIGCHAR_LONGLONG, offsetof(struct T::Big, dummy), 50);
    dcCloseAggr(s);
    dcReset(pc);
    dcMode(pc, mode);

    dcBeginCallAggr(pc, s);
    dcArgPointer(pc, &o);
    dcMode(pc, DC_CALL_C_ELLIPSIS);
    dcArgInt(pc, 89);
    dcMode(pc, DC_CALL_C_ELLIPSIS_VARARGS);
    dcArgInt(pc, -157);
    dcArgInt(pc, 888);
    struct T::Big big;
    dcCallAggr(pc, vtbl[VTBI_BASE+2], s, &big);

    dcFreeAggr(s);

    bool b = (big.sum == 820) && istriv;
    r = r && b;
    printf("r:{il[50]}  (%s/trivial/ellipsis): %d\n", str, b);



    /* non-trivial test ----------------------------------------------------------- */

#if (__cplusplus >= 201103L)
    istriv = std::is_trivial<typename T::NonTriv>::value;
#else
    istriv = false; /* own deduction as no type trait */
#endif
    dcReset(pc);
    dcMode(pc, mode);

    /* non trivial aggregates: pass NULL for DCaggr* and do copy on our own (see doc) */
    dcBeginCallAggr(pc, NULL);

    typename T::NonTriv nt0(5, 6), nt1(7, 8), ntr(0, 0);
    dcArgAggr(pc, NULL, &o); // this ptr
    /* make *own* copies, as dyncall cannot know how to call copy ctor */ //@@@ put into doc
    typename T::NonTriv nt0_ = nt0, nt1_ = nt1;
    dcArgAggr(pc, NULL, &nt0_); /* use *own* copy */
    dcArgAggr(pc, NULL, &nt1_); /* use *own* copy */

    dcCallAggr(pc, vtbl[VTBI_BASE+3], NULL, &ntr); /* note: "copy elision", so retval might *not* call copy ctor */


    b = ntr.i == 13*14 && ntr.j == 37*38 && !istriv;
    r = r && b;
    printf("r:{ii}  (%s/nontrivial/retval_copy_elision): %d\n", str, b);
  }

  dcFree(pc);
  return r;
}

#endif


extern "C" {

int main(int argc, char* argv[])
{
  dcTest_initPlatform();

  signal(SIGSEGV, segv_handler);

  bool r = true;

  r = testCallThis<ValueThisDef>(DC_CALL_C_DEFAULT_THIS, "thisDef") && r;
#if defined(DC__Arch_Intel_x86)
  r = testCallThis<ValueThisCdecl>(DC_CALL_C_X86_CDECL, "thisCdecl") && r;
#if defined(DC__OS_Win32) && defined(DC__C_MSVC)
  r = testCallThis<ValueThisMS>(DC_CALL_C_X86_WIN32_THIS_MS, "thisMS") && r;
#endif
#endif

#if defined(DC__Feature_AggrByVal)
  r = testCallThisAggr<ValueAggrThisDef>(DC_CALL_C_DEFAULT_THIS, "thisDef") && r;
#if defined(DC__Arch_Intel_x86)
  r = testCallThisAggr<ValueAggrThisCdecl>(DC_CALL_C_X86_CDECL, "thisCdecl") && r;
#if defined(DC__OS_Win32) && defined(DC__C_MSVC)
  r = testCallThisAggr<ValueAggrThisMS>(DC_CALL_C_X86_WIN32_THIS_MS, "thisMS") && r;
#endif
#endif
#endif

  printf("result: plain_c++: %d\n", r);

  dcTest_deInitPlatform();

  return !r;
}

}  // extern "C"

