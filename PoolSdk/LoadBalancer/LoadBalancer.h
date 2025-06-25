#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <ctime>
#include <numeric>
#include <cmath>
#include <memory>
#include <mutex>
using namespace std;
// 服务器节点结构体
struct ServerNode {
    std::string address;
    int port;
    int weight;          // 权重
    int connections;     // 当前连接数
    int processingPower; // 处理能力
    double responseTime; // 响应时间

    ServerNode(std::string addr, int p, int w = 1, int pp = 1, double rt = 0.0)
        : address(addr), port(p), weight(w), connections(0), 
          processingPower(pp), responseTime(rt) {}
};

// 负载均衡器基类
class LoadBalancer {
public:
    virtual ~LoadBalancer() = default;
    virtual void addServer(const ServerNode& server) = 0;
    virtual ServerNode& selectServer() = 0;
    virtual void releaseConnection(const std::string& serverAddress) = 0;
    virtual void updateServerStats(const std::string& serverAddress, 
                                 int connections, double responseTime) = 0;
};

// 轮转调度 (Round Robin)
class RoundRobinBalancer : public LoadBalancer {
public:
    RoundRobinBalancer();
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override;
    void releaseConnection(const std::string& serverAddress) override;
    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override;
protected:
    std::vector<ServerNode> servers;
    size_t currentIndex;
    std::mutex mutex;	
};

// 加权轮转调度 (Weighted Round Robin)
class WeightedRoundRobinBalancer : public RoundRobinBalancer {
public:
    WeightedRoundRobinBalancer();
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override;
private:
    int computeGCD(int a, int b);
    int computeGCDOfList();
    void initializeWeights();	
private:
    std::vector<int> effectiveWeights;
    std::vector<int> currentWeights;
    int totalWeight;
    int gcd;

};

// 随机均衡调度 (Random)
class RandomBalancer : public LoadBalancer {
public:
    RandomBalancer();
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override;

    void releaseConnection(const std::string& serverAddress) override ;
    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override;
protected:
    std::vector<ServerNode> servers;
    std::mutex mutex;
    std::mt19937 rng;
	
};

// 加权随机均衡调度 (Weighted Random)
class WeightedRandomBalancer : public RandomBalancer {
private:
    std::vector<double> weights;
    double totalWeight;
    void initializeWeights();
public:
    WeightedRandomBalancer();
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override;
};

// 最小连接调度 (Least Connections)
class LeastConnectionsBalancer : public LoadBalancer {
protected:
    std::vector<ServerNode> servers;
    std::mutex mutex;

public:
    void addServer(const ServerNode& server) override;

    ServerNode& selectServer() override;

    void releaseConnection(const std::string& serverAddress) override;

    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override;
};

// 加权最小连接调度 (Weighted Least Connections)
class WeightedLeastConnectionsBalancer : public LeastConnectionsBalancer {
public:
    ServerNode& selectServer() override;
};

// 目标地址散列调度 (Destination IP Hash)
class DestinationHashBalancer : public LoadBalancer {
protected:
    std::vector<ServerNode> servers;
    std::mutex mutex;
    size_t hashFunction(const std::string& key) const;
public:
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer(const std::string& destination);
    ServerNode& selectServer() override ;
    void releaseConnection(const std::string& serverAddress) override ;
    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override ;
};

// 源地址散列调度 (Source IP Hash)
class SourceHashBalancer : public DestinationHashBalancer {
public:
    ServerNode& selectServer(const std::string& source);
};

// 带复制的基于局部性最少链接调度 (Locality-Based Least Connections with Replication)
class LBLCRBalancer : public LoadBalancer {
private:
    std::vector<ServerNode> servers;
    std::unordered_map<std::string, std::vector<ServerNode*>> destinationMap;
    std::mutex mutex;

public:
    void addServer(const ServerNode& server) override;
    void addDestinationServer(const std::string& destination, ServerNode& server);
    ServerNode& selectServer(const std::string& destination);
    ServerNode& selectServer() override;
    void releaseConnection(const std::string& serverAddress) override;
    void updateServerStats(const std::string& serverAddress,
        int connections, double responseTime) override;
};

// 响应速度均衡调度 (Response Time)
class ResponseTimeBalancer : public LoadBalancer {
protected:
    std::vector<ServerNode> servers;
    std::mutex mutex;

public:
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override ;
    void releaseConnection(const std::string& serverAddress) override;
    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override;
};

// 处理能力均衡调度 (Processing Power)
class ProcessingPowerBalancer : public LoadBalancer {
protected:
    std::vector<ServerNode> servers;
    std::mutex mutex;
public:
    void addServer(const ServerNode& server) override;
    ServerNode& selectServer() override;
    void releaseConnection(const std::string& serverAddress) override;
    void updateServerStats(const std::string& serverAddress, 
                          int connections, double responseTime) override;
};


