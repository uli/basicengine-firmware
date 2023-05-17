// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Ulrich Hecht

#ifdef __linux__

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#include "basic.h"
#include "eb_sys.h"

EBAPI void eb_set_cpu_speed(int percent)
{
    std::string policy = "/sys/devices/system/cpu/cpufreq/policy0/";
    std::string set_min_freq = policy + "scaling_min_freq";
    std::string set_max_freq = policy + "scaling_max_freq";
    std::string available = policy + "scaling_available_frequencies";

    std::fstream f(available, std::fstream::in);
    if (f.fail()) {
        // no frequency scaling, forget about it
        return;
    }

    std::vector<int> available_freqs;
    std::string s;
    while (getline(f, s, ' '))
        available_freqs.push_back(atoi(s.c_str()));

    f.close();

    int max_freq = *max_element(std::begin(available_freqs), std::end(available_freqs));
    int wanted_freq = max_freq * percent / 100;
    int real_freq = max_freq;

    for (auto i : available_freqs) {
        if (i >= wanted_freq && i < real_freq) {
            real_freq = i;
        }
    }

    std::ofstream ofile;
    ofile.open(set_min_freq);
    ofile << real_freq << std::endl;
    ofile.close();

    ofile.open(set_max_freq);
    ofile << real_freq << std::endl;
    ofile.close();
}

#endif	// __linux__
