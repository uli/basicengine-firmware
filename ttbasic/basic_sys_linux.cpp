// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Ulrich Hecht

#if defined(__linux__) || defined(JAILHOUSE)

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <libgen.h>

#include "basic.h"

void Basic::idtbload()
{
    BString filename = getParamFname();

    std::ifstream src(filename.c_str(), std::ios::binary);
    if (src.fail()) {
        err = ERR_FILE_OPEN;
        return;
    }

    char basenm[filename.length() + 1];
    strcpy(basenm, filename.c_str());
    std::string ovl_dir = std::string("/sys/kernel/config/device-tree/overlays/") +
                          basename(basenm);

    if (mkdir(ovl_dir.c_str(), 0755) < 0) {
        err = ERR_IO;
        err_expected = "failed to create overlay";
        return;
    }

    std::ofstream dst(ovl_dir + "/dtbo", std::ios::binary);
    if (dst.fail()) {
        err = ERR_IO;
        err_expected = "failed to write overlay";
        return;
    }

    dst << src.rdbuf();

    dst.close();
    src.close();

    std::ofstream status(ovl_dir + "/status", std::ios_base::binary);
    if (status.fail()) {
        err = ERR_IO;
        err_expected = "failed to access overlay status";
        return;
    }

    status << "1\n";
    status.close();

    std::ifstream status_rb(ovl_dir + "/status", std::ios_base::binary);
    if (status_rb.fail()) {
        err = ERR_IO;
        err_expected = "failed to read back overlay status";
        return;
    }

    std::string readback;
    status_rb >> readback;

    if (readback != "1") {
        err = ERR_IO;
        err_expected = "failed to activate overlay";
    }
}

#endif // __linux__
