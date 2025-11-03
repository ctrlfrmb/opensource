/**
 * @file    nwswitchscriptgenerator.cpp
 * @brief   网络交换机脚本生成器
 * @author  leiwei
 * @date    2025.04.17
 */

#include "nwswitch/nwswitchscriptgenerator.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QRandomGenerator>

// 定义脚本中使用的文件路径常量
const QString NWSwitchScriptGenerator::NETPLAN_CONFIG_FILE = "/etc/netplan/01-netcfg.yaml";
const QString NWSwitchScriptGenerator::NETPLAN_TEMP_CONFIG_FILE = "/tmp/01-netcfg.yaml";
const QString NWSwitchScriptGenerator::NETWORK_RECOVERY_SCRIPT = "/usr/local/bin/network-recovery.sh";
const QString NWSwitchScriptGenerator::NETWORK_RECOVERY_SERVICE = "/etc/systemd/system/network-recovery.service";

NWSwitchScriptGenerator::NWSwitchScriptGenerator()
{
}

QString NWSwitchScriptGenerator::generateNetworkScript(const NWSwitchInfo &switchInfo, bool enableIRQBinding, bool enableAdvanceFeature)
{
    // 每次生成新脚本时，清空旧的MAC地址缓存
    generatedVethDutMacs_.clear();

    QString script;

    // 脚本头部信息和通用函数
    script += "#!/bin/bash\n";
    script += "#\n";
    script += "# Dynamically generated network configuration script.\n";
    script += "# Function: Creates network namespaces, configures interfaces, IP addresses, and routing.\n";
    script += "# Author: leiwei\n";
    script += "#\n\n";
    script += "set -e\n\n"; // 如果任何命令失败，立即退出脚本

    script += "log() {\n";
    script += "    echo \"[$(date +\"%H:%M:%S\")] $1\"\n";
    script += "}\n\n";

    // generateInitNamespacesScriptContent 将会填充 generatedVethDutMacs_ 缓存
    script += generateInitNamespacesScriptContent(switchInfo);
    script += generateARPCleanupScriptContent(switchInfo);

    if (enableAdvanceFeature) {
        script += generateAdvancedLinkConfigScriptContent(switchInfo);
    } else {
        script += "# Advanced link features (MAC spoofing, static ARP) are disabled.\n\n";
    }

    script += generateIPConfigScriptContent(switchInfo);
    script += generateRouteConfigScriptContent(switchInfo);

    if (enableIRQBinding) {
        script += generateIRQBindingScriptContent();
    } else {
        script += "# Network optimization (IRQ binding) is disabled in settings.\n\n";
    }

    script += "log \"Script execution completed successfully.\"\n";

    return script;
}

QMap<QString, QString> NWSwitchScriptGenerator::getGeneratedVethDutMacs() const
{
    return generatedVethDutMacs_;
}

QString NWSwitchScriptGenerator::generateInitNamespacesScriptContent(const NWSwitchInfo &switchInfo)
{
    QString script;
    script += "# ============= 1. Namespace Initialization =============\n\n";
    script += "log \"Starting DUT initialization and cleanup...\"\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        const int dutIndex = i + 1;
        const QString &nsName = switchInfo.duts[i].name;
        const QString &ethName = switchInfo.duts[i].ethName;
        script += QString("\n# Clean up configuration for DUT %1\n").arg(dutIndex);
        script += QString("if ip netns list | grep -q \"%1\"; then\n").arg(nsName);
        script += QString("    if ip netns exec %1 ip link show %2 &>/dev/null; then\n").arg(nsName).arg(ethName);
        script += QString("        ip netns exec %1 ip link set %2 netns 1 2>/dev/null || true\n").arg(nsName).arg(ethName);
        script += "    fi\n";
        script += QString("    ip netns del %1 2>/dev/null || true\n").arg(nsName);
        script += "fi\n";
        script += QString("if ip link show veth-host%1 &>/dev/null; then\n").arg(dutIndex);
        script += QString("    ip link del veth-host%1 &>/dev/null || true\n").arg(dutIndex);
        script += "fi\n";
    }
    script += "\n# Ensure bridge br0 exists and is up\n";
    script += "if ! ip link show br0 &>/dev/null; then\n";
    script += "    ip link add name br0 type bridge\n";
    script += "fi\n";
    script += "ip link set dev br0 up\n\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        const int dutIndex = i + 1;
        const auto& dut = switchInfo.duts[i];

        QString vethMac = generateVirtualMacAddress(dut.vethName, false);
        QString vethHostMac = generateVirtualMacAddress(dut.vethName, true);
        generatedVethDutMacs_[dut.vethName] = vethMac;

        script += QString("\n# Create veth pair for DUT %1\n").arg(dutIndex);
        script += QString("ip link add %1 type veth peer name veth-host%2\n").arg(dut.vethName).arg(dutIndex);
        script += QString("ip link set %1 address %2\n").arg(dut.vethName).arg(vethMac);
        script += QString("ip link set veth-host%1 address %2\n").arg(dutIndex).arg(vethHostMac);
        script += QString("ip link set veth-host%1 master br0\n").arg(dutIndex);
        script += QString("ip link set veth-host%1 up\n").arg(dutIndex);
    }
    script += "\n# Create network namespaces and move interfaces\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        const int dutIndex = i + 1;
        const auto& dut = switchInfo.duts[i];
        script += QString("\n# Configure DUT %1 (Namespace: %2)\n").arg(dutIndex).arg(dut.name);
        script += QString("ip netns add %1\n").arg(dut.name);
        script += QString("ip link set %1 netns %2\n").arg(dut.vethName).arg(dut.name);
        script += QString("if ip link show %1 &>/dev/null; then\n").arg(dut.ethName);
        script += QString("    ip link set %1 netns %2 || true\n").arg(dut.ethName).arg(dut.name);
        script += "else\n";
        script += QString("    log \"WARNING: Physical interface %1 not found, skipping move operation\"\n").arg(dut.ethName);
        script += "fi\n";
    }
    script += "\nlog \"DUT initialization completed.\"\n";
    script += "log \"Current namespace list:\"\n";
    script += "ip netns list\n\n";
    return script;
}

QString NWSwitchScriptGenerator::generateAdvancedLinkConfigScriptContent(const NWSwitchInfo &switchInfo)
{
    QString script;
    script += "# ============= 3. Advanced Link-Layer Configuration =============\n\n";
    bool hasAdvancedConfig = false;
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; ++i) {
        const auto& dutInfo = switchInfo.duts[i];
        const QString &nsName = dutInfo.name;
        const QString &ethName = dutInfo.ethName;
        if (dutInfo.ethMAC.isEmpty() && dutInfo.staticArp.isEmpty()) {
            continue;
        }
        if (!hasAdvancedConfig) {
            script += "log \"Starting advanced link-layer configuration...\"\n";
            hasAdvancedConfig = true;
        }
        script += QString("\n# --- Advanced config for DUT %1 (Namespace: %2, Interface: %3) ---\n").arg(i + 1).arg(nsName).arg(ethName);
        if (!dutInfo.ethMAC.isEmpty() && isValidMacAddress(dutInfo.ethMAC)) {
            script += QString("ip netns exec %1 ip link set dev %2 down\n").arg(nsName).arg(ethName);
            script += QString("ip netns exec %1 ip link set dev %2 address %3\n").arg(nsName).arg(ethName).arg(dutInfo.ethMAC);
        }
        if (!dutInfo.staticArp.isEmpty()) {
            for (auto it = dutInfo.staticArp.constBegin(); it != dutInfo.staticArp.constEnd(); ++it) {
                const QString& ip = it.key();
                const QString& mac = it.value();
                if (!ip.isEmpty() && isValidMacAddress(mac)) {
                    script += QString("ip netns exec %1 ip neigh add %2 lladdr %3 dev %4 nud permanent\n").arg(nsName).arg(ip).arg(mac).arg(ethName);
                }
            }
        }
    }
    if (hasAdvancedConfig) {
        script += "\nlog \"Advanced link-layer configuration completed.\"\n\n";
    }
    return script;
}

QString NWSwitchScriptGenerator::generateARPCleanupScriptContent(const NWSwitchInfo &switchInfo)
{
    QString script;
    script += "# ============= 2. ARP Cache Cleanup =============\n\n";
    script += "log \"Starting ARP cache cleanup...\"\n";
    script += "ip neigh flush all\n";
    script += "sync && sleep 0.5\n\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        const QString &nsName = switchInfo.duts[i].name;
        script += QString("ip netns exec %1 ip neigh flush all\n").arg(nsName);
        script += QString("ip netns exec %1 sync && sleep 0.2\n").arg(nsName);
    }
    script += "\nlog \"ARP cache cleanup completed.\"\n\n";
    return script;
}

QString NWSwitchScriptGenerator::generateIPConfigScriptContent(const NWSwitchInfo &switchInfo)
{
    QString script;
    script += "# ============= 4. IP Address Configuration =============\n\n";
    script += "log \"Starting IP address configuration...\"\n";
    script += "if ! lsmod | grep -q 8021q; then\n";
    script += "    modprobe 8021q || log \"WARNING: Failed to load 802.1q module, VLANs may not work.\"\n";
    script += "fi\n\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        script += generateDUTIPConfigScriptContent(switchInfo.duts[i], i + 1);
    }
    script += "log \"IP address configuration completed.\"\n\n";
    return script;
}

QString NWSwitchScriptGenerator::generateDUTIPConfigScriptContent(const NWSwitchDUTInfo &dutInfo, int dutIndex)
{
    QString script;
    const QString &nsName = dutInfo.name;
    const QString &vethName = dutInfo.vethName;
    const QString &ethName = dutInfo.ethName;
    script += QString("\n# --- Configuring IPs for DUT %1 (Namespace: %2) ---\n").arg(dutIndex).arg(nsName);
    script += QString("ip netns exec %1 ip link set %2 up\n").arg(nsName).arg(vethName);
    script += generateIpConfigForInterface(nsName, vethName, dutInfo.veth);
    script += QString("if ip netns exec %1 ip link show %2 &>/dev/null; then\n").arg(nsName).arg(ethName);
    script += QString("    ip netns exec %1 ip link set %2 up\n").arg(nsName).arg(ethName);
    script += generateIpConfigForInterface(nsName, ethName, dutInfo.eth);
    script += "else\n";
    script += QString("    log \"WARNING: Physical interface %1 not found in namespace %2, skipping its IP configuration.\"\n").arg(ethName).arg(nsName);
    script += "fi\n";
    return script;
}

QString NWSwitchScriptGenerator::generateIpConfigForInterface(const QString &nsName, const QString &baseInterfaceName, const QMap<QString, NWSwitchEthInfo> &ipConfigs)
{
    QString script;
    if (ipConfigs.isEmpty()) {
        return script;
    }
    QMap<uint16_t, QList<QPair<QString, NWSwitchEthInfo>>> vlanGroups;
    for (auto it = ipConfigs.constBegin(); it != ipConfigs.constEnd(); ++it) {
        vlanGroups[it.value().vlanID].append({it.key(), it.value()});
    }
    for (auto it = vlanGroups.constBegin(); it != vlanGroups.constEnd(); ++it) {
        const uint16_t vlanID = it.key();
        const auto& ipListWithInfo = it.value();
        QString interfaceToUse = baseInterfaceName;
        if (vlanID > 0) {
            interfaceToUse = QString("%1.%2").arg(baseInterfaceName).arg(vlanID);
            script += QString("\n# Configure VLAN %1 on %2 (interface: %3)\n").arg(vlanID).arg(baseInterfaceName).arg(interfaceToUse);
            script += QString("ip netns exec %1 ip link del %2 2>/dev/null || true\n").arg(nsName).arg(interfaceToUse);
            script += QString("ip netns exec %1 ip link add link %2 name %3 type vlan id %4\n").arg(nsName).arg(baseInterfaceName).arg(interfaceToUse).arg(vlanID);
            script += QString("ip netns exec %1 ip link set %2 up\n").arg(nsName).arg(interfaceToUse);
        }
        for (const auto& ipPair : ipListWithInfo) {
            const QString& ipAddress = ipPair.first;
            const uint8_t subnetSize = ipPair.second.subnetSize;
            script += QString("ip netns exec %1 ip addr add %2/%3 dev %4\n").arg(nsName).arg(ipAddress).arg(subnetSize).arg(interfaceToUse);
        }
    }
    return script + "\n";
}

QString NWSwitchScriptGenerator::generateRouteConfigScriptContent(const NWSwitchInfo &switchInfo)
{
    QString script;
    script += "# ============= 5. Routing and NAT Configuration =============\n\n";
    script += "log \"Starting routing and NAT configuration...\"\n";
    for (int i = 0; i < NETWORK_SWITCH_DUT_NUM; i++) {
        script += generateDUTRouteConfigScriptContent(switchInfo.duts[i], i + 1);
    }
    script += "log \"Routing and NAT configuration completed.\"\n\n";
    return script;
}

QString NWSwitchScriptGenerator::generateDUTRouteConfigScriptContent(const NWSwitchDUTInfo &dutInfo, int dutIndex)
{
    QString script;
    const QString &nsName = dutInfo.name;
    script += QString("\n# --- Configuring routes for DUT %1 (Namespace: %2) ---\n").arg(dutIndex).arg(nsName);
    if (!dutInfo.isEnableRoute || dutInfo.route.isEmpty()) {
        script += QString("log \"Routing for DUT %1 is disabled or has no rules, skipping.\"\n").arg(dutIndex);
        return script;
    }
    script += "# Enabling IP forwarding and ARP proxy\n";
    script += QString("ip netns exec %1 sysctl -w net.ipv4.ip_forward=1\n").arg(nsName);
    script += QString("ip netns exec %1 sysctl -w net.ipv4.conf.all.proxy_arp=1\n\n").arg(nsName);
    script += "# Configuring nftables for NAT and Mangle\n";
    script += QString("ip netns exec %1 nft flush ruleset 2>/dev/null || true\n").arg(nsName);
    script += QString("ip netns exec %1 nft add table ip nat\n").arg(nsName);
    script += QString("ip netns exec %1 nft 'add chain ip nat prerouting { type nat hook prerouting priority -100; }'\n").arg(nsName);
    script += QString("ip netns exec %1 nft 'add chain ip nat postrouting { type nat hook postrouting priority 100; }'\n").arg(nsName);
    script += QString("ip netns exec %1 nft add table ip mangle\n").arg(nsName);
    script += QString("ip netns exec %1 nft 'add chain ip mangle prerouting { type filter hook prerouting priority -150; }'\n\n").arg(nsName);
    for (int i = 0; i < dutInfo.route.size(); ++i) {
        const auto& routeInfo = dutInfo.route[i];
        script += QString("# Add route rule %1\n").arg(i + 1);
        QString vethInterfaceName = dutInfo.vethName;
        if (auto it = dutInfo.veth.find(routeInfo.vethIP); it != dutInfo.veth.end() && it.value().vlanID > 0) {
            vethInterfaceName = QString("%1.%2").arg(dutInfo.vethName).arg(it.value().vlanID);
        }
        QString ethInterfaceName = dutInfo.ethName;
        if (auto it = dutInfo.eth.find(routeInfo.ethIP); it != dutInfo.eth.end() && it.value().vlanID > 0) {
            ethInterfaceName = QString("%1.%2").arg(dutInfo.ethName).arg(it.value().vlanID);
        }
        script += "# Set fixed TTL=64 for traffic from physical interface\n";
        script += QString("ip netns exec %1 nft add rule ip mangle prerouting iifname %2 ip ttl set %3\n\n").arg(nsName).arg(ethInterfaceName).arg(NETWORK_SWITCH_TTL_NUM);
        script += "# DNAT rules\n";
        script += QString("ip netns exec %1 nft add rule ip nat prerouting ip daddr %2 dnat to %3\n").arg(nsName).arg(routeInfo.ethIP).arg(routeInfo.pcIP);
        script += QString("ip netns exec %1 nft add rule ip nat prerouting ip daddr %2 dnat to %3\n\n").arg(nsName).arg(routeInfo.vethIP).arg(routeInfo.productIP);
        script += "# SNAT rules\n";
        // 当数据从交换机输出时，将严格的SNAT规则变得更通用。去掉对目标地址routeInfo.pcIP的检查
        // 当一个数据包准备从 veth-dut 接口离开时，只要 它的源IP是 routeInfo.productIP，就将它的源IP伪装成 routeInfo.vethIP
        // 这样就解决了主机网卡多个 IP 时，可能出现问题的情况
		// TODO
        script += QString("ip netns exec %1 nft add rule ip nat postrouting oifname %2 ip saddr %3 snat to %4\n")
            .arg(nsName)
            .arg(vethInterfaceName)
            .arg(routeInfo.productIP)
            .arg(routeInfo.vethIP);
        // script += QString("ip netns exec %1 nft add rule ip nat postrouting oifname %2 ip saddr %3 ip daddr %4 snat to %5\n").arg(nsName).arg(vethInterfaceName).arg(routeInfo.productIP).arg(routeInfo.pcIP).arg(routeInfo.vethIP);
        script += QString("ip netns exec %1 nft add rule ip nat postrouting oifname %2 ip saddr %3 ip daddr %4 snat to %5\n\n").arg(nsName).arg(ethInterfaceName).arg(routeInfo.pcIP).arg(routeInfo.productIP).arg(routeInfo.ethIP);
    }
    script += QString("log \"Final NAT ruleset for namespace %1:\"\n").arg(nsName);
    script += QString("ip netns exec %1 nft list ruleset\n").arg(nsName);
    return script;
}

QString NWSwitchScriptGenerator::generateIRQBindingScriptContent()
{
    QString script;
    script += "# ============= 6. Network Optimization (IRQ Binding) =============\n\n";
    script += "log \"Starting network interface IRQ CPU binding...\"\n";
    script += "if [ ! -f \"/proc/interrupts\" ]; then\n";
    script += "    log \"ERROR: /proc/interrupts not found, skipping IRQ binding.\"\n";
    script += "    return 1\n";
    script += "fi\n\n";
    script += "for i in {1..6}; do\n";
    script += "    irqs=$(grep -E \"eth${i}\" /proc/interrupts | awk '{print $1}' | tr -d :)\n";
    script += "    if [ -z \"$irqs\" ]; then\n";
    script += "        log \"WARNING: No IRQs found for eth${i}, skipping.\"\n";
    script += "        continue\n";
    script += "    fi\n";
    script += "    for irq in $irqs; do\n";
    script += "        if [ -d \"/proc/irq/$irq\" ]; then\n";
    script += "            echo $i > /proc/irq/$irq/smp_affinity_list 2>/dev/null && log \"SUCCESS: IRQ $irq (eth${i}) bound to CPU${i}\" || log \"ERROR: Failed to bind IRQ $irq.\"\n";
    script += "        fi\n";
    script += "    done\n";
    script += "done\n\n";
    script += "log \"IRQ binding completed.\"\n\n";
    return script;
}

// ... 其他 generate... 函数保持不变 ...
QString NWSwitchScriptGenerator::generateRCLocalScript(const QString &configFilePath)
{
    QString rcLocalContent = "#!/bin/sh\n\n";
    rcLocalContent += "# Autostart script for network configuration.\n";
    rcLocalContent += QString("if [ -f %1 ]; then\n").arg(configFilePath);
    rcLocalContent += QString("    echo \"Executing network configuration script...\"\n");
    rcLocalContent += QString("    %1\n").arg(configFilePath);
    rcLocalContent += "    echo \"Network configuration loaded successfully.\"\n";
    rcLocalContent += "else\n";
    rcLocalContent += QString("    echo \"Network configuration script not found: %1\"\n").arg(configFilePath);
    rcLocalContent += "fi\n\n";
    rcLocalContent += "exit 0\n";
    return rcLocalContent;
}

QString NWSwitchScriptGenerator::generateSystemdServiceUnit()
{
    QString serviceContent = "[Unit]\n";
    serviceContent += "Description=RC Local startup script\n";
    serviceContent += "After=network.target\n\n";
    serviceContent += "[Service]\n";
    serviceContent += "Type=oneshot\n";
    serviceContent += "ExecStart=/etc/rc.local\n";
    serviceContent += "TimeoutSec=0\n";
    serviceContent += "StandardOutput=journal+console\n";
    serviceContent += "RemainAfterExit=yes\n\n";
    serviceContent += "[Install]\n";
    serviceContent += "WantedBy=multi-user.target\n";
    return serviceContent;
}

QString NWSwitchScriptGenerator::generateBridgeMacAddress(const QString& ipAddress)
{
    // 计算 IP 地址的哈希值
    QByteArray ipData = ipAddress.toUtf8();
    QByteArray hash = QCryptographicHash::hash(ipData, QCryptographicHash::Md5);

    // 构造 MAC 地址，前两字节固定为 36:B0，后四字节取哈希值
    QString mac = QString("36:b0:%1:%2:%3:%4")
        .arg(hash.at(0) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(1) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(2) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(3) & 0xFF, 2, 16, QChar('0'));

    return mac.toLower();
}

QString NWSwitchScriptGenerator::generateVirtualMacAddress(const QString& ethName, bool isHost)
{
    // 内部生成当前时间戳，精确到毫秒
    QString timeStr = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz"); // zzz 为毫秒
    QString combined = timeStr + ethName;
    QByteArray data = combined.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);

    // 根据 isHost 参数选择前缀
    QString prefix = isHost ? "ee:62" : "22:48";

    // 构造 MAC 地址
    QString mac = QString("%1:%2:%3:%4:%5")
        .arg(prefix)
        .arg(hash.at(0) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(1) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(2) & 0xFF, 2, 16, QChar('0'))
        .arg(hash.at(3) & 0xFF, 2, 16, QChar('0'));

    return mac.toLower();
}

bool NWSwitchScriptGenerator::isValidMacAddress(const QString& mac)
{
    QRegularExpression macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$", QRegularExpression::CaseInsensitiveOption);
    return macRegex.match(mac).hasMatch();
}

QString NWSwitchScriptGenerator::generateNetplanConfig(const QString& ipAddress)
{
    QString macAddress = generateBridgeMacAddress(ipAddress);
    QString config = R"(# Network config generated by NWSwitchTool
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      dhcp6: no
      optional: true
  bridges:
    br0:
      interfaces:
        - eth0
      addresses:
        - %1/24
      macaddress: %2
      parameters:
        stp: true
        forward-delay: 4
)";
    return config.arg(ipAddress).arg(macAddress);
}

QString NWSwitchScriptGenerator::generateNetworkConfigScript(const QString& ipAddress)
{
    // 生成配置文件内容
    QString configContent = generateNetplanConfig(ipAddress);

    // 创建应用脚本
    QString scriptContent = QString(
        "#!/bin/bash\n"
        "# Network configuration script\n"
        "set -e\n\n"
        "# Create temporary config file\n"
        "cat > %1 << 'EOF'\n"
        "%2\n"
        "EOF\n\n"
        "# Verify config file is not empty and contains required network settings\n"
        "if [ ! -s %1 ] || ! grep -q 'addresses:' %1 || ! grep -q 'br0:' %1; then\n"
        "  echo 'ERROR: Invalid network configuration file!'\n"
        "  exit 1\n"
        "fi\n\n"
        "# Backup original config file\n"
        "if [ -f %3 ]; then\n"
        "  cp -f %3 %3.bak\n"
        "fi\n\n"
        "# Apply new network config file\n"
        "cp -f %1 %3\n"
        "chmod 600 %3\n\n"
        "# Verify the copied file is valid before applying\n"
        "if [ ! -s %3 ] || ! grep -q 'addresses:' %3; then\n"
        "  echo 'ERROR: Config file verification failed after copy!'\n"
        "  if [ -f %3.bak ]; then\n"
        "    cp -f %3.bak %3\n"
        "  fi\n"
        "  exit 1\n"
        "fi\n\n"
        "echo '%4 network configuration'\n"
        "# Wait a while for the client to receive the message\n"
        "sleep 1\n"
        "netplan apply\n"
        "sleep 2\n"
        "exit 0\n"
    ).arg(NETPLAN_TEMP_CONFIG_FILE).arg(configContent).arg(NETPLAN_CONFIG_FILE).arg(NETWORK_CONIFG_SUCCESS_FLAG);

    return scriptContent;
}
