#include "LoadBalancer.h"
// 示例使用
int main() {
    try {
        // 创建负载均衡器
        auto balancer = LoadBalancerFactory::create(
            LoadBalancerFactory::Type::WEIGHTED_LEAST_CONNECTIONS);
        
        // 添加服务器
        balancer->addServer(ServerNode("192.168.1.1", 8080, 3, 10)); // weight=3, processingPower=10
        balancer->addServer(ServerNode("192.168.1.2", 8080, 2, 8));
        balancer->addServer(ServerNode("192.168.1.3", 8080, 1, 5));
        
        // 模拟请求
        for (int i = 0; i < 10; ++i) {
            ServerNode& server = balancer->selectServer();
            std::cout << "Request " << i+1 << " routed to " << server.address 
                      << " (Connections: " << server.connections 
                      << ", Weight: " << server.weight << ")\n";
            
            // 模拟释放连接
            if (i % 3 == 0) {
                balancer->releaseConnection(server.address);
                std::cout << "Released connection from " << server.address << "\n";
            }
            
            // 模拟更新服务器状态
            if (i % 4 == 0) {
                balancer->updateServerStats(server.address, 
                                          server.connections, 
                                          (i % 5) * 0.1);
            }
        }
        
        // 测试目标地址散列
        auto hashBalancer = LoadBalancerFactory::create(
            LoadBalancerFactory::Type::DESTINATION_HASH);
        
        hashBalancer->addServer(ServerNode("10.0.0.1", 80));
        hashBalancer->addServer(ServerNode("10.0.0.2", 80));
        hashBalancer->addServer(ServerNode("10.0.0.3", 80));
        
        std::string clientIp = "192.168.0.100";
        ServerNode& hashedServer = dynamic_cast<DestinationHashBalancer*>(hashBalancer.get())
                                  ->selectServer(clientIp);
        std::cout << "Client " << clientIp << " always routed to " 
                  << hashedServer.address << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}