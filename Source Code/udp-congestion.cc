#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeUdpFlows");

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // 创建6个节点 Create 6 nodes
    NodeContainer nodes;
    nodes.Create(6);

    // 设置PointToPoint链路参数 Set PointToPoint link attributes
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // 配置链路 Connect nodes via point-to-point links
    NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(3));
    NetDeviceContainer d11 = p2p.Install(nodes.Get(1), nodes.Get(3));
    NetDeviceContainer d21 = p2p.Install(nodes.Get(2), nodes.Get(4));
    NetDeviceContainer d34 = p2p.Install(nodes.Get(3), nodes.Get(5));
    NetDeviceContainer d45 = p2p.Install(nodes.Get(4), nodes.Get(5));

    // 安装协议栈 Install Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // 分配IP地址 Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = address.Assign(d01);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i11 = address.Assign(d11);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i21 = address.Assign(d21);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i34 = address.Assign(d34);

    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i45 = address.Assign(d45);

    // 安装UDP服务器 Install UDP servers on node 5
    UdpServerHelper server1(8001), server2(8002), server3(8003);
    ApplicationContainer s1 = server1.Install(nodes.Get(5));
    ApplicationContainer s2 = server2.Install(nodes.Get(5));
    ApplicationContainer s3 = server3.Install(nodes.Get(5));
    s1.Start(Seconds(1.0));
    s2.Start(Seconds(1.0));
    s3.Start(Seconds(1.0));
    s1.Stop(Seconds(10.0));
    s2.Stop(Seconds(10.0));
    s3.Stop(Seconds(10.0));

    // 安装UDP客户端 Install UDP clients on nodes 0, 1, 2
    UdpClientHelper client1(i45.GetAddress(1), 8001);
    client1.SetAttribute("MaxPackets", UintegerValue(10000));
    client1.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
    client1.SetAttribute("PacketSize", UintegerValue(1024));

    UdpClientHelper client2(i45.GetAddress(1), 8002);
    client2.SetAttribute("MaxPackets", UintegerValue(10000));
    client2.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
    client2.SetAttribute("PacketSize", UintegerValue(1024));

    UdpClientHelper client3(i45.GetAddress(1), 8003);
    client3.SetAttribute("MaxPackets", UintegerValue(10000));
    client3.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
    client3.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer c1 = client1.Install(nodes.Get(0));
    ApplicationContainer c2 = client2.Install(nodes.Get(1));
    ApplicationContainer c3 = client3.Install(nodes.Get(2));
    c1.Start(Seconds(2.0));
    c2.Start(Seconds(2.0));
    c3.Start(Seconds(2.0));
    c1.Stop(Seconds(10.0));
    c2.Stop(Seconds(10.0));
    c3.Stop(Seconds(10.0));

    // 启用路由 Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 启用FlowMonitor Enable FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    // 输出流量统计信息 Output flow statistics with Chinese-English labels
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    for (auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        double throughput = flow.second.rxBytes * 8.0 /
                            (flow.second.timeLastRxPacket.GetSeconds() -
                             flow.second.timeFirstTxPacket.GetSeconds()) /
                            1e6;

        double delay = flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        double lossRatio = 100.0 * (flow.second.txPackets - flow.second.rxPackets) / flow.second.txPackets;

        std::cout << "FlowID 流编号: " << flow.first << "\n"
                  << "Source 源地址: " << t.sourceAddress << "\n"
                  << "Destination 目的地址: " << t.destinationAddress << "\n"
                  << "Throughput 吞吐量: " << throughput << " Mbps\n"
                  << "Average Delay 平均时延: " << delay << " s\n"
                  << "Packet Loss 丢包率: " << lossRatio << " %\n"
                  << "------------------------------------------\n";
    }

    Simulator::Destroy();
    return 0;
}
