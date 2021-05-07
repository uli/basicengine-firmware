#include "ttconfig.h"
#include "basic.h"
#include "net.h"

#define ERR_OK lwip_ERR_OK
extern "C" {
#include <lwip/apps/tftp_client.h>
#include <lwip/apps/tftp_server.h>
}

#define LWIP_TFTP_EXAMPLE_BASE_DIR ""

static char full_filename[256];
bool tftp_done;

static void wait_for_result() {
  while (tftp_done != true) {
    process_events();

    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c))
      break;

    yield();
  }
}

static void *
tftp_open_file(const char* fname, u8_t is_write)
{
    snprintf(full_filename, sizeof(full_filename), "%s%s", LWIP_TFTP_EXAMPLE_BASE_DIR, fname);
    full_filename[sizeof(full_filename)-1] = 0;

    if (is_write) {
        return (void*)fopen(full_filename, "wb");
    } else {
        return (void*)fopen(full_filename, "rb");
    }
}

static void*
tftp_open(const char* fname, const char* mode, u8_t is_write)
{
    LWIP_UNUSED_ARG(mode);
    return tftp_open_file(fname, is_write);
}

static void
tftp_close(void* handle)
{
    fclose((FILE*)handle);
    tftp_done = true;
}

static int
tftp_read(void* handle, void* buf, int bytes)
{
    int ret = fread(buf, 1, bytes, (FILE*)handle);
    if (ret <= 0) {
        return -1;
    }
    return ret;
}

static int
tftp_write(void* handle, struct pbuf* p)
{
    while (p != NULL) {
        if (fwrite(p->payload, 1, p->len, (FILE*)handle) != (size_t)p->len) {
            return -1;
        }
        p = p->next;
    }

    return 0;
}

/* For TFTP client only */
static void
tftp_error(void* handle, int err, const char* msg, int size)
{
    char message[100];

    LWIP_UNUSED_ARG(handle);

    memset(message, 0, sizeof(message));
    MEMCPY(message, msg, LWIP_MIN(sizeof(message)-1, (size_t)size));

    E_NETWORK(BString(_("TFTP error ")) + BString(err) +
              BString(" (") + BString(message) + BString(')'));
}

static const struct tftp_context tftp = {
    tftp_open,
    tftp_close,
    tftp_read,
    tftp_write,
    tftp_error
};

void
tftp_example_init_server(void)
{
    tftp_init_server(&tftp);
}

void Basic::itftpget() {
    BString host = istrexp();

    if (*cip++ != I_COMMA)
        E_SYNTAX(I_COMMA);

    BString file = getParamFname();

    void *f;
    err_t err;
    ip_addr_t srv;

    int ret = ipaddr_aton(host.c_str(), &srv);
    if (ret != 1) {
        E_NETWORK(_("ipaddr_aton failed"));
        return;
    }

    err = tftp_init_client(&tftp);
    if (err != lwip_ERR_OK) {
        E_NETWORK(_("could not initialize TFTP client"));
        return;
    }

    f = tftp_open_file(file.c_str(), 1);
    if (f == NULL) {
        E_NETWORK(_("failed to create file"));
        return;
    }

    tftp_done = false;

    err = tftp_get(f, &srv, TFTP_PORT, file.c_str(), TFTP_MODE_OCTET);
    if (err != lwip_ERR_OK) {
        E_NETWORK("failed to get remote file");
        return;
    }

    wait_for_result();
    tftp_cleanup();
}

void Basic::itftpput() {
    BString host = istrexp();

    if (*cip++ != I_COMMA)
        E_SYNTAX(I_COMMA);

    BString file = getParamFname();

    void *f;
    err_t err;
    ip_addr_t srv;

    int ret = ipaddr_aton(host.c_str(), &srv);
    if (ret != 1) {
        E_NETWORK(_("ipaddr_aton failed"));
        return;
    }

    err = tftp_init_client(&tftp);
    if (err != lwip_ERR_OK) {
        E_NETWORK(_("could not initialize TFTP client"));
        return;
    }

    f = tftp_open_file(file.c_str(), 0);
    if (f == NULL) {
        E_NETWORK(_("failed to open file"));
        return;
    }

    tftp_done = false;

    err = tftp_put(f, &srv, TFTP_PORT, file.c_str(), TFTP_MODE_OCTET);
    if (err != lwip_ERR_OK) {
        E_NETWORK(_("failed to send file"));
        return;
    }

    wait_for_result();
    tftp_cleanup();
}

void Basic::itftpd() {
    if (*cip == I_ON) {
        tftp_init_server(&tftp);
        ++cip;
    } else if (*cip == I_OFF) {
        tftp_cleanup();
        ++cip;
    } else
        SYNTAX_T(_("expected ON or OFF"));
}
