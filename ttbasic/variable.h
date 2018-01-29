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
#define dbg_var(x...) do {Serial.printf(x);Serial.flush();} while(0)
#else
#define dbg_var(x...)
#endif

class VarNames {
public:
  VarNames() {
    m_var_top = m_prg_var_top = m_size = 0;
    m_var_name = NULL;
  }

  inline bool reserve(int count) {
    dbg_var("vnames reserve %d\n", count);
    if (count > m_size)
      return doReserve(count);
    else
      return false;
  }

  void deleteDownTo(uint8_t idx) {
    dbg_var("vnames del dto %d\n", idx);
    for (int i = idx; i < m_var_top; ++i) {
      if (m_var_name[i]) {
        free((void *)m_var_name[i]);
        m_var_name[i] = NULL;
      }
    }
    m_var_top = idx;
  }

  void deleteAll() {
    dbg_var("vnames del all\n");
    deleteDownTo(0);
    if (m_var_name) {
      free(m_var_name);
      m_var_name = NULL;
    }
    m_size = m_prg_var_top = m_var_top = 0;
  }

  void deleteDirect() {
    dbg_var("vnames del dir\n");
    deleteDownTo(m_prg_var_top);
    m_var_top = m_prg_var_top;
  }

  int find(const char *name)
  {
    dbg_var("vnames find %s\n", name);
    for (int i=0; i < m_var_top; ++i) {
      if (!strcasecmp(name, m_var_name[i])) {
#ifdef DEBUG_VAR
        Serial.printf("found %d\n", i);
#endif
        return i;
      }
    }
    return -1;
  }

  int assign(const char *name, bool is_prg_text)
  {
    dbg_var("vnames assign %s text %d\n", name, is_prg_text);
#ifdef DEBUG_VAR
    Serial.printf("assign %s ", name);
#endif
    int v = find(name);
    if (v >= 0)
      return v;

    if (reserve(m_var_top+1))
      return -1;

    m_var_name[m_var_top++] = strdup(name);
    if (is_prg_text)
      ++m_prg_var_top;
    dbg_var("got %d\n", m_var_top-1);
    return m_var_top-1;
  }

  inline const char *name(uint8_t idx) {
    return m_var_name[idx];
  }

  inline int varTop() {
    return m_var_top;
  }
  inline int prgVarTop() {
    return m_prg_var_top;
  }

protected:
  int m_var_top;
  int m_prg_var_top;
  int m_size;

private:
  bool doReserve(int count) {
    m_var_name = (char **)realloc(m_var_name, count * sizeof(char *));
    if (!m_var_name)
      return true;
    for (int i = m_size; i < count; ++i)
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

  void reset() {
    for (int i = 0; i < m_size; ++i)
      m_var[i] = 0;
  }

  bool reserve(uint8_t count) {
    dbg_var("nv reserve %d\n", count);
    if (count == 0) {
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    m_var = (num_t *)realloc(m_var, count * sizeof(num_t));
    if (!m_var)
      return true;
    for (int i = m_size; i < count; ++i)
      m_var[i] = 0;
    m_size = count;
    return false;
  }

  inline int size() {
    return m_size;
  }
  
  inline num_t& var(uint8_t index) {
    return m_var[index];
  }
  
private:
  int m_size;
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
    dbg_var("na reserve dims %d\n", dims);
    m_dims = dims;
    m_sizes = (int *)malloc(dims * sizeof(int));
    if (!m_sizes) {
      m_dims = 0;
      return true;
    }
    memcpy(m_sizes, sizes, dims * sizeof(int));
    m_total = 1;
    for (int i = 0; i < dims; ++i) {
      m_total *= sizes[i];
    }
    dbg_var("na total %d\n", m_total);
    m_var = (T *)malloc(m_total * sizeof(T));
    if (!m_var) {
      free(m_sizes);
      m_dims = 0;
      m_sizes = NULL;
      m_var = NULL;
      return true;
    }
    for (int i = 0; i < m_total; ++i) {
      m_var[i] = 0;
    }
    return false;
  }

  inline T& var(int *idxs) {
    if (!m_sizes) {
      err = ERR_UNDEFARR;
      // XXX: Is it possible to return an invalid reference without crashing?
      return bull;
    }
    int mul = 1;
    int idx = 0;
    for (int i = 0; i < m_dims; ++i) {
      if (idxs[i] >= m_sizes[i]) {
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

template <typename T> class NumArrayVariables {
public:
  NumArrayVariables() {
    m_size = 0;
    m_var = NULL;
  }
  
  void reset() {
    for (int i = 0; i < m_size; ++i)
      m_var[i]->reset();
  }
  
  bool reserve(uint8_t count) {
    dbg_var("na reserve %d\n", count);
    if (count == 0) {
      for (int i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    
    if (count < m_size) {
      for (int i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }
    
    m_var = (NumArray<T> **)realloc(m_var, count * sizeof(NumArray<T> **));
    if (!m_var)
      return true;
    for (int i = m_size; i < count; ++i) {
      m_var[i] = new NumArray<T>();
    }
    m_size = count;
    return false;
  }
  
  inline int size() {
    return m_size;
  }
  
  inline NumArray<T>& var(uint8_t index) {
    return *m_var[index];
  }

private:
  int m_size;
  NumArray<T> **m_var;
};

class StringVariables {
public:
  StringVariables() {
    m_size = 0;
    m_var = NULL;
  }
  
  void reset() {
    for (int i = 0; i < m_size; ++i)
      *m_var[i] = "";
  }

  bool reserve(uint8_t count) {
    dbg_var("sv reserve %d\n", count);
    if (count == 0) {
      for (int i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    if (count < m_size) {
      for (int i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }
    m_var = (BString **)realloc(m_var, count * sizeof(BString *));
    if (!m_var)
      return true;
    for (int i = m_size; i < count; ++i) {
      m_var[i] = new BString();
      *m_var[i] = "";
    }
    m_size = count;
    return false;
  }
  
  inline BString& var(uint8_t index) {
    return *m_var[index];
  }

private:
  int m_size;
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
    dbg_var("sa reserve dims %d\n", dims);
    m_dims = dims;
    m_sizes = (int *)malloc(dims * sizeof(int));
    if (!m_sizes) {
      m_dims = 0;
      return true;
    }
    memcpy(m_sizes, sizes, dims * sizeof(int));
    m_total = 1;
    for (int i = 0; i < dims; ++i) {
      m_total *= sizes[i];
    }
    dbg_var("na total %d\n", m_total);
    m_var = new BString[m_total];
    if (!m_var) {
      free(m_sizes);
      m_dims = 0;
      m_sizes = NULL;
      m_var = NULL;
      return true;
    }
    return false;
  }

  inline T& var(int *idxs) {
    if (!m_sizes) {
      err = ERR_UNDEFARR;
      // XXX: Is it possible to return an invalid reference without crashing?
      return bull;
    }
    int mul = 1;
    int idx = 0;
    for (int i = 0; i < m_dims; ++i) {
      if (idxs[i] >= m_sizes[i]) {
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
  
  void reset() {
    for (int i = 0; i < m_size; ++i)
      m_var[i]->reset();
  }
  
  bool reserve(uint8_t count) {
    dbg_var("sa reserve %d\n", count);
    if (count == 0) {
      for (int i = 0; i < m_size; ++i) {
        delete m_var[i];
      }
      free(m_var);
      m_var = NULL;
      m_size = 0;
      return false;
    }
    
    if (count < m_size) {
      for (int i = count; i < m_size; ++i) {
        delete m_var[i];
      }
    }
    
    m_var = (StringArray<T> **)realloc(m_var, count * sizeof(StringArray<T> **));
    if (!m_var)
      return true;
    for (int i = m_size; i < count; ++i) {
      m_var[i] = new StringArray<T>();
    }
    m_size = count;
    return false;
  }
  
  inline int size() {
    return m_size;
  }
  
  inline StringArray<T>& var(uint8_t index) {
    return *m_var[index];
  }

private:
  int m_size;
  StringArray<T> **m_var;
};

struct basic_location {
  unsigned char *lp;
  unsigned char *ip;
};

class Procedures {
public:
  Procedures() {
    m_size = 0;
    m_proc = NULL;
  }

  void reset() {
    for (int i = 0; i < m_size; ++i) {
      m_proc[i].lp = NULL;
      m_proc[i].ip = NULL;
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
    m_proc = (struct basic_location *)realloc(m_proc, count * sizeof(*m_proc));
    if (!m_proc)
      return true;
    for (int i = m_size; i < count; ++i) {
      m_proc[i].lp = NULL;
      m_proc[i].ip = NULL;
    }
    m_size = count;
    return false;
  }

  inline int size() {
    return m_size;
  }
  
  inline struct basic_location& proc(uint8_t index) {
    return m_proc[index];
  }
  
private:
  int m_size;
  struct basic_location *m_proc;
};
