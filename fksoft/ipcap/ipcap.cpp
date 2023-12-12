// ipcap.cpp: 定义应用程序的入口点。
//
#include <WinSock2.h>
#include <functional>
#include "ipcap.h"
#include "common/thread_pool.hpp"
#include "common/logger.hpp"

#define ETHERNET_HEADER_LEN 14

struct ip_header {
    unsigned char  iph_ihl : 4, iph_ver : 4;
    unsigned char  iph_tos;
    unsigned short iph_len;
    unsigned short iph_ident;
    unsigned short iph_offset;
    unsigned char  iph_ttl;
    unsigned char  iph_protocol;
    unsigned short iph_chksum;
    unsigned int   iph_sourceip;
    unsigned int   iph_destip;
};

struct tcp_header {
    unsigned short th_sport;
    unsigned short th_dport;
    unsigned int   th_seq;
    unsigned int   th_ack;
    unsigned char  th_off : 4, th_x2 : 4;
    unsigned char  th_flags;
    unsigned short th_win;
    unsigned short th_sum;
    unsigned short th_urp;
};

struct udp_header {
    unsigned short uh_sport;
    unsigned short uh_dport;
    unsigned short uh_len;
    unsigned short uh_sum;
};
namespace figkey{

// 获取线程池的实例
opensource::ctrlfrmb::ThreadPool& pool = opensource::ctrlfrmb::ThreadPool::Instance();
// 获取日志的实例
opensource::ctrlfrmb::Logger& logger = opensource::ctrlfrmb::Logger::Instance();

static void InitLogger()
{
    using namespace opensource::ctrlfrmb;
    // 设置日志等级为 DEBUG，这样所有等级的日志都将显示
    logger.setLevel(LogLevel::DEBUG);

    // 设置自定义前缀回调
    logger.setPrefixCallback([]() {
        time_t now = time(nullptr);
        return ctime(&now);
        });

    FileLoggerConfig config("logs", "mylog", ".txt", 1024 * 1024, 5, true); // 启用异步写入
    logger.enableFileWrite(config);
}

void PcapCom::packetHandler(unsigned char* userData, const struct pcap_pkthdr* pkthdr, const unsigned char* packet) {
    struct ip_header* iph = (struct ip_header*)(packet + ETHERNET_HEADER_LEN);
    unsigned int IPHeaderLength = iph->iph_ihl * 4;

    char sourceIp[INET_ADDRSTRLEN];
    char destIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(iph->iph_sourceip), sourceIp, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iph->iph_destip), destIp, INET_ADDRSTRLEN);

    std::ostringstream ss_log;
    ss_log << "Source IP: " << sourceIp << ", Destination IP: " << destIp;

    switch (iph->iph_protocol) {
    case IPPROTO_TCP: {
        struct tcp_header* tcph = (struct tcp_header*)(packet + ETHERNET_HEADER_LEN + IPHeaderLength);
        ss_log << "###TCP Packet: Src Port: " << ntohs(tcph->th_sport) << ", Dst Port: " << ntohs(tcph->th_dport);
        break;
    }
    case IPPROTO_UDP: {
        struct udp_header* udph = (struct udp_header*)(packet + ETHERNET_HEADER_LEN + IPHeaderLength);
        ss_log << "###UDP Packet: Src Port: " << ntohs(udph->uh_sport) << ", Dst Port: " << ntohs(udph->uh_dport);
        break;
    }
    case IPPROTO_ICMP:
        ss_log << "###ICMP Packet";
        break;
    default:
        ss_log << "###Other Protocol";
        break;
    }
    logger.info(ss_log.str());
}

std::vector<network_info> PcapCom::getNetworkList() {
    pcap_if_t* alldevs;
    pcap_if_t* device;
    char errbuf[PCAP_ERRBUF_SIZE];
    std::vector<network_info> devices;

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs: " << errbuf << std::endl;
        return devices;
    }

    for (device = alldevs; device != NULL; device = device->next) {
        network_info info;
        info.name = device->name;
        info.description = device->description ? device->description : "No description available";
        devices.push_back(info);
    }

    pcap_freealldevs(alldevs);
    return devices;
}

bool PcapCom::setNetwork(const std::string& network_name) {
    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live(network_name.c_str(), 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        std::cerr << "Couldn't open device " << network_name << ": " << errbuf << std::endl;
        return false;
    }
    return true;

#if 0
    // 设置过滤器（例如只捕获TCP数据包）
    struct bpf_program filter;
    char filter_exp[] = "tcp";
    if (pcap_compile(handle, &filter, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Bad filter - " << pcap_geterr(handle) << std::endl;
        pcap_freealldevs(alldevs);
        pcap_close(handle);
        return -1;
    }
    if (pcap_setfilter(handle, &filter) == -1) {
        std::cerr << "Error setting filter - " << pcap_geterr(handle) << std::endl;
        pcap_freealldevs(alldevs);
        pcap_close(handle);
        return -1;
    }
#endif 
}

void PcapCom::asynStartCapture()
{
    pcap_loop(handle, 0, packetHandler, NULL);
}

void PcapCom::startCapture(bool use_thread_pool) {
    if (handle) {
        InitLogger();

        if (use_thread_pool)
        {
            //using std::placeholders::_1;
            std::function<void()> fun_asyn = std::bind(&PcapCom::asynStartCapture, this);
            pool.submit(fun_asyn);
        }
        else
            pcap_loop(handle, 0, packetHandler, NULL);
    }
}

}