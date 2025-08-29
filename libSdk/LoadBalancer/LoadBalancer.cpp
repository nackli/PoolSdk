/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
#include "LoadBalancer.h"
#include "LoadBalancerFactory.h" 
 
RoundRobinBalancer::RoundRobinBalancer() : currentIndex(0) {}

void RoundRobinBalancer::addServer(const ServerNode& server)  
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

ServerNode& RoundRobinBalancer::selectServer() 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");
	
	ServerNode& selected = servers[currentIndex];
	currentIndex = (currentIndex + 1) % servers.size();
	return selected;
}

void RoundRobinBalancer::releaseConnection(const std::string& serverAddress)  
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0) 
		{
			server.connections--;
			break;
		}
	}
}

void RoundRobinBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}



WeightedRoundRobinBalancer::WeightedRoundRobinBalancer(): totalWeight(0), gcd(1){}

void WeightedRoundRobinBalancer::addServer(const ServerNode& server) 
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
	initializeWeights();
}

ServerNode& WeightedRoundRobinBalancer::selectServer()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");


	size_t selectedIndex = 0;
	bool found = false;

	while (!found) 
	{
		for (size_t i = 0; i < servers.size(); ++i) 
			currentWeights[i] += effectiveWeights[i];


		for (size_t i = 0; i < servers.size(); ++i) 
		{
			if (currentWeights[i] >= gcd) 
			{
				currentWeights[i] -= totalWeight;
				selectedIndex = i;
				found = true;
				break;
			}
		}
	}

	return servers[selectedIndex];
}

int WeightedRoundRobinBalancer::computeGCD(int a, int b) 
{
	return b == 0 ? a : computeGCD(b, a % b);
}

int WeightedRoundRobinBalancer::computeGCDOfList()
 {
	int result = servers[0].weight;
	for (size_t i = 1; i < servers.size(); ++i) 
		result = computeGCD(result, servers[i].weight);
	return result;
}

void WeightedRoundRobinBalancer::initializeWeights() 
{
	effectiveWeights.clear();
	currentWeights.clear();
	totalWeight = 0;
	
	for (const auto& server : servers) 
	{
		effectiveWeights.push_back(server.weight);
		currentWeights.push_back(0);
		totalWeight += server.weight;
	}
	
	gcd = computeGCDOfList();
}


RandomBalancer::RandomBalancer() : rng(std::random_device{}()) {}

void RandomBalancer::addServer(const ServerNode& server) {
    std::lock_guard<std::mutex> lock(mutex);
    servers.push_back(server);
}

ServerNode& RandomBalancer::selectServer() 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");
	
	std::uniform_int_distribution<size_t> dist(0, servers.size() - 1);
	return servers[dist(rng)];
}

void RandomBalancer::releaseConnection(const std::string& serverAddress) 
 {
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0) 
		{
			server.connections--;
			break;
		}
	}
}

void RandomBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}



WeightedRandomBalancer::WeightedRandomBalancer() : totalWeight(0.0) {}

void WeightedRandomBalancer::addServer(const ServerNode& server) 
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
	initializeWeights();
}

ServerNode& WeightedRandomBalancer::selectServer()  
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	std::uniform_real_distribution<double> dist(0.0, totalWeight);
	double randomValue = dist(rng);
	double cumulativeWeight = 0.0;

	for (size_t i = 0; i < servers.size(); ++i)
	{
		cumulativeWeight += weights[i];
		if (randomValue <= cumulativeWeight) 
			return servers[i];
	}

	// Fallback to last server if rounding errors occur
	return servers.back();
}

void WeightedRandomBalancer::initializeWeights() 
{
	weights.clear();
	totalWeight = 0.0;
	
	for (const auto& server : servers) {
		weights.push_back(server.weight);
		totalWeight += server.weight;
	}
}



void LeastConnectionsBalancer::addServer(const ServerNode& server)  
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

ServerNode& LeastConnectionsBalancer::selectServer() 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	auto it = std::min_element(servers.begin(), servers.end(),
		[](const ServerNode& a, const ServerNode& b) {
			return a.connections < b.connections;
		});

	it->connections++;
	return *it;
}

void LeastConnectionsBalancer::releaseConnection(const std::string& serverAddress) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0) 
		{
			server.connections--;
			break;
		}
	}
}

void LeastConnectionsBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}

ServerNode& WeightedLeastConnectionsBalancer::selectServer() 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	auto it = std::min_element(servers.begin(), servers.end(),
		[](const ServerNode& a, const ServerNode& b) 
		{
			double aRatio = static_cast<double>(a.connections) / a.weight;
			double bRatio = static_cast<double>(b.connections) / b.weight;
			return aRatio < bRatio;
		});

	it->connections++;
	return *it;
}



void DestinationHashBalancer::addServer(const ServerNode& server)
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

ServerNode& DestinationHashBalancer::selectServer(const std::string& destination) 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	size_t hash = hashFunction(destination);
	size_t index = hash % servers.size();
	return servers[index];
}

ServerNode& DestinationHashBalancer::selectServer() 
{
	// Default implementation, requires destination IP
	throw std::runtime_error("DestinationHashBalancer requires a destination IP");
}

void DestinationHashBalancer::releaseConnection(const std::string& serverAddress) 
 {
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0) 
		{
			server.connections--;
			break;
		}
	}
}

void DestinationHashBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers)
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}

size_t DestinationHashBalancer::hashFunction(const std::string& key) const 
{
	std::hash<std::string> hasher;
	return hasher(key);
}

ServerNode& SourceHashBalancer::selectServer(const std::string& source) 
{
	return DestinationHashBalancer::selectServer(source);
}

void LBLCRBalancer::addServer(const ServerNode& server) 
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

void LBLCRBalancer::addDestinationServer(const std::string& destination, ServerNode& server) 
{
	std::lock_guard<std::mutex> lock(mutex);
	destinationMap[destination].push_back(&server);
}

ServerNode& LBLCRBalancer::selectServer(const std::string& destination)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	auto it = destinationMap.find(destination);
	if (it != destinationMap.end() && !it->second.empty()) 
	{
		// Find server with least connections in the destination's server list
		auto minIt = std::min_element(it->second.begin(), it->second.end(),
			[](const ServerNode* a, const ServerNode* b) {
				return a->connections < b->connections;
			});

		(*minIt)->connections++;
		return **minIt;
	}

	// Fallback to global least connections
	auto globalMinIt = std::min_element(servers.begin(), servers.end(),
		[](const ServerNode& a, const ServerNode& b) {
			return a.connections < b.connections;
		});

	globalMinIt->connections++;
	return *globalMinIt;
}

ServerNode& LBLCRBalancer::selectServer() 
{
	// Default implementation, requires destination
	throw std::runtime_error("LBLCRBalancer requires a destination");
}

void LBLCRBalancer::releaseConnection(const std::string& serverAddress)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0) 
		{
			server.connections--;
			break;
		}
	}
}

void LBLCRBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}




void ResponseTimeBalancer::addServer(const ServerNode& server)
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

ServerNode& ResponseTimeBalancer::selectServer() 
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");


	auto it = std::min_element(servers.begin(), servers.end(),
		[](const ServerNode& a, const ServerNode& b) {
			return a.responseTime < b.responseTime;
		});

	it->connections++;
	return *it;
}

void ResponseTimeBalancer::releaseConnection(const std::string& serverAddress) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers)
	{
		if (server.address == serverAddress && server.connections > 0)
		{
			server.connections--;
			break;
		}
	}
}

void ResponseTimeBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime) 
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}


void ProcessingPowerBalancer::addServer(const ServerNode& server) 
{
	std::lock_guard<std::mutex> lock(mutex);
	servers.push_back(server);
}

ServerNode& ProcessingPowerBalancer::selectServer()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (servers.empty()) 
		throw std::runtime_error("No servers available");

	auto it = std::min_element(servers.begin(), servers.end(),
		[](const ServerNode& a, const ServerNode& b) {
			double aLoad = static_cast<double>(a.connections) / a.processingPower;
			double bLoad = static_cast<double>(b.connections) / b.processingPower;
			return aLoad < bLoad;
		});

	it->connections++;
	return *it;
}

void ProcessingPowerBalancer::releaseConnection(const std::string& serverAddress)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress && server.connections > 0)
		{
			server.connections--;
			break;
		}
	}
}

void ProcessingPowerBalancer::updateServerStats(const std::string& serverAddress, 
					  int connections, double responseTime)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto& server : servers) 
	{
		if (server.address == serverAddress) 
		{
			server.connections = connections;
			server.responseTime = responseTime;
			break;
		}
	}
}


std::unique_ptr<LoadBalancer> LoadBalancerFactory::create(Type type) 
{
	switch (type) 
	{
		case Type::ROUND_ROBIN:
			return std::make_unique<RoundRobinBalancer>();
		case Type::WEIGHTED_ROUND_ROBIN:
			return std::make_unique<WeightedRoundRobinBalancer>();
		case Type::RANDOM:
			return std::make_unique<RandomBalancer>();
		case Type::WEIGHTED_RANDOM:
			return std::make_unique<WeightedRandomBalancer>();
		case Type::LEAST_CONNECTIONS:
			return std::make_unique<LeastConnectionsBalancer>();
		case Type::WEIGHTED_LEAST_CONNECTIONS:
			return std::make_unique<WeightedLeastConnectionsBalancer>();
		case Type::DESTINATION_HASH:
			return std::make_unique<DestinationHashBalancer>();
		case Type::SOURCE_HASH:
			return std::make_unique<SourceHashBalancer>();
		case Type::LBLCR:
			return std::make_unique<LBLCRBalancer>();
		case Type::RESPONSE_TIME:
			return std::make_unique<ResponseTimeBalancer>();
		case Type::PROCESSING_POWER:
			return std::make_unique<ProcessingPowerBalancer>();
		default:
			throw std::invalid_argument("Unknown load balancer type");
	}
}