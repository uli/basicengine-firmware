#include "basic.h"

/***bf m SIN
Returns the sine of a specified angle.
\usage s = SIN(angle)
\args
@angle	an angle expressed in radians
\ret Sine of `angle`.
\ref COS() TAN()
***/
num_t BASIC_FP Basic::nsin() {
  return sin(getparam());
}
/***bf m COS
Returns the cosine of a specified angle.
\usage c = COS(angle)
\args
@angle	an angle expressed in radians
\ret Cosine of `angle`.
\ref SIN() TAN()
***/
num_t BASIC_FP Basic::ncos() {
  return cos(getparam());
}
/***bf m EXP
Returns _e_ raised to a specified power, where _e_ is the base of natural
logarithms.
\usage ex = EXP(pow)
\args
@pow	Power to raise _e_ to.
\ret Exponential value of `pow`.
***/
num_t BASIC_FP Basic::nexp() {
  return exp(getparam());
}
/***bf m ATN
Arc tangent function.
\usage a = ATN(x)
\args
@x	any numeric expression
\desc
`ATN()` calculates the principal value of the arc tangent of `x`; that  is the
value whose tangent is `x`.
\ret The principal value of the arc tangent of `x` in radians.
\ref ATN2()
***/
num_t BASIC_FP Basic::natn() {
  return atan(getparam());
}

/***bf m ATN2
Arc tangent function of two variables.
\usage a2 = ATN2(x, y)
\args
@x	any numeric expression
@y	any numeric expression
\desc
Calculates the principal value of the arc tangent of `y`/`x`, using the signs of
the two arguments to determine the quadrant of the result.
\ret Arc tangent of `y`/`x`.
\ref ATN()
***/
num_t BASIC_FP Basic::natn2() {
  num_t value, value2;
  if (checkOpen() || getParam(value, I_COMMA) || getParam(value2, I_CLOSE))
    return 0;
  return atan2(value, value2);
}

/***bf m SQR
Square root function.
\usage s = SQR(num)
\args
@num	any numeric expression larger than or equal to `0`
\ret The nonnegative square root of `num`.
***/
num_t BASIC_FP Basic::nsqr() {
  return sqrt(getparam());
}

/***bf m TAN
Returns the tangent of a specified angle.
\usage t = TAN(angle)
\args
@angle	an angle expressed in radians
\ret Tangent of `angle`.
\ref COS() SIN()
***/
num_t BASIC_FP Basic::ntan() {
  return tan(getparam());
}

/***bf m LOG
Returns the natural logarithm of a numeric expression.
\usage l = LOG(num)
\args
@num	any numeric expression
\ret The natural logarithm of `num`.
***/
num_t BASIC_FP Basic::nlog() {
  return log(getparam());
}

/***bf m INT
Returns the largest integer less than or equal to a numeric expression.
\usage i = INT(num)
\args
@num	any numeric expression
\ret `num` rounded down to the nearest integer.
***/
num_t BASIC_FP Basic::nint() {
  return floor(getparam());
}

/***bf m SGN
Returns a value indicating the sign of a numeric expression.
\usage s = SGN(num)
\args
@num	any numeric expression
\ret
`1` if the expression is positive, `0` if it is zero, or `-1` if it is
negative.
***/
num_t BASIC_FP Basic::nsgn() {
  num_t value = getparam();
  return (0 < value) - (value < 0);
}

// multiply or divide calculation
num_t BASIC_FP Basic::imul() {
  num_t value, tmp; //値と演算値

  while (1) {
    switch (*cip++) {
/***bo op - (unary)
Negation operator.
\usage -a
\res The negative of `a`.
\prec 2
***/
    case I_MINUS:         //「-」
      return 0 - imul();  //値を取得して負の値に変換
      break;
/***bo op + (unary)
Identity operator.
\usage +a
\res `a`.
\note
This operator serves decorative purposes only.
\prec 2
***/
    case I_PLUS:
      return imul();
      break;
    default:
      cip--;
      value = ivalue();
      goto out;
    }
  }
out:

  if (err)
    return -1;

  while (1)            //無限に繰り返す
    switch (*cip++) {  //中間コードで分岐

/***bo op *
Multiplication operator.
\usage a * b
\res Product of `a` and `b`.
\prec 3
***/
    case I_MUL: //掛け算の場合
      tmp = ivalue();
      value *= tmp; //掛け算を実行
      break;

/***bo op /
Division operator.
\usage a / b
\res Quotient of `a` and `b`.
\prec 3
***/
    case I_DIV: //割り算の場合
      tmp = ivalue();
      if (err)
        return -1;
      value /= tmp; //割り算を実行
      break;

/***bo op MOD
Modulus operator.
\usage a MOD b
\res Remainder of division of `a` and `b`.
\prec 3
***/
    case I_MOD: //剰余の場合
      tmp = ivalue();
      if (err)
        return -1;
      value = (int64_t)value % (int64_t)tmp; //割り算を実行
      break;

/***bo op <<
Bit-shift operator, left.
\usage a << b
\res `a` converted to an unsigned integer and shifted left by `b` bits.
\prec 3
***/
    case I_LSHIFT: // シフト演算 "<<" の場合
      tmp = ivalue();
      value = ((uint32_t)value) << (uint32_t)tmp;
      break;

/***bo op >>
Bit-shift operator, right.
\usage a >> b
\res `a` converted to an unsigned integer and shifted right by `b` bits.
\prec 3
***/
    case I_RSHIFT: // シフト演算 ">>" の場合
      tmp = ivalue();
      value = ((uint32_t)value) >> (uint32_t)tmp;
      break;

    default:
      --cip;
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

// add or subtract calculation
num_t BASIC_FP Basic::iplus() {
  num_t value, tmp;  //値と演算値
  value = imul();    //値を取得
  if (err)
    return -1;

  while (1)
    switch (*cip) {
/***bo op +
Numeric addition operator.
\usage a + b
\res Sum of `a` and `b`.
\prec 4
***/
    case I_PLUS: //足し算の場合
      cip++;
      tmp = imul();
      value += tmp; //足し算を実行
      break;

/***bo op -
Numeric subtraction operator.
\usage a - b
\res Difference of `a` and `b`.
\prec 4
***/
    case I_MINUS: //引き算の場合
      cip++;
      tmp = imul();
      value -= tmp; //引き算を実行
      break;

    default:
      if (!math_exceptions_disabled && !err && !finite(value)) {
        if (isinf(value))
          err = ERR_DIVBY0;
        else
          err = ERR_FP;
      }
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

num_t BASIC_FP Basic::irel() {
  num_t value, tmp;
  value = iplus();

  if (err)
    return -1;

  // conditional expression
  while (1)
    switch (*cip++) {
/***bo op =
Equality operator.
\usage a = b
\res `-1` if `a` and `b` are equal, `0` otherwise.
\prec 6
***/
    case I_EQ:
      tmp = iplus();
      value = basic_bool(value == tmp);
      break;
/***bo op <>
Inequality operator.
\usage
a <> b

a >< b
\res `-1` if `a` and `b` are not equal, `0` otherwise.
\prec 6
***/
    case I_NEQ:
    case I_NEQ2:
      tmp = iplus();
      value = basic_bool(value != tmp);
      break;
/***bo op <
Less-than operator.
\usage
a < b
\res `-1` if `a` is less than `b`, `0` otherwise.
\prec 6
***/
    case I_LT:
      tmp = iplus();
      value = basic_bool(value < tmp);
      break;
/***bo op \<=
Less-than-or-equal operator.
\usage
a <= b
\res `-1` if `a` is less than or equal to `b`, `0` otherwise.
\prec 6
***/
    case I_LTE:
      tmp = iplus();
      value = basic_bool(value <= tmp);
      break;
/***bo op >
Greater-than operator.
\usage
a > b
\res `-1` if `a` is greater than `b`, `0` otherwise.
\prec 6
***/
    case I_GT:
      tmp = iplus();
      value = basic_bool(value > tmp);
      break;
/***bo op >=
Greater-than-or-equal operator.
\usage
a >= b
\res `-1` if `a` is greater than or equal to `b`, `0` otherwise.
\prec 6
***/
    case I_GTE:
      tmp = iplus();
      value = basic_bool(value >= tmp);
      break;
    default:
      cip--;
      return value;
    }
}

num_t BASIC_FP Basic::iand() {
  num_t value, tmp;

  switch (*cip++) {
/***bo op NOT
Bitwise inversion operator.
\usage NOT a
\res Bitwise inversion of `a`.
\prec 7
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical NOT" operator; instead, the bitwise operator is used.
***/
  case I_BITREV: // NOT
    return ~((int64_t)irel());
  default:
    cip--;
    value = irel();
  }

  if (err)
    return -1;

  while (1)
    switch (*cip++) {
    case I_AND:
/***bo op AND
Bitwise AND operator.
\usage a AND b
\res Bitwise conjunction of `a` and `b`.
\prec 7
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical AND" operator; instead, the bitwise operator is used.
***/
      tmp = iand();
      value = ((int64_t)value) & ((int64_t)tmp);
      break;
    default:
      cip--;
      return value;
    }
}

// Numeric expression parser
num_t BASIC_FP Basic::iexp() {
  num_t value, tmp;

  value = iand();
  if (err)
    return -1;

  while (1)
    switch (*cip++) {
/***bo op OR
Bitwise OR operator.
\usage a OR b
\res Bitwise inclusive disjunction of `a` and `b`.
\prec 8
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical OR" operator; instead, the bitwise operator is used.
***/
    case I_OR:
      tmp = iand();
      value = ((int64_t)value) | ((int64_t)tmp);
      break;
/***bo op EOR
Bitwise exclusive-OR operator.
\usage a EOR b
\res Bitwise exclusive disjunction of `a` and `b`.
\prec 8
***/
    case I_XOR:
      tmp = iand();
      value = ((int64_t)value) ^ ((int64_t)tmp);
      break;
    default:
      cip--;
      return value;
    }
}
