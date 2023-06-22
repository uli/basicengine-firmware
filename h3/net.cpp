#ifndef JAILHOUSE

#include "ttconfig.h"
#include "basic.h"
#include "net.h"

#include <network.h>
#define ERR_OK lwip_ERR_OK
#include <lwip/apps/http_client.h>
#include <lwip/dhcp.h>

static void E_LWIP(err_t error) {
  BString lwip_err("lwip: ");
  lwip_err += lwip_strerr(error);
  E_NETWORK(lwip_err);
}

static void iconnect() {
  network_if_start();
  dhcp_start(&netif_eth0);
}

static void idisconnect() {
  network_if_stop();
}

struct _done {
  bool done;
  httpc_result_t result;
  uint32_t len;
  err_t err;
};

static void httpc_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
  auto res = (struct _done *)arg;
  res->done = true;
  res->result = httpc_result;
  res->len = rx_content_len;
  res->err = err;
}

static bool split_url(BString &url, BString &protocol, BString &hostname, BString &path) {
  int colon = url.indexOf("://");
  if (colon < 0) {
    E_NETWORK(_("invalid URL"));
    return false;
  }

  protocol = url.substring(0, colon);
  BString host_path = url.substring(colon + 3);

  int host_path_sep = host_path.indexOf('/');
  if (host_path_sep < 0) {
    hostname = host_path;
    path = "/";
  } else {
    hostname = host_path.substring(0, host_path_sep);
    path = host_path.substring(host_path_sep);
  }

  return true;
}

static void wait_for_result(httpc_state_t *state, struct _done *result) {
  while (result->done != true) {
    process_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      httpc_abort(state);
      break;
    }
    yield();
  }
}

void Basic::iwget() {
  BString url = istrexp();
  BString file;
  if (*cip == I_TO) {
    ++cip;
    file = getParamFname();
  } else {
    int sl = url.lastIndexOf('/');
    if (sl < 0)
      file = F("index.html");
    else
      file = url.substring(sl + 1);
  }
  if (err)
    return;

  if (!netif_is_link_up(&netif_eth0)) {
    E_NETWORK(_("not connected"));
    return;
  }

  httpc_connection_t conn_settings;
  memset(&conn_settings, 0, sizeof(conn_settings));
  httpc_state_t	*conn_state;

  conn_settings.result_fn = httpc_result;
  conn_settings.headers_done_fn = NULL;
  conn_settings.use_proxy = 0;

  struct _done result;
  result.done = false;

  BString protocol, dns_name, path;
  if (split_url(url, protocol, dns_name, path) == false)
    return;

  err_t error = httpc_get_file_dns_to_disk(dns_name.c_str(), 80, path.c_str(),
    &conn_settings, &result, file.c_str(), &conn_state);

  if (error != lwip_ERR_OK) {
    E_LWIP(error);
    return;
  }

  wait_for_result(conn_state, &result);

  retval[0] = result.len;
  retval[1] = result.err;
}

void Basic::inet() {
  switch (*cip++) {
  case I_CONNECT:
  case I_UP:
    iconnect();
    break;
  case I_DOWN:
    idisconnect();
    break;
  default:
    E_ERR(SYNTAX, _("expected network command"));
    break;
  }
}

BString Basic::snetinput() {
  BString rx;
  size_t count = getparam();
  return rx;
}

BString Basic::snetget() {
  BString rx;
  if (checkOpen())
    return rx;
  BString url = istrexp();
  if (err || checkClose())
    return rx;
  return rx;
}

num_t Basic::nconnect() {
  if (checkOpen() || checkClose())
    return 0;
  return netif_is_link_up(&netif_eth0);
}

#endif
