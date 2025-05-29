// udp-congestion.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    // 创建4个节点（2对通信）
    NodeContainer nodes;
    nodes.Create(4);

    // 瓶颈链路配置
    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));

    // 边缘链路配置
    PointToPointHelper edge;
    edge.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    edge.SetChannelAttribute("Delay", StringValue("2ms"));

    // 网络拓扑构建
    NetDeviceContainer dev0 = edge.Install(nodes.Get(0), nodes.Get(2));
    NetDeviceContainer dev1 = edge.Install(nodes.Get(1), nodes.Get(3));
    NetDeviceContainer dev2 = bottleneck.Install(nodes.Get(2), nodes.Get(3));

    // 协议栈安装
    InternetStackHelper stack;
    stack.Install(nodes);

    // IP地址分配
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(dev0);
    address.Assign(dev1);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterfaces = address.Assign(dev2);

    // 背景流量生成（模拟拥塞）
    OnOffHelper onoff("ns3::UdpSocketFactory", 
                     Address(InetSocketAddress(bottleneckInterfaces.GetAddress(1), 9)));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("DataRate", StringValue("800Kbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer bgApps = onoff.Install(nodes.Get(0));
    bgApps.Start(Seconds(1.0));
    bgApps.Stop(Seconds(9.0));

    // UDP测试流量
    UdpEchoClientHelper echoClient(bottleneckInterfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(500));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.02)));
    echoClient.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(8.0));

    // 流量监控
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // 运行仿真
    Simulator::Stop(Seconds(10));
    Simulator::Run();

    // 结果输出
    monitor->SerializeToXmlFile("udp-congestion.xml", true, true);
    Simulator::Destroy();

    return 0;
}