// udp-wireless-sim.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    // 创建5个移动节点 / Create 5 mobile nodes
    NodeContainer nodes;
    nodes.Create(5);

    // 配置无线物理层 / Configure WiFi PHY
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // 配置MAC层为Adhoc模式 / Setup MAC as Adhoc
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    // 安装无线设备 / Install devices
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // 设置移动模型，随机游走 / Mobility model: Random Walk 2D
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue("100.0"),
                                 "Y", StringValue("100.0"),
                                 "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue(Rectangle(0, 200, 0, 200)));
    mobility.Install(nodes);

    // 安装协议栈 / Install internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // IP地址分配 / Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // UDP服务器安装在节点0 / UDP Echo server at node 0
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    // 4个UDP客户端分别安装在节点1-4 / 4 UDP clients at nodes 1 to 4
    for (uint32_t i = 1; i < 5; ++i) {
        UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(10));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(512));

        ApplicationContainer clientApps = echoClient.Install(nodes.Get(i));
        clientApps.Start(Seconds(2.0 + i));
        clientApps.Stop(Seconds(18.0));
    }

    // 流量监控 / Flow monitor
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // 运行仿真 / Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    // 输出结果，打印到控制台 / Print flow stats in console 中英文结合
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::cout << "===== UDP Wireless Simulation Results / UDP无线仿真结果 =====" << std::endl;
    for (auto iter : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter.first);
        std::cout << "Flow ID: " << iter.first 
                  << " 源IP(Source IP): " << t.sourceAddress 
                  << " 目标IP(Dest IP): " << t.destinationAddress << std::endl;
        std::cout << "发送包数(Packets Sent): " << iter.second.txPackets 
                  << ", 接收包数(Packets Received): " << iter.second.rxPackets << std::endl;
        std::cout << "丢包数(Packets Lost): " << iter.second.txPackets - iter.second.rxPackets << std::endl;
        std::cout << "总字节数(Bytes Transferred): " << iter.second.rxBytes << std::endl;
        std::cout << "平均延迟(Average Delay): " 
                  << (iter.second.delaySum.GetSeconds() / iter.second.rxPackets) << " 秒(s)" << std::endl;
        std::cout << "----------------------------------------------------------" << std::endl;
    }

    // 保存结果文件（可选）/ Save to XML file
    monitor->SerializeToXmlFile("udp-wireless-sim-result.xml", true, true);

    Simulator::Destroy();
    return 0;
}
