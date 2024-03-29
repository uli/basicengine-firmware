// SPDX-License-Identifier: MIT
/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2018 Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include <Arduino.h>
#include <stdlib.h>
#include "BString.h"
#include "error.h"
#include "kwenum.h"

#define FLOAT_NUMS
#ifdef FLOAT_NUMS
typedef double num_t;
#define strtonum strtod
#else
typedef int32_t num_t;
#define strtonum strtol
#endif

//#define DEBUG_VAR
#ifdef DEBUG_VAR
#define dbg_var(x...) \
  do {                \
    Serial.printf(x); \
    Serial.flush();   \
  } while (0)
#else
#define dbg_var(x...)
#endif

#define varRealloc(p, nu) \
  (__typeof__((p)))realloc((p), nu * sizeof(*(p)));

class VarNames {
public:
  VarNames() {
    m_var_top = m_prg_var_top = m_size = 0;
    m_var_name = NULL;
  }

  ~VarNames() {
    deleteAll();
  }

  inline bool reserve(unsigned int count) {
    dbg_var("vnames reserve %d\r\n", count);
    if (count > m_size)
      return doReserve(count);
    else
      return false;
  }

  void deleteDownTo(index_t idx) {
    dbg_var("vnames del dto %d\r\n", idx);
    for (index_t i = idx; i < m_var_top; ++i) {
      if (m_var_name[i]) {
        free((void *)m_var_name[i]);
        m_var_name[i] = NULL;
      }
    }
    m_var_top = idx;
  }

  void deleteAll() {
    dbg_var("vnames del all\r\n");
    deleteDownTo(0);
    if (m_var_name) {
      free(m_var_name);
      m_var_name = NULL;
    }
    m_size = m_prg_var_top = m_var_top = 0;
  }

  void deleteDirect() {
    dbg_var("vnames del dir\r\n");
    deleteDownTo(m_prg_var_top);
    m_var_top = m_prg_var_top;
  }

  int find(const char *name) {
    dbg_var("vnames find %s\r\n", name);
    for (index_t i = 0; i < m_var_top; ++i) {
      if (!strcasecmp(name, m_var_name[i])) {
#ifdef DEBUG_VAR
        Serial.printf("found %d\r\n", i);
#endif
        return i;
      }
    }
    return -1;
  }

  int assign(const char *name, bool is_prg_text) {
    dbg_var("vnames assign %s text %d\r\n", name, is_prg_text);
#ifdef DEBUG_VAR
    Serial.printf("assign %s ", name);
#endif
    int v = find(name);
    if (v >= 0) {
      // This variable may have been created in direct mode, but it is now
      // referenced in program mode, so we have to make sure it is
      // preserved.
      if (is_prg_text && m_prg_var_top < (unsigned int)v + 1)
        m_prg_var_top = v + 1;
      return v;
    }

    if (reserve(m_var_top + 1))
      return -1;

    m_var_name[m_var_top++] = strdup(name);
    if (is_prg_text)
      m_prg_var_top = m_var_top;
    dbg_var("got %d\r\n", m_var_top - 1);
    return m_var_top - 1;
  }

  inline const char *name(index_t idx) {
    return m_var_name[idx];
  }

  inline int varTop() {
    return m_var_top;
  }
  inline int prgVarTop() {
    return m_prg_var_top;
  }

protected:
  unsigned int m_var_top;
  unsigned int m_prg_var_top;
  unsigned int m_size;

private:
  bool doReserve(unsigned int count) {
    m_var_name = varRealloc(m_var_name, count);
    if (!m_var_name) {
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i)
      m_var_name[i] = NULL;
    m_size = count;
    return false;
  }

  char **m_var_name;
};

class NumVariables {
public:
  NumVariables() {
    m_size = 0;
    m_var = NULL;
  }

  ~NumVariables() {
    free(m_var);
  }

  void reset() {
    for (index_t i = 0; i < m_size; ++i)
      m_var[i] = 0;
  }

  bool reserve(index_t count) {
    dbg_var("nv reserve %d\r\n", count);
    if (count == 0) {
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    m_var = varRealloc(m_var, count);
    if (!m_var) {
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i)
      m_var[i] = 0;
    m_size = count;
    return false;
  }

  inline unsigned int size() {
    return m_size;
  }

  inline num_t& var(index_t index) {
    return m_var[index];
  }

private:
  unsigned int m_size;
  num_t *m_var;
};

template <typename T> class NumArray {
public:
  NumArray() {
    m_dims = 0;
    m_sizes = NULL;
    m_var = NULL;
    m_total = 0;
  }

  ~NumArray() {
    if (m_sizes)
      free(m_sizes);
    if (m_var)
      free(m_var);
  }

  void reset() {
    m_dims = 0;
    m_total = 0;
    if (m_sizes) {
      free(m_sizes);
      m_sizes = NULL;
    }
    if (m_var) {
      free(m_var);
      m_var = NULL;
    }
  }

  bool reserve(int dims, int *sizes) {
    dbg_var("na reserve dims %d\r\n", dims);
    m_sizes = varRealloc(m_sizes, dims);
    m_dims = dims;
    if (!m_sizes) {
      m_dims = 0;
      err = ERR_OOM;
      return true;
    }
    memcpy(m_sizes, sizes, dims * sizeof(int));
    m_total = 1;
    for (int i = 0; i < dims; ++i) {
      m_total *= sizes[i];
    }
    dbg_var("na total %d\r\n", m_total);
    m_var = varRealloc(m_var, m_total);
    if (!m_var) {
      free(m_sizes);
      m_dims = 0;
      m_sizes = NULL;
      m_var = NULL;
      err = ERR_OOM;
      return true;
    }
    for (index_t i = 0; i < m_total; ++i) {
      m_var[i] = 0;
    }
    return false;
  }

  inline T& var(int dims, int *idxs) {
    if (!m_sizes) {
      err = ERR_UNDEFARR;
      // XXX: Is it possible to return an invalid reference without crashing?
      return bull;
    }
    if (dims != m_dims) {
      err = ERR_SOR;
      // XXX: Likewise.
      return bull;
    }
    int mul = 1;
    int idx = 0;
    for (int i = 0; i < m_dims; ++i) {
      if (idxs[i] < 0 || idxs[i] >= m_sizes[i]) {
        err = ERR_SOR;
        // XXX: Likewise.
        return bull;
      }
      idx += idxs[i] * mul;
      mul *= m_sizes[i];
    }
    return m_var[idx];
  }

  inline int dims() {
    return m_dims;
  }

private:
  int m_dims;
  unsigned int m_total;
  int *m_sizes;
  T *m_var;
  T bull;
};

template <typename T> class NumArrayVariables {
public:
  NumArrayVariables() {
    m_size = 0;
    m_var = NULL;
  }

  ~NumArrayVariables() {
    reserve(0);
  }

  void reset() {
    for (index_t i = 0; i < m_size; ++i)
      m_var[i]->reset();
  }

  bool reserve(index_t count) {
    dbg_var("na reserve %d\r\n", count);
    if (count == 0) {
      for (index_t i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }

    if (count < m_size) {
      for (index_t i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }

    m_var = varRealloc(m_var, count);
    if (!m_var) {
      // XXX: How are we going to delete the elements already there before
      // the failed realloc() call?
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i) {
      m_var[i] = new NumArray<T>();
      if (!m_var[i]) {
        for (index_t j = m_size; j < i; ++j)
          delete m_var[j];
        err = ERR_OOM;
        return true;
      }
    }
    m_size = count;
    return false;
  }

  inline unsigned int size() {
    return m_size;
  }

  inline NumArray<T>& var(index_t index) {
    return *m_var[index];
  }

private:
  unsigned int m_size;
  NumArray<T> **m_var;
};

class StringVariables {
public:
  StringVariables() {
    m_size = 0;
    m_var = NULL;
  }

  ~StringVariables() {
    reserve(0);
  }

  void reset() {
    for (index_t i = 0; i < m_size; ++i)
      *m_var[i] = "";
  }

  bool reserve(index_t count) {
    dbg_var("sv reserve %d\r\n", count);
    if (count == 0) {
      for (index_t i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    if (count < m_size) {
      for (index_t i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }
    m_var = varRealloc(m_var, count);
    if (!m_var) {
      // XXX: How are we going to delete the elements already there before
      // the failed realloc() call?
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i) {
      m_var[i] = new BString();
      if (!m_var[i]) {
        for (index_t j = m_size; j < i; ++j)
          delete m_var[j];
        err = ERR_OOM;
        return true;
      }
      *m_var[i] = "";
    }
    m_size = count;
    return false;
  }

  inline unsigned int size() {
    return m_size;
  }

  inline BString& var(index_t index) {
    return *m_var[index];
  }

private:
  unsigned int m_size;
  BString **m_var;
};

template <typename T> class StringArray {
public:
  StringArray() {
    m_dims = 0;
    m_sizes = NULL;
    m_var = NULL;
    m_total = 0;
  }

  ~StringArray() {
    if (m_sizes)
      free(m_sizes);
    if (m_var)
      delete[] m_var;
  }

  void reset() {
    m_dims = 0;
    m_total = 0;
    if (m_sizes) {
      free(m_sizes);
      m_sizes = NULL;
    }
    if (m_var) {
      delete[] m_var;
      m_var = NULL;
    }
  }

  bool reserve(int dims, int *sizes) {
    dbg_var("sa reserve dims %d\r\n", dims);
    m_sizes = varRealloc(m_sizes, dims);
    m_dims = dims;
    if (!m_sizes) {
      m_dims = 0;
      err = ERR_OOM;
      return true;
    }
    memcpy(m_sizes, sizes, dims * sizeof(int));
    m_total = 1;
    for (int i = 0; i < dims; ++i) {
      m_total *= sizes[i];
    }
    dbg_var("na total %d\r\n", m_total);
    if (m_var)
      delete[] m_var;
    m_var = new BString[m_total];
    if (!m_var) {
      free(m_sizes);
      m_dims = 0;
      m_sizes = NULL;
      m_var = NULL;
      err = ERR_OOM;
      return true;
    }
    return false;
  }

  inline T& var(int dims, int *idxs) {
    if (!m_sizes) {
      err = ERR_UNDEFARR;
      // XXX: Is it possible to return an invalid reference without crashing?
      return bull;
    }
    if (dims != m_dims) {
      err = ERR_SOR;
      // XXX: Likewise.
      return bull;
    }
    int mul = 1;
    int idx = 0;
    for (int i = 0; i < m_dims; ++i) {
      if (idxs[i] < 0 || idxs[i] >= m_sizes[i]) {
        err = ERR_SOR;
        // XXX: Likewise.
        return bull;
      }
      idx += idxs[i] * mul;
      mul *= m_sizes[i];
    }
    return m_var[idx];
  }

  inline int dims() {
    return m_dims;
  }

private:
  int m_dims;
  int m_total;
  int *m_sizes;
  T *m_var;
  T bull;
};

template <typename T> class StringArrayVariables {
public:
  StringArrayVariables() {
    m_size = 0;
    m_var = NULL;
  }

  ~StringArrayVariables() {
    reserve(0);
  }

  void reset() {
    for (index_t i = 0; i < m_size; ++i)
      m_var[i]->reset();
  }

  bool reserve(index_t count) {
    dbg_var("sa reserve %d\r\n", count);
    if (count == 0) {
      for (index_t i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }

    if (count < m_size) {
      for (index_t i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }

    m_var = varRealloc(m_var, count);
    if (!m_var) {
      // XXX: How are we going to delete the elements already there before
      // the failed realloc() call?
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i) {
      m_var[i] = new StringArray<T>();
      if (!m_var[i]) {
        for (index_t j = m_size; j < i; ++j)
          delete m_var[j];
        err = ERR_OOM;
        return true;
      }
    }
    m_size = count;
    return false;
  }

  inline unsigned int size() {
    return m_size;
  }

  inline StringArray<T>& var(index_t index) {
    return *m_var[index];
  }

private:
  unsigned int m_size;
  StringArray<T> **m_var;
};

#include "QList.h"

template <typename T> class BasicList {
public:
  BasicList() {
    m_list.clear();
  }

  ~BasicList() {
  }

  void reset() {
    m_list.clear();
  }

  inline T& var(int idx) {
    if (idx >= m_list.size()) {
      err = ERR_SOR;
      return bull;
    }
    return m_list.at(idx);
  }

  inline unsigned int size() {
    return m_list.size();
  }

  inline void append(T& item) {
    if (!m_list.push_back(item))
      err = ERR_OOM;
  }
  inline void prepend(T& item) {
    if (!m_list.push_front(item))
      err = ERR_OOM;
  }
  inline T front() {
    if (m_list.size() == 0) {
      err = ERR_EMPTY;
      return bull;
    } else {
      return m_list.front();
    }
  }
  inline T back() {
    if (m_list.size() == 0) {
      err = ERR_EMPTY;
      return bull;
    } else {
      return m_list.back();
    }
  }
  inline void pop_front() { m_list.pop_front(); }
  inline void pop_back() { m_list.pop_back(); }

private:
  QList<T> m_list;
  T bull;
};

template <typename T> class BasicListVariables {
public:
  BasicListVariables() {
    m_size = 0;
    m_var = NULL;
  }

  ~BasicListVariables() {
    reserve(0);
  }

  void reset() {
    for (index_t i = 0; i < m_size; ++i)
      m_var[i]->reset();
  }

  bool reserve(index_t count) {
    dbg_var("sl reserve %d\r\n", count);
    if (count == 0) {
      for (index_t i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }

    if (count < m_size) {
      for (index_t i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }

    m_var = varRealloc(m_var, count);
    if (!m_var) {
      // XXX: How are we going to delete the elements already there before
      // the failed realloc() call?
      err = ERR_OOM;
      m_size = 0;
      return true;
    }
    for (index_t i = m_size; i < count; ++i) {
      m_var[i] = new BasicList<T>();
      if (!m_var[i]) {
        for (index_t j = m_size; j < i; ++j)
          delete m_var[j];
        err = ERR_OOM;
        return true;
      }
    }
    m_size = count;
    return false;
  }

  inline unsigned int size() {
    return m_size;
  }

  inline BasicList<T>& var(index_t index) {
    return *m_var[index];
  }

private:
  unsigned int m_size;
  BasicList<T> **m_var;
};
