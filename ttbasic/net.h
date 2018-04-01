void inet();
num_t nconnect();
BString snetget();
BString snetinput();

#define E_NETWORK(code) do { \
  err = ERR_NETWORK; err_expected = code; \
  } while(0)
