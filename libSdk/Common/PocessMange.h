
#include <string>
#include <vector>
class PocessMange {
public:    
    enum class ServiceStatus {
        RUNNING,
        STOPPED,
        PAUSED,
        START_PENDING,
        STOP_PENDING,
        NOT_FOUND,
        ACCESS_DENIED,
        UNKNOWN_ERROR
    };

    struct ServiceInfo {
        std::string name;
        std::string displayName;
        ServiceStatus status;
        std::string statusText;
    #ifndef _WIN32 
        pid_t processId;  // Unix进程ID
    #else   
        DWORD winProcessId; // Windows进程ID
    #endif    
    };  

public:
    static bool killByName(const std::string& strProcName,bool bForce= true);
    static void listProcesses();
    static bool createProcWithArg(const std::string& strProcName, const std::vector<std::string>& args = {}, bool waitForExit = false);
    static bool startService(const std::string& serviceName, int timeoutSeconds = 30, const std::vector<std::string>& arguments = {});
    static bool stopService(const std::string& serviceName,  int timeoutSeconds = 30);
    static ServiceStatus getServiceStatus(const std::string& serviceName);
    static ServiceInfo getServiceInfo(const std::string& serviceName);
    static bool serviceExists(const std::string& serviceName);
    static std::vector<ServiceInfo> listServices();
    static bool installService(const std::string& serviceName, const std::string& displayName,  const std::string& executablePath,
                              const std::string& description = "", const std::string& dependencies = "",  const std::string& account = "",
                              const std::string& password = "");
    static bool uninstallService(const std::string& serviceName);  
    static bool restartService(const std::string& serviceName, int stopTimeout = 10, int startTimeout = 30);
  
};