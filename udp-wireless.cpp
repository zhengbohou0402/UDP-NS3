// udp-wireless.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    // 创建5个移动节点
    NodeContainer nodes;
    nodes.Create(5);

    // 无线物理层配置
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // MAC层配置
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    // 安装无线设备
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // 移动模型（随机游走）
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue("100.0"),
                                 "Y", StringValue("100.0"),
                                 "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(0, 200, 0, 200)));
    mobility.Install(nodes);

    // 协议栈安装
    InternetStackHelper stack;
    stack.Install(nodes);

    // IP地址分配
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(devices);

    // UDP服务器（节点0）
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    // 4个UDP客户端
    for (uint32_t i = 1; i < 5; ++i) {
        UdpEchoClientHelper echoClient(Ipv4Address("10.1.1.1"), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(100));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
        echoClient.SetAttribute("PacketSize", UintegerValue(512));

        ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
        clientApps.Start(Seconds(2.0 + i));
        clientApps.Stop(Seconds(18.0));
    }

    // 流量监控
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // 运行仿真
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    // 结果输出
    monitor->SerializeToXmlFile("udp-wireless.xml", true, true);
    Simulator::Destroy();

    return 0;
}