#include "ipcap.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "common/thread_pool.hpp"

void InitThreadPool()
{
    // 获取线程池的实例
    opensource::ctrlfrmb::ThreadPool& pool = opensource::ctrlfrmb::ThreadPool::Instance();
    // 设置线程池参数
    pool.set(4, 2, 5); // 设置最大线程数为4，最小线程数为2，线程超时时间为600秒
}

int main() {
    InitThreadPool();

    using namespace figkey;
    PcapCom pcap;
    auto networkList = pcap.getNetworkList();

    std::cout << "Available network interfaces:" << std::endl;
    for (size_t i = 0; i < networkList.size(); ++i) {
        std::cout << i + 1 << ": " << networkList[i].name << " - " << networkList[i].description << std::endl;
    }

    std::cout << "Select an interface to monitor: ";
    int choice;
    std::cin >> choice;

    if (choice < 1 || choice > networkList.size()) {
        std::cerr << "Invalid selection." << std::endl;
        return 1;
    }

    if (!pcap.setNetwork(networkList[choice - 1].name)) {
        std::cerr << "Failed to set network." << std::endl;
        return 1;
    }

    std::cout << "Starting capture on " << networkList[choice - 1].name << std::endl;
    pcap.startCapture(true);
    while (true)
    {
        std::string s;
        std::getline(std::cin, s);
        if (s == "exit")
        {
            break;
        }
    }

    return 0;
}
