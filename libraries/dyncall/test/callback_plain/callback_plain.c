/*

 Package: dyncall
 Library: test
 File: test/callback_plain/callback_plain.c
 Description: 
 License:

   Copyright (c) 2011-2022 Daniel Adler <dadler@uni-goettingen.de>,
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

#include "../../dyncallback/dyncall_callback.h"
#include "../common/platformInit.h"
#include "../common/platformInit.c" /* Impl. for functions only used in this translation unit */


char cbSimpleHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  int       arg1;
  float     arg2;
  short     arg3;
  double    arg4;
  long long arg5;

  printf("reached callback\n");
  printf("userdata (should be 1337): %d\n", *ud);

  arg1 = dcbArgInt     (args);  printf("1st argument (should be  123): %d\n",   arg1);
  arg2 = dcbArgFloat   (args);  printf("2nd argument (should be 23.f): %f\n",   arg2);
  arg3 = dcbArgShort   (args);  printf("3rd argument (should be    3): %d\n",   arg3);
  arg4 = dcbArgDouble  (args);  printf("4th argument (should be 1.82): %f\n",   arg4);
  arg5 = dcbArgLongLong(args);  printf("5th argument (should be 9909): %lld\n", arg5);

  if(*ud == 1337) *ud = 1;
  if(arg1 ==  123) ++*ud;
  if(arg2 == 23.f) ++*ud;
  if(arg3 ==    3) ++*ud;
  if(arg4 == 1.82) ++*ud;
  if(arg5 == 9909) ++*ud;

  result->s = 1234;
  return 's';
}

int testSimpleCallback()
{
  DCCallback* cb;
  short result = 0;
  int userdata = 1337;

  cb = dcbNewCallback("ifsdl)s", &cbSimpleHandler, &userdata);

  result = ((short(*)(int, float, short, double, long long))cb)(123, 23.f, 3, 1.82, 9909ull);
  dcbFreeCallback(cb);

  printf("successfully returned from callback\n");
  printf("return value (should be 1234): %d\n", result);

  return (userdata == 6) && (result == 1234);
}


#if defined(DC__Feature_AggrByVal)
typedef struct {
  float a;
  float b;
} Float_Float;

typedef struct {
  unsigned char a;
  double b;
} U8_Double;

typedef struct {
  unsigned long long a;
  unsigned long long b;
} U64_U64;

typedef struct {
  double a;
  double b;
} Double_Double;

typedef struct {
  unsigned long long a;
  unsigned long long b;
  unsigned long long c;
} Three_U64;

typedef struct {
  double a;
  double b;
  double c;
} Three_Double;


char cbAggrArgHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  int           arg1;
  Float_Float   arg2;
  U8_Double     arg3;
  Three_Double  arg4;
  double        arg5;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgInt(args);                printf("1st argument: %d\n", arg1);
  dcbArgAggr(args, (DCpointer)&arg2);    printf("2nd argument: %f %f\n", arg2.a, arg2.b);
  dcbArgAggr(args, (DCpointer)&arg3);    printf("3nd argument: %d %f\n", arg3.a, arg3.b);
  dcbArgAggr(args, (DCpointer)&arg4);    printf("4rd argument: %f %f %f\n", arg4.a, arg4.b, arg4.c);
  arg5 = dcbArgDouble(args);             printf("5th argument: %f\n", arg5);

  result->d = *ud + arg1 + arg2.a + arg2.b + arg3.a + arg3.b + arg4.a + arg4.b + arg4.c + arg5;
  return 'd';
}


int testAggrArgsCallback()
{
  DCCallback* cb;
  DCaggr *float_float_aggr, *u8_double_aggr, *three_double_aggr, *aggrs[3];
  Float_Float ff;
  U8_Double u8d;
  Three_Double threed;

  int ret = 1;
  double result = 0;
  int userdata = 1337;

  ff.a = 1.5;
  ff.b = 5.5;
  float_float_aggr = dcNewAggr(2, sizeof(ff));
  dcAggrField(float_float_aggr, DC_SIGCHAR_FLOAT, offsetof(Float_Float, a), 1);
  dcAggrField(float_float_aggr, DC_SIGCHAR_FLOAT, offsetof(Float_Float, b), 1);
  dcCloseAggr(float_float_aggr);

  u8d.a = 5;
  u8d.b = 5.5;
  u8_double_aggr = dcNewAggr(2, sizeof(u8d));
  dcAggrField(u8_double_aggr, DC_SIGCHAR_UCHAR,  offsetof(U8_Double, a), 1);
  dcAggrField(u8_double_aggr, DC_SIGCHAR_DOUBLE, offsetof(U8_Double, b), 1);
  dcCloseAggr(u8_double_aggr);

  threed.a = 1.5;
  threed.b = 2.5;
  threed.c = 3.5;
  three_double_aggr = dcNewAggr(3, sizeof(threed));
  dcAggrField(three_double_aggr, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, a), 1);
  dcAggrField(three_double_aggr, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, b), 1);
  dcAggrField(three_double_aggr, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, c), 1);
  dcCloseAggr(three_double_aggr);

  aggrs[0] = float_float_aggr;
  aggrs[1] = u8_double_aggr;
  aggrs[2] = three_double_aggr;

  cb = dcbNewCallback2("iAAAd)d", &cbAggrArgHandler, &userdata, aggrs);

  result = ((double(*)(int, Float_Float, U8_Double, Three_Double, double))cb)(123, ff, u8d, threed, 4.5);
  dcbFreeCallback(cb);
  dcFreeAggr(float_float_aggr);
  dcFreeAggr(u8_double_aggr);
  dcFreeAggr(three_double_aggr);

  printf("successfully returned from callback\n");
  printf("return value (should be 1489.5): %f\n", result);

  ret = result == 1489.5 && ret;

  return ret;
}

char cbFloatFloatReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  float arg1, arg2;
  Float_Float ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgFloat(args);  printf("1st argument: %f\n", arg1);
  arg2 = dcbArgFloat(args);  printf("2th argument: %f\n", arg2);

  ret.a = *ud + arg1;
  ret.b = arg2;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

char cbU8DoubleReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  int arg1;
  double arg2;
  U8_Double ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgInt   (args);  printf("1st argument: %d\n", arg1);
  arg2 = dcbArgDouble(args);  printf("2th argument: %f\n", arg2);

  ret.a = *ud + arg1;
  ret.b = arg2;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

char cbU64U64ReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  unsigned long long arg1, arg2;
  U64_U64 ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgULongLong(args);  printf("1st argument: %lld\n", arg1);
  arg2 = dcbArgULongLong(args);  printf("2th argument: %lld\n", arg2);

  ret.a = *ud + arg1;
  ret.b = arg2;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

char cbDoubleDoubleReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  double arg1, arg2;
  Double_Double ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgDouble(args);  printf("1st argument: %f\n", arg1);
  arg2 = dcbArgDouble(args);  printf("2th argument: %f\n", arg2);

  ret.a = *ud + arg1;
  ret.b = arg2;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

char cbThreeU64ReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  unsigned long long arg1, arg2, arg3;
  Three_U64 ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgULongLong(args);  printf("1st argument: %lld\n", arg1);
  arg2 = dcbArgULongLong(args);  printf("2th argument: %lld\n", arg2);
  arg3 = dcbArgULongLong(args);  printf("3th argument: %lld\n", arg3);

  ret.a = *ud + arg1;
  ret.b = arg2;
  ret.c = arg3;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

char cbThreeDoubleReturnHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata)
{
  int* ud = (int*)userdata;
  double arg1, arg2, arg3;
  Three_Double ret;

  printf("reached callback\n");
  printf("userdata: %d\n", *ud);

  arg1 = dcbArgDouble(args);   printf("1st argument: %f\n", arg1);
  arg2 = dcbArgDouble(args);   printf("2th argument: %f\n", arg2);
  arg3 = dcbArgDouble(args);   printf("3th argument: %f\n", arg3);

  ret.a = *ud + arg1;
  ret.b = arg2;
  ret.c = arg3;

  dcbReturnAggr(args, result, (DCpointer)&ret);

  return 'A';
}

int testAggrReturnCallback()
{
  int ret = 1;

  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    Float_Float expected, result;

    expected.a = 11.5;
    expected.b = 2.5;

    s = dcNewAggr(2, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_FLOAT, offsetof(Float_Float, a), 1);
    dcAggrField(s, DC_SIGCHAR_FLOAT, offsetof(Float_Float, b), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("ff)A", &cbFloatFloatReturnHandler, &userdata, aggrs);

    result = ((Float_Float(*)(float, float))cb)(1.5, 2.5);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %f %f): %f %f\n", expected.a, expected.b, result.a, result.b);

    ret = result.a == expected.a && result.b == expected.b && ret;
  }
  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    U8_Double expected, result;

    expected.a = 15;
    expected.b = 5.5;

    s = dcNewAggr(2, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_UCHAR,  offsetof(U8_Double, a), 1);
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(U8_Double, b), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("id)A", &cbU8DoubleReturnHandler, &userdata, aggrs);

    result = ((U8_Double(*)(int, double))cb)(5, 5.5);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %d %f): %d %f\n", (int)expected.a, expected.b, (int)result.a, result.b);

    ret = result.a == expected.a && result.b == expected.b && ret;
  }
  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    U64_U64 expected, result;

    expected.a = 35;
    expected.b = 26;
    s = dcNewAggr(2, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_ULONGLONG, offsetof(U64_U64, a), 1);
    dcAggrField(s, DC_SIGCHAR_ULONGLONG, offsetof(U64_U64, b), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("LL)A", &cbU64U64ReturnHandler, &userdata, aggrs);

    result = ((U64_U64(*)(unsigned long long, unsigned long long))cb)(25, 26);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %lld %lld): %lld %lld\n", expected.a, expected.b, result.a, result.b);

    ret = result.a == expected.a && result.b == expected.b && ret;
  }
  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    Double_Double expected, result;

    expected.a = 11.5;
    expected.b = 2.5;
    s = dcNewAggr(2, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Double_Double, a), 1);
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Double_Double, b), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("dd)A", &cbDoubleDoubleReturnHandler, &userdata, aggrs);

    result = ((Double_Double(*)(double, double))cb)(1.5, 2.5);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %f %f): %f %f\n", expected.a, expected.b, result.a, result.b);

    ret = result.a == expected.a && result.b == expected.b && ret;
  }
  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    Three_U64 expected, result;

    expected.a = 11;
    expected.b = 2;
    expected.c = 3;
    s = dcNewAggr(3, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_ULONGLONG, offsetof(Three_U64, a), 1);
    dcAggrField(s, DC_SIGCHAR_ULONGLONG, offsetof(Three_U64, b), 1);
    dcAggrField(s, DC_SIGCHAR_ULONGLONG, offsetof(Three_U64, c), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("LLL)A", &cbThreeU64ReturnHandler, &userdata, aggrs);

    result = ((Three_U64(*)(unsigned long long, unsigned long long, unsigned long long))cb)(1, 2, 3);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %lld %lld %lld): %lld %lld %lld\n", expected.a, expected.b, expected.c, result.a, result.b, result.c);

    ret = result.a == expected.a && result.b == expected.b && result.c == expected.c && ret;
  }
  {
    DCCallback* cb;
    DCaggr *s;
    DCaggr *aggrs[1];
    int userdata = 10;
    Three_Double expected, result;

    expected.a = 11.5;
    expected.b = 2.5;
    expected.c = 3.5;
    s = dcNewAggr(3, sizeof(expected));
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, a), 1);
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, b), 1);
    dcAggrField(s, DC_SIGCHAR_DOUBLE, offsetof(Three_Double, c), 1);
    dcCloseAggr(s);

    aggrs[0] = s;

    cb = dcbNewCallback2("ddd)A", &cbThreeDoubleReturnHandler, &userdata, aggrs);

    result = ((Three_Double(*)(double, double, double))cb)(1.5, 2.5, 3.5);
    dcbFreeCallback(cb);
    dcFreeAggr(s);

    printf("successfully returned from callback\n");
    printf("return value (should be %f %f %f): %f %f %f\n", expected.a, expected.b, expected.c, result.a, result.b, result.c);

    ret = result.a == expected.a && result.b == expected.b && result.c == expected.c && ret;
  }

  return ret;
}
#endif


int main()
{
  int result = 1;

  dcTest_initPlatform();

  result = testSimpleCallback() && result;
#if defined(DC__Feature_AggrByVal)
  result = testAggrArgsCallback() && result;
  result = testAggrReturnCallback() && result;
#endif

  printf("result: callback_plain: %d\n", result);

  dcTest_deInitPlatform();

  return !result;
}

