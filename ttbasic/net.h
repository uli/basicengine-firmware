void inet();
num_t nconnect();
BString swget();

#define E_NETWORK(code) do { \
  err = ERR_NETWORK; err_expected = code; \
  } while(0)
