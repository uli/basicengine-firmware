#include <Arduino.h>
#include <stdlib.h>

#define FLOAT_NUMS
#ifdef FLOAT_NUMS
typedef double num_t;
#else
typedef int32_t num_t;
#endif

class BString : public String {
public:
  using String::operator=;

  int fromBasic(unsigned char *s) {
    len = *s++;
    if (!reserve(len)) {
      invalidate();
      return 0;
    }
    os_memcpy(buffer, s, len);
    buffer[len] = 0;
    return len + 1;
  }
};

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

  int find(char *name)
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

  uint8_t assign(char *name, bool is_prg_text)
  {
    dbg_var("vnames assign %s text %d\n", name, is_prg_text);
#ifdef DEBUG_VAR
    Serial.printf("assign %s ", name);
#endif
    int v = find(name);
    if (v >= 0)
      return v;

    if (reserve(m_var_top+1))
      return 0xff;

    m_var_name[m_var_top++] = strdup(name);
    if (is_prg_text)
      ++m_prg_var_top;
#ifdef DEBUG_VAR
    Serial.printf("got %d\n", m_var_top-1);
#endif
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

  int size() {
    return m_size;
  }
  
  inline num_t& var(uint8_t index) {
    return m_var[index];
  }
  
private:
  int m_size;
  num_t *m_var;
};

class StringVariables {
public:
  StringVariables() {
    m_size = 0;
    m_var = NULL;
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

