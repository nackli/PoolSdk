// 负载均衡器工厂
class LoadBalancerFactory {
public:
    enum class Type {
        ROUND_ROBIN,                    //轮转调度 (Round Robin): 依次选择每个服务器
        WEIGHTED_ROUND_ROBIN,           //加权轮转调度 (Weighted Round Robin): 根据服务器权重分配请求
        RANDOM,                         //随机均衡调度 (Random): 随机选择服务器
        WEIGHTED_RANDOM,                //加权随机均衡调度 (Weighted Random): 根据权重随机选择服务器
        LEAST_CONNECTIONS,              //最小连接调度 (Least Connections): 选择当前连接数最少的服务器
        WEIGHTED_LEAST_CONNECTIONS,     //加权最小连接调度 (Weighted Least Connections): 考虑权重的连接数最少服务器
        DESTINATION_HASH,               //目标地址散列调度 (Destination IP Hash): 基于目标IP地址的哈希选择
        SOURCE_HASH,                    //源地址散列调度 (Source IP Hash): 基于源IP地址的哈希选择
        LBLCR,                          //带复制的基于局部性最少链接调度 (LBLCR): 结合局部性和最少连接
        RESPONSE_TIME,                  //响应速度均衡调度 (Response Time): 选择响应时间最短的服务器
        PROCESSING_POWER                //处理能力均衡调度 (Processing Power): 根据服务器处理能力分配请求
    };
    static std::unique_ptr<LoadBalancer> create(Type type);
};