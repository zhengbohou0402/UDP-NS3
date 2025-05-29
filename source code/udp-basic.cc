#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    // 启用日志输出 / Enable logging
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // 基础配置 / Basic configuration
    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(512));

    // 创建节点 / Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    // 点对点链路配置 / Point-to-point link configuration
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    // 协议栈安装 / Install protocol stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // IP地址分配 / IP address assignment
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // UDP服务器配置 / UDP server configuration
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // UDP客户端配置 / UDP client configuration
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1000));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.01)));
    echoClient.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(9.0));

    // 流量监控 / Flow monitoring
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // 打印初始配置 / Print initial configuration
    std::cout << "\n=== Simulation Configuration / 仿真配置 ===" << std::endl;
    std::cout << "Simulation Duration: 10 sec / 仿真时长: 10秒" << std::endl;
    std::cout << "Packet Size: 512 bytes / 数据包大小: 512字节" << std::endl;
    std::cout << "Tx Interval: 0.01 sec / 发送间隔: 0.01秒" << std::endl;
    std::cout << "Link Data Rate: 10Mbps / 链路速率: 10Mbps" << std::endl;
    std::cout << "Link Delay: 2ms / 链路延迟: 2毫秒" << std::endl;

    // 运行仿真 / Run simulation
    Simulator::Stop(Seconds(10));
    Simulator::Run();

    // 结果统计分析 / Result analysis
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    std::cout << "\n=== Performance Metrics / 性能指标 ===" << std::endl;
    for (auto iter = stats.begin(); iter != stats.end(); ++iter) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        
        std::cout << "\nFlow ID: " << iter->first << " (" 
                  << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;
                  
        // 数据包统计 / Packet statistics
        std::cout << "  Tx Packets: " << iter->second.txPackets 
                  << " / 发送包数: " << iter->second.txPackets << std::endl;
        std::cout << "  Rx Packets: " << iter->second.rxPackets
                  << " / 接收包数: " << iter->second.rxPackets << std::endl;
        
        // 丢包率 / Packet loss
        double lossRate = (iter->second.txPackets - iter->second.rxPackets) * 100.0 / iter->second.txPackets;
        std::cout << "  Packet Loss: " << std::fixed << std::setprecision(2) << lossRate
                  << "% / 丢包率: " << lossRate << "%" << std::endl;
        
        // 延迟 / Delay
        double avgDelay = iter->second.delaySum.GetSeconds() / iter->second.rxPackets;
        std::cout << "  Avg Delay: " << std::fixed << std::setprecision(4) << avgDelay
                  << " sec / 平均延迟: " << avgDelay << " 秒" << std::endl;
        
        // 吞吐量 / Throughput
        double throughput = iter->second.rxBytes * 8.0 / 
                          (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1000;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) << throughput
                  << " kbps / 吞吐量: " << throughput << " 千比特每秒" << std::endl;
    }

    // 结束信息 / Closing info
    std::cout << "\n=== Simulation Completed / 仿真完成 ===" << std::endl;
    std::cout << "Results saved to: udp-basic.xml / 结果已保存至: udp-basic.xml" << std::endl;

    // 原始数据输出 / Original data output
    monitor->SerializeToXmlFile("udp-basic.xml", false, false);
    Simulator::Destroy();

    return 0;
}