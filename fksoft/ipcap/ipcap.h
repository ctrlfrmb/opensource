/**
 * @file    ipcap.hpp
 * @ingroup figkey
 * @brief   
 * @author  leiwei
 * @date    2023.12.12
 * Copyright (c) figkey 2023-2033
 */

#pragma once

#ifndef FIGKEY_PCAP_COM_HPP
#define FIGKEY_PCAP_COM_HPP

#include <pcap.h>
#include <iostream>
#include <vector>
#include <string>

namespace figkey {

// Structure for storing network interface information
struct network_info {
    std::string name;
    std::string description;
};

class PcapCom {
public:
    PcapCom() : handle(nullptr) {
        // Initialize any required fields
    }

    ~PcapCom() {
        if (handle) {
            pcap_close(handle);
        }
    }

    std::vector<network_info> getNetworkList();

    bool setNetwork(const std::string& network_name);

    void startCapture(bool use_thread_pool=false);

private:
    pcap_t* handle;

    void asynStartCapture();

    static void packetHandler(unsigned char* userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet);
};

}  // namespace figkey

#endif // !FIGKEY_PCAP_COM_HPP
