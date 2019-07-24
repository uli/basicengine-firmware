#include "basic.h"

static inline bool is_var(unsigned char tok)
{
  return tok >= I_VAR && tok <= I_STRLSTREF;
}

// Variable assignment handler
void BASIC_FP Basic::ivar() {
  num_t value; //値
  short index; //変数番号

  index = *cip++; //変数番号を取得して次へ進む

  if (*cip != I_EQ) {
    // Unlike other cases of missing assignment operators (which can only
    // occur if some sort of syntactic element that unambigously identifies
    // them as variable identifiers is used), this one is most likely caused
    // by an unknown or misspelled command.
    err = ERR_UNK;
    err_expected = nvar_names.name(index);
    return;
  }
  cip++;
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  nvar.var(index) = value;
}

// Local variables are handled by encoding them with global variable indices.
//
// When a local variable is created (either through a procedure signature or
// by referencing it), a global variable entry of the same name is created
// (unless it already exists), and its index is used to encode the local
// variable in the program text.
//
// When a local variable is dereferenced, the global index is translated to
// an argument stack offset depending on which procedure is currently running.
// So global and local variables of the same name share entries in the variable
// table, but the value in the table is only used by the global variable. The
// local's value resides on the argument stack.
//
// The reason for doing this in such a convoluted way is that we don't know
// what a procedure's stack frame looks like at compile time because we may
// have to compile code out-of-order.

int BASIC_FP Basic::get_num_local_offset(uint8_t arg, bool &is_local)
{
  is_local = false;
  if (!gstki) {
    // not in a subroutine
    err = ERR_GLOBAL;
    return 0;
  }
  uint8_t proc_idx = gstk[gstki-1].proc_idx;
  if (proc_idx == NO_PROC) {
    err = ERR_GLOBAL;
    return 0;
  }
  int local_offset = procs.getNumArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = procs.getNumLoc(proc_idx, arg);
    if (local_offset < 0) {
      err = ERR_ASTKOF;
      return 0;
    }
    is_local = true;
  }
  return local_offset;
}

num_t& BASIC_FP Basic::get_lvar(uint8_t arg)
{
  bool is_local;
  int local_offset = get_num_local_offset(arg, is_local);
  if (err)
    return astk_num[0];	// dummy
  if (is_local) {
    if (astk_num_i + local_offset >= SIZE_ASTK) {
      err = ERR_ASTKOF;
      return astk_num[0];	// dummy
    }
    return astk_num[astk_num_i + local_offset];
  } else {
    uint16_t argc = gstk[gstki-1].num_args;
    return astk_num[astk_num_i - argc + local_offset];
  }
}

void BASIC_FP Basic::ilvar() {
  num_t value;
  short index;	// variable index

  index = *cip++;

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;
  value = iexp();
  if (err)
    return;
  num_t &var = get_lvar(index);
  if (err)
    return;
  var = value;
}

int BASIC_FP Basic::get_array_dims(int *idxs) {
  int dims = 0;
  while (dims < MAX_ARRAY_DIMS) {
    if (getParam((int32_t&)idxs[dims], I_NONE))
      return -1;
    dims++;
    if (*cip == I_CLOSE)
      break;
    if (*cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return -1;
    }
    cip++;
  }
  cip++;
  return dims;
}

/***bc bas DIM
Declares an array.
\usage DIM variable(dimension[, ...]) [= [value, ...]]
\args
@variable	an numeric or string array variable
@dimension	one or more dimensions of the array
@value		initial value(s) for the array elements
\note
Initialization is only possible for one-dimensional arrays.
***/
void Basic::idim() {
  int dims = 0;
  int idxs[MAX_ARRAY_DIMS];
  bool is_string;
  uint8_t index;

  for (;;) {
    if (*cip != I_VARARR && *cip != I_STRARR) {
      SYNTAX_T("not an array variable");
      return;
    }
    is_string = *cip == I_STRARR;
    ++cip;

    index = *cip++;

    dims = get_array_dims(idxs);
    if (dims < 0)
      return;
    if (dims == 0) {
      SYNTAX_T("missing dimensions");
      return;
    }

    // BASIC convention: reserve one more element than specified
    for (int i = 0; i < dims; ++i)
      idxs[i]++;

    if ((!is_string && num_arr.var(index).reserve(dims, idxs)) ||
        (is_string  && str_arr.var(index).reserve(dims, idxs))) {
      err = ERR_OOM;
      return;
    }

    if (*cip == I_EQ) {
      // Initializers
      if (dims > 1) {
        err = ERR_NOT_SUPPORTED;
      } else {
        if (*++cip != I_SQOPEN) {
          E_SYNTAX(I_SQOPEN);
          return;
        }
        if (is_string) {
          BString svalue;
          int cnt = 0;
          do {
            cip++;
            svalue = istrexp();
            if (err)
              return;
            BString &s = str_arr.var(index).var(1, &cnt);
            if (err)
              return;
            s = svalue;
            cnt++;
          } while(*cip == I_COMMA);
        } else {
          num_t value;
          int cnt = 0;
          do {
            cip++;
            value = iexp();
            if (err)
              return;
            num_t &n = num_arr.var(index).var(1, &cnt);
            if (err)
              return;
            n = value;
            cnt++;
          } while(*cip == I_COMMA);
        }
        if (*cip++ != I_SQCLOSE) {
          E_SYNTAX(I_SQCLOSE);
          return;
        }
      }
      break;
    }
    if (*cip != I_COMMA)
      break;
    ++cip;
  }

  return;
}

// Numeric array variable assignment handler
void BASIC_FP Basic::ivararr() {
  num_t value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++; //変数番号を取得して次へ進む

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;
  num_t &n = num_arr.var(index).var(dims, idxs);
  if (err)
    return;

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return;
  }
  cip++;
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  n = value;
}

int Basic::get_str_local_offset(uint8_t arg, bool &is_local)
{
  is_local = false;
  if (!gstki) {
    // not in a subroutine
    err = ERR_GLOBAL;
    return 0;
  }
  uint8_t proc_idx = gstk[gstki-1].proc_idx;
  if (proc_idx == NO_PROC) {
    err = ERR_GLOBAL;
    return 0;
  }
  int local_offset = procs.getStrArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = procs.getStrLoc(proc_idx, arg);
    if (local_offset < 0) {
      err = ERR_ASTKOF;
      return 0;
    }
    is_local = true;
  }
  return local_offset;
}

BString& Basic::get_lsvar(uint8_t arg)
{
  bool is_local;
  int local_offset = get_str_local_offset(arg, is_local);
  if (err)
    return astk_str[0];	// dummy
  if (is_local) {
    if (astk_str_i + local_offset >= SIZE_ASTK) {
      err = ERR_ASTKOF;
      return astk_str[0];	// dummy
    }
    return astk_str[astk_str_i + local_offset];
  } else {
    uint16_t argc = gstk[gstki-1].str_args;
    return astk_str[astk_str_i - argc + local_offset];
  }
}

void Basic::set_svar(bool is_lsvar) {
  BString value;
  uint8_t index = *cip++;
  int32_t offset;
  uint8_t sval;

  if (*cip == I_SQOPEN) {
    // String character accessor
    ++cip;

    if (getParam(offset, 0, svar.var(index).length(), I_SQCLOSE))
      return;

    if (*cip != I_EQ) {
      err = ERR_VWOEQ;
      return;
    }
    cip++;
    
    sval = iexp();
    if (is_lsvar) {
      BString &str = get_lsvar(index);
      if (err)
        return;
      str[offset] = sval;
    } else {
      svar.var(index)[offset] = sval;
    }

    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  if (*cip == I_SQOPEN) {
    uint32_t element;
    ++cip;
    for (;; ++cip) {
      if (getParam(element, 0, 255, I_NONE))
        return;
      value.concat((char)element);

      if (*cip == I_SQCLOSE) {
        ++cip;
        break;
      }
      if (*cip != I_COMMA) {
        E_ERR(SYNTAX, ", or ] expected");
        return;
      }
    }
  } else {
    value = istrexp();
    if (err)
      return;
  }

  if (is_lsvar) {
    BString &str = get_lsvar(index);
    if (err)
      return;
    str = value;
  } else {
    svar.var(index) = value;
  }
}

void Basic::isvar() {
  set_svar(false);
}

void Basic::ilsvar() {
  set_svar(true);
}

// String array variable assignment handler
void Basic::istrarr() {
  BString value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;
  BString &s = str_arr.var(index).var(dims, idxs);
  if (err)
    return;

  if (*cip == I_SQOPEN) {
    // String character accessor
    ++cip;

    int32_t offset;
    if (getParam(offset, 0, s.length(), I_SQCLOSE))
      return;

    if (*cip != I_EQ) {
      err = ERR_VWOEQ;
      return;
    }
    cip++;
    
    uint8_t sval = iexp();
    if (err)
      return;

    s[offset] = sval;

    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = istrexp();
  if (err)
    return;

  s = value;
}

// String list variable assignment handler
void Basic::istrlst() {
  BString value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims != 1) {
    SYNTAX_T("invalid list index");
    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = istrexp();
  if (err)
    return;

  BString &s = str_lst.var(index).var(idxs[0]);
  if (err)
    return;
  s = value;
}

// Numeric list variable assignment handler
void Basic::inumlst() {
  num_t value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims != 1) {
    SYNTAX_T("invalid list index");
    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = iexp();
  if (err)
    return;

  num_t &s = num_lst.var(index).var(idxs[0]);
  if (err)
    return;
  s = value;
}

void Basic::inumlstref() {
  uint8_t index = *cip++;
  num_t value;

  if (*cip++ != I_EQ) {
    E_SYNTAX(I_EQ);
    return;
  }
  if (*cip != I_SQOPEN) {
    E_SYNTAX(I_SQOPEN);
    return;
  }

  num_lst.var(index).reset();
  do {
    cip++;
    value = iexp();
    if (err)
      return;
    num_lst.var(index).append(value);
    if (err)
      return;
  } while(*cip == I_COMMA);

  if (*cip++ != I_SQCLOSE) {
    E_SYNTAX(I_SQCLOSE);
    return;
  }
}

void Basic::istrlstref() {
  uint8_t index = *cip++;
  BString value;

  if (*cip++ != I_EQ) {
    E_SYNTAX(I_EQ);
    return;
  }
  if (*cip != I_SQOPEN) {
    E_SYNTAX(I_SQOPEN);
    return;
  }

  str_lst.var(index).reset();
  do {
    cip++;
    value = istrexp();
    if (err)
      return;
    str_lst.var(index).append(value);
    if (err)
      return;
  } while(*cip == I_COMMA);

  if (*cip++ != I_SQCLOSE) {
    E_SYNTAX(I_SQCLOSE);
    return;
  }
}

void Basic::iappend() {
  uint8_t index;
  if (*cip == I_STRLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    BString value = istrexp();
    if (err)
      return;
    str_lst.var(index).append(value);
  } else if (*cip == I_NUMLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    num_t value = iexp();
    if (err)
      return;
    num_lst.var(index).append(value);
  } else {
    SYNTAX_T("exp list reference");
  }
  return;
}

void Basic::iprepend() {
  uint8_t index;
  if (*cip == I_STRLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    BString value = istrexp();
    if (err)
      return;
    str_lst.var(index).prepend(value);
  } else if (*cip == I_NUMLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    num_t value = iexp();
    if (err)
      return;
    num_lst.var(index).prepend(value);
  } else {
    SYNTAX_T("exp list reference");
  }
  return;
}

/***bc bas LET
Assigns the value of an expression to a variable. 
\usage LET variable = expression
\args
@variable	any variable
@expression	any expression that is of the same type as `variable`
\note
The use of this command is optional; any variable assignment can be
written as `variable = expression`, without prefixing it with `LET`.

It is provided solely as a compatibility feature with other
BASIC dialects. Its use in new programs is not recommended, as it
consumes additional memory and compute resources without providing
any benefit.
***/
// XXX: 32-byte jump table
void BASIC_INT __attribute__((optimize ("no-jump-tables"))) Basic::ilet() {
  switch (*cip) { //中間コードで分岐
  case I_VAR: // 変数の場合
    cip++;     // 中間コードポインタを次へ進める
    ivar();    // 変数への代入を実行
    break;

  case I_LVAR:
    cip++;
    ilvar();
    break;

  case I_VARARR:
    cip++;
    ivararr();
    break;

  case I_SVAR:
    cip++;
    isvar();
    break;

  case I_LSVAR:
    cip++;
    ilsvar();
    break;

  case I_STRARR:
    cip++;
    istrarr();
    break;

  case I_STRLST:
    cip++;
    istrlst();
    break;

  case I_NUMLST:
    cip++;
    inumlst();
    break;

  case I_NUMLSTREF:
    cip++;
    inumlstref();
    break;

  case I_STRLSTREF:
    cip++;
    istrlstref();
    break;

  default:      // 以上のいずれにも該当しなかった場合
    err = ERR_LETWOV; // エラー番号をセット
    break;            // 打ち切る
  }

  if (err == ERR_UNK) {
    // Variable assignment code assumes that a malformed assignment is
    // usually caused by a misspelled command and reports it as such; in our
    // case, however, we know it's supposed to be an assignment, so we report
    // it as such.
    err = ERR_VWOEQ;
    err_expected = NULL;
  }
}

/***bf bas POPF
Remove an element from the beginning of a numeric list.
\usage e = POPF(~list)
\args
@~list	reference to a numeric list
\ret Value of the element removed.
\error
An error is generated if `~list` is empty.
\ref POPB()
***/
num_t BASIC_FP Basic::npopf() {
  num_t value;
  if (checkOpen()) return 0;
  if (*cip++ == I_NUMLSTREF) {
    value = num_lst.var(*cip).front();
    num_lst.var(*cip++).pop_front();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

/***bf bas POPB
Remove an element from the end of a numeric list.
\usage e = POPB(~list)
\args
@~list	reference to a numeric list
\ret Value of the element removed.
\error
An error is generated if `~list` is empty.
\ref POPF()
***/
num_t BASIC_FP Basic::npopb() {
  num_t value;
  if (checkOpen()) return 0;
  if (*cip++ == I_NUMLSTREF) {
    value = num_lst.var(*cip).back();
    num_lst.var(*cip++).pop_back();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

/***bf bas POPF$
Remove an element from the beginning of a string list.
\usage e$ = POPF$(~list$)
\args
@~list$	reference to a string list
\ret Value of the element removed.
\error
An error is generated if `~list$` is empty.
\ref POPB$()
***/
BString BASIC_INT Basic::spopf() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).front();
    str_lst.var(*cip++).pop_front();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp string list reference");
    return value;
  }
  checkClose();
  return value;
}

/***bf bas POPB$
Remove an element from the end of a string list.
\usage e$ = POPB$(~list$)
\args
@~list$	reference to a string list
\ret Value of the element removed.
\error
An error is generated if `~list$` is empty.
\ref POPF$()
***/
BString BASIC_INT Basic::spopb() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).back();
    str_lst.var(*cip++).pop_back();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp string list reference");
    return value;
  }
  checkClose();
  return value;
}

