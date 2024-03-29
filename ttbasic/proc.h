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

#include <stdlib.h>

#define MAX_PROC_ARGS 32
#define MAX_PROC_LOCS 32

#define NO_PROC ((uint8_t)-1)

struct proc_t {
  icode_t *lp;
  icode_t *ip;
  unsigned char args_num[MAX_PROC_ARGS];
  unsigned char args_str[MAX_PROC_ARGS];
  unsigned char locs_num[MAX_PROC_ARGS];
  unsigned char locs_str[MAX_PROC_ARGS];
  unsigned char argc_num, argc_str;
  unsigned char locc_num, locc_str;
  uint32_t profile_current;
  uint32_t profile_total;
};

class Procedures {
public:
  Procedures() {
    m_size = 0;
    m_proc = NULL;
  }

  ~Procedures() {
    reserve(0);
  }

  void reset() {
    for (int i = 0; i < m_size; ++i) {
      memset(&m_proc[i], 0, sizeof(m_proc[i]));
    }
  }

  bool reserve(uint8_t count) {
    dbg_var("nv reserve %d\n", count);
    if (count == 0) {
      free(m_proc);
      m_proc = NULL;
      m_size = 0;
      return false;
    }
    m_proc = (struct proc_t *)realloc(m_proc, count * sizeof(*m_proc));
    if (!m_proc) {
      m_size = 0;
      return true;
    }
    for (int i = m_size; i < count; ++i) {
      memset(&m_proc[i], 0, sizeof(m_proc[i]));
    }
    m_size = count;
    return false;
  }

  inline int size() {
    return m_size;
  }

  inline struct proc_t &proc(uint8_t index) {
    return m_proc[index];
  }

  inline int getNumArg(uint8_t index, uint8_t arg) {
    if (index >= m_size)
      return -1;
    proc_t &pr = m_proc[index];
    for (int i = 0; i < pr.argc_num; ++i) {
      if (pr.args_num[i] == arg)
        return i;
    }
    return -1;
  }

  inline int getStrArg(uint8_t index, uint8_t arg) {
    if (index >= m_size)
      return -1;
    proc_t &pr = m_proc[index];
    for (int i = 0; i < pr.argc_str; ++i) {
      if (pr.args_str[i] == arg)
        return i;
    }
    return -1;
  }

  inline int getNumLoc(uint8_t index, uint8_t loc) {
    if (index >= m_size)
      return -1;
    proc_t &pr = m_proc[index];
    for (int i = 0; i < pr.locc_num; ++i) {
      if (pr.locs_num[i] == loc)
        return i;
    }
    if (pr.locc_num >= MAX_PROC_LOCS)
      return -1;
    pr.locs_num[pr.locc_num++] = loc;
    return pr.locc_num - 1;
  }

  inline int getStrLoc(uint8_t index, uint8_t loc) {
    if (index >= m_size)
      return -1;
    proc_t &pr = m_proc[index];
    for (int i = 0; i < pr.locc_str; ++i) {
      if (pr.locs_str[i] == loc)
        return i;
    }
    if (pr.locc_str >= MAX_PROC_LOCS)
      return -1;
    pr.locs_str[pr.locc_str++] = loc;
    return pr.locc_str - 1;
  }

private:
  int m_size;
  struct proc_t *m_proc;
};

struct label_t {
  icode_t *lp;
  icode_t *ip;
};

class Labels {
public:
  Labels() {
    m_size = 0;
    m_lab = NULL;
  }

  ~Labels() {
    reserve(0);
  }

  void reset() {
    for (int i = 0; i < m_size; ++i) {
      memset(&m_lab[i], 0, sizeof(m_lab[i]));
    }
  }

  bool reserve(uint8_t count) {
    dbg_var("lb reserve %d\n", count);
    if (count == 0) {
      free(m_lab);
      m_lab = NULL;
      m_size = 0;
      return false;
    }
    m_lab = (struct label_t *)realloc(m_lab, count * sizeof(*m_lab));
    if (!m_lab) {
      m_size = 0;
      return true;
    }
    for (int i = m_size; i < count; ++i) {
      memset(&m_lab[i], 0, sizeof(m_lab[i]));
    }
    m_size = count;
    return false;
  }

  inline int size() {
    return m_size;
  }

  inline struct label_t &label(uint8_t index) {
    return m_lab[index];
  }

private:
  int m_size;
  struct label_t *m_lab;
};
