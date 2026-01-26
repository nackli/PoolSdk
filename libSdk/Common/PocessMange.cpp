#include "PocessMange.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#endif
/********************************************************************************************************************************************************************* */
#ifdef _WIN32
    static bool OnKillWindowsProcess(const std::string& processName) 
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) 
            return false;

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe32)) 
        {
            CloseHandle(hSnapshot);
            return false;
        }

        bool killed = false;
        do 
        {
            std::string currentProcess(pe32.szExeFile);
            if (currentProcess == processName) 
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) 
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    killed = true;
                    std::cout << "Killed: " << processName 
                              << " (PID: " << pe32.th32ProcessID << ")" << std::endl;
                }
            }
        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return killed;
    }

    static void OnListWindowsProcesses() 
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) 
            return;

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32)) 
        {
            do
            {
                std::cout << "PID: " << pe32.th32ProcessID 
                          << "\tName: " << pe32.szExeFile << std::endl;
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    static std::string OnEscapeArgument(const std::string& arg) 
    {
        if (arg.empty())
            return "\"\"";
        
        if (arg.find_first_of(" \t\n\"") == std::string::npos) 
            return arg;
        

        std::string escaped = "\"";
        for (char c : arg) 
        {
            if (c == '\"') 
                escaped += "\\\"";
            else
                escaped += c;
        }

        escaped += "\"";
        
        return escaped;
    }

    static int OnLaunchWindowsWithArgs(const std::string& strProcPath,
                                    const std::vector<std::string>& args,
                                    bool waitForExit) 
    {
        // 构建命令行字符串
        std::string strCommadLine = strProcPath;
        for (const auto& arg : args) {
            strCommadLine += " " + OnEscapeArgument(arg);
        }
        
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);
    
        
        if (!CreateProcessA(NULL, (LPSTR)strCommadLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si,  &pi)) 
        {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return -1;
        }
        
        if (waitForExit)
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode;
        }
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }
    

 static bool OnStartWindowsService(const std::string& serviceName, int timeoutSeconds)
  {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
        {
            std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
            return false;
        }
        
        SC_HANDLE service = OpenService(scm, serviceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);
        if (!service) 
        {
            std::cerr << "OpenService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(scm);
            return false;
        }
        
        // 检查服务是否已经在运行
        SERVICE_STATUS status;
        if (QueryServiceStatus(service, &status)) 
        {
            if (status.dwCurrentState == SERVICE_RUNNING) 
            {
                std::cout << "Service is already running" << std::endl;
                CloseServiceHandle(service);
                CloseServiceHandle(scm);
                return true;
            }
        }
        
        // 启动服务
        if (!StartService(service, 0, NULL)) 
        {
            DWORD error = GetLastError();
            if (error != ERROR_SERVICE_ALREADY_RUNNING)
            {
                std::cerr << "StartService failed: " << error << std::endl;
                CloseServiceHandle(service);
                CloseServiceHandle(scm);
                return false;
            }
        }
        
        // 等待服务启动
        auto startTime = std::chrono::steady_clock::now();
        bool started = false;
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count() < timeoutSeconds) 
        {
            
            if (!QueryServiceStatus(service, &status)) 
                break;
            
            if (status.dwCurrentState == SERVICE_RUNNING) 
            {
                started = true;
                break;
            } 
            else if (status.dwCurrentState == SERVICE_START_PENDING) 
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            else 
                break;
        }
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return started;
    }
    
    static bool stopWindowsService(const std::string& serviceName, int timeoutSeconds) 
    {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
            return false;
        
        SC_HANDLE service = OpenService(scm, serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!service) 
        {
            CloseServiceHandle(scm);
            return false;
        }
        
        // 检查服务是否已经停止
        SERVICE_STATUS status;
        if (QueryServiceStatus(service, &status)) 
        {
            if (status.dwCurrentState == SERVICE_STOPPED) 
            {
                CloseServiceHandle(service);
                CloseServiceHandle(scm);
                return true;
            }
        }
        
        // 发送停止信号
        SERVICE_STATUS_PROCESS ssp;
        DWORD bytesNeeded;
        if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
        {
            DWORD error = GetLastError();
            if (error != ERROR_SERVICE_NOT_ACTIVE) 
            {
                CloseServiceHandle(service);
                CloseServiceHandle(scm);
                return false;
            }
        }
        
        // 等待服务停止
        auto startTime = std::chrono::steady_clock::now();
        bool stopped = false;
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count() < timeoutSeconds) 
        {
            
            if (!QueryServiceStatus(service, &status)) 
                break;
            
            if (status.dwCurrentState == SERVICE_STOPPED) 
            {
                stopped = true;
                break;
            }
            else if (status.dwCurrentState == SERVICE_STOP_PENDING) 
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            else
                break;
        }
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return stopped;
    }
    
    static PocessMange::ServiceStatus getWindowsServiceStatus(const std::string& serviceName) 
    {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
            return PocessMange::ServiceStatus::ACCESS_DENIED;
        
        SC_HANDLE service = OpenService(scm, serviceName.c_str(), SERVICE_QUERY_STATUS);
        if (!service) 
        {
            CloseServiceHandle(scm);
            return PocessMange::ServiceStatus::NOT_FOUND;
        }
        
        SERVICE_STATUS status;
        PocessMange::ServiceStatus result = PocessMange::ServiceStatus::UNKNOWN_ERROR;
        
        if (QueryServiceStatus(service, &status))
        {
            switch (status.dwCurrentState) {
                case SERVICE_STOPPED:
                    result = PocessMange::ServiceStatus::STOPPED;
                    break;
                case SERVICE_START_PENDING:
                    result = PocessMange::ServiceStatus::START_PENDING;
                    break;
                case SERVICE_STOP_PENDING:
                    result = PocessMange::ServiceStatus::STOP_PENDING;
                    break;
                case SERVICE_RUNNING:
                    result = PocessMange::ServiceStatus::RUNNING;
                    break;
                case SERVICE_PAUSED:
                    result = PocessMange::ServiceStatus::PAUSED;
                    break;
                default:
                    result = PocessMange::ServiceStatus::UNKNOWN_ERROR;
            }
        }
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return result;
    }
    
    static PocessMange::ServiceInfo getWindowsServiceInfo(const std::string& serviceName) 
    {
        PocessMange::ServiceInfo info;
        info.name = serviceName;
        
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
        {
            info.status = PocessMange::ServiceStatus::ACCESS_DENIED;
            return info;
        }
        
        SC_HANDLE service = OpenService(scm, serviceName.c_str(), 
                                       SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
        if (!service) 
        {
            CloseServiceHandle(scm);
            info.status = PocessMange::ServiceStatus::NOT_FOUND;
            return info;
        }
        
        // 获取显示名称
        DWORD bytesNeeded = 0;
        QueryServiceConfig(service, NULL, 0, &bytesNeeded);
        if (bytesNeeded > 0)
        {
            std::vector<BYTE> buffer(bytesNeeded);
            LPQUERY_SERVICE_CONFIG config = (LPQUERY_SERVICE_CONFIG)buffer.data();
            
            if (QueryServiceConfig(service, config, bytesNeeded, &bytesNeeded)) 
                info.displayName = config->lpDisplayName ? config->lpDisplayName : "";
        }
        
        // 获取状态
        SERVICE_STATUS_PROCESS ssp;
        DWORD bytesNeededStatus;
        
        if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, 
                               (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), 
                               &bytesNeededStatus)) 
        {
            info.winProcessId = ssp.dwProcessId;
            
            switch (ssp.dwCurrentState) 
            {
                case SERVICE_STOPPED:
                    info.status = PocessMange::ServiceStatus::STOPPED;
                    info.statusText = "Stopped";
                    break;
                case SERVICE_START_PENDING:
                    info.status = PocessMange::ServiceStatus::START_PENDING;
                    info.statusText = "Start pending";
                    break;
                case SERVICE_STOP_PENDING:
                    info.status = PocessMange::ServiceStatus::STOP_PENDING;
                    info.statusText = "Stop pending";
                    break;
                case SERVICE_RUNNING:
                    info.status = PocessMange::ServiceStatus::RUNNING;
                    info.statusText = "Running";
                    break;
                case SERVICE_PAUSED:
                    info.status = PocessMange::ServiceStatus::PAUSED;
                    info.statusText = "Paused";
                    break;
                default:
                    info.status = PocessMange::ServiceStatus::UNKNOWN_ERROR;
                    info.statusText = "Unknown";
            }
        } 
        else 
        {
            info.status = PocessMange::ServiceStatus::UNKNOWN_ERROR;
            info.statusText = "Query failed";
        }
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return info;
    }
    
    static bool windowsServiceExists(const std::string& serviceName) 
    {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
            return false;
        
        SC_HANDLE service = OpenService(scm, serviceName.c_str(), SERVICE_QUERY_STATUS);
        bool exists = (service != NULL);
        
        if (service) 
            CloseServiceHandle(service);

        CloseServiceHandle(scm);
        
        return exists;
    }
    
    static std::vector<PocessMange::ServiceInfo> listWindowsServices() 
    {
        std::vector<PocessMange::ServiceInfo> services;
        
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
        if (!scm) 
            return services;
        
        DWORD bytesNeeded = 0;
        DWORD serviceCount = 0;
        DWORD resumeHandle = 0;
        
        EnumServicesStatus(scm, SERVICE_WIN32, SERVICE_STATE_ALL,
                          NULL, 0, &bytesNeeded, &serviceCount, &resumeHandle);
        
        if (bytesNeeded == 0)
        {
            CloseServiceHandle(scm);
            return services;
        }
        
        std::vector<BYTE> buffer(bytesNeeded);
        LPENUM_SERVICE_STATUS serviceStatus = (LPENUM_SERVICE_STATUS)buffer.data();
        
        if (EnumServicesStatus(scm, SERVICE_WIN32, SERVICE_STATE_ALL,
                             serviceStatus, bytesNeeded, &bytesNeeded,
                             &serviceCount, &resumeHandle)) 
        {
            for (DWORD i = 0; i < serviceCount; i++) 
            {
                PocessMange::ServiceInfo info;
                info.name = serviceStatus[i].lpServiceName;
                info.displayName = serviceStatus[i].lpDisplayName;
                
                switch (serviceStatus[i].ServiceStatus.dwCurrentState) 
                {
                    case SERVICE_STOPPED:
                        info.status = PocessMange::ServiceStatus::STOPPED;
                        break;
                    case SERVICE_RUNNING:
                        info.status = PocessMange::ServiceStatus::RUNNING;
                        break;
                    case SERVICE_PAUSED:
                        info.status = PocessMange::ServiceStatus::PAUSED;
                        break;
                    case SERVICE_START_PENDING:
                        info.status = PocessMange::ServiceStatus::START_PENDING;
                        break;
                    case SERVICE_STOP_PENDING:
                        info.status = PocessMange::ServiceStatus::STOP_PENDING;
                        break;
                    default:
                        info.status = PocessMange::ServiceStatus::UNKNOWN_ERROR;
                }
                
                services.push_back(info);
            }
        }
        
        CloseServiceHandle(scm);
        return services;
    }
    
    static bool installWindowsService(const std::string& serviceName, const std::string& displayName, const std::string& executablePath,
                                     const std::string& description, const std::string& dependencies, const std::string& account,
                                     const std::string& password) 
    {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!scm) 
            return false;

        
        std::string fullPath = "\"" + executablePath + "\"";
        
        SC_HANDLE service = CreateService(
            scm,
            serviceName.c_str(),
            displayName.c_str(),
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START, 
            SERVICE_ERROR_NORMAL,
            fullPath.c_str(),
            NULL, NULL,
            dependencies.empty() ? NULL : dependencies.c_str(),
            account.empty() ? NULL : account.c_str(),
            password.empty() ? NULL : password.c_str()
        );
        
        if (!service)
        {
            CloseServiceHandle(scm);
            return false;
        }
        
        if (!description.empty()) 
        {
            SERVICE_DESCRIPTION sd;
            sd.lpDescription = const_cast<char*>(description.c_str());
            ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &sd);
        }
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return true;
    }
    
    static bool uninstallWindowsService(const std::string& serviceName)
    {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (!scm) 
            return false;

        SC_HANDLE service = OpenService(scm, serviceName.c_str(), 
                                       SERVICE_STOP | DELETE);
        if (!service) 
        {
            CloseServiceHandle(scm);
            return false;
        }
        
        // 先停止服务
        SERVICE_STATUS status;
        ControlService(service, SERVICE_CONTROL_STOP, &status);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 删除服务
        bool success = DeleteService(service) != 0;
        
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        
        return success;
    }
#else
    static bool OnIsNumber(const char* str) 
    {
        for (int i = 0; str[i]; i++) 
        {
            if (!isdigit(str[i])) 
                return false;
        }
        return true;
    }

    static bool OnKillUnixProcess(const std::string& processName,bool bForce = true) 
    {
        DIR* dir;
        struct dirent* ent;
        bool killed = false;

        if ((dir = opendir("/proc")) != NULL) 
        {
            while ((ent = readdir(dir)) != NULL)
             {
                // 检查是否是进程目录（纯数字）
                if (ent->d_type == DT_DIR && OnIsNumber(ent->d_name)) 
                {
                    std::string pid = ent->d_name;
                    std::string cmdlinePath = "/proc/" + pid + "/comm";
                    
                    FILE* fp = fopen(cmdlinePath.c_str(), "r");
                    if (fp) 
                    {
                        char szProcName[256];
                        if (fgets(szProcName, sizeof(szProcName), fp))
                         {
                            // 移除换行符
                            szProcName[strcspn(szProcName, "\n")] = 0;
                            if (std::string(szProcName) == processName) 
                            {
                                if(bForce)
                                    kill(atoi(pid.c_str()), SIGKILL);
                                else
                                    kill(atoi(pid.c_str()), SIGQUIT);

                                std::cout << "Killed: " << processName 
                                          << " (PID: " << pid << ")" << std::endl;
                                killed = true;
                            }
                        }
                        fclose(fp);
                    }
                }
            }
            closedir(dir);
        }
        return killed;
    }

    static void OnListUnixProcesses() 
    {
        DIR* dir;
        struct dirent* ent;

        if ((dir = opendir("/proc")) != NULL) 
        {
            while ((ent = readdir(dir)) != NULL) 
            {
                if (ent->d_type == DT_DIR && OnIsNumber(ent->d_name)) 
                {
                    std::string pid = ent->d_name;
                    std::string cmdlinePath = "/proc/" + pid + "/comm";
                    
                    FILE* fp = fopen(cmdlinePath.c_str(), "r");
                    if (fp) 
                    {
                        char name[256];
                        if (fgets(name, sizeof(name), fp))
                         {
                            name[strcspn(name, "\n")] = 0;
                            std::cout << "PID: " << pid 
                                      << "\tName: " << name << std::endl;
                        }
                        fclose(fp);
                    }
                }
            }
            closedir(dir);
        }
    }

    // Unix/Linux实现
    static int OnLaunchUnixWithArgs(const std::string& strProgram,
                                 const std::vector<std::string>& args,
                                 bool waitForExit)
    {
        pid_t pid = fork();
        
        if (pid == -1) 
        {
            std::cerr << "Fork failed" << std::endl;
            return -1;
        } 
        else if (pid == 0) 
        {
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(strProgram.c_str()));
            
            for (const auto& arg : args)
                argv.push_back(const_cast<char*>(arg.c_str()));

            argv.push_back(nullptr);
            
    
            execvp(strProgram.c_str(), argv.data());
            
            // 如果execvp失败
            std::cerr << "Exec failed for: " << strProgram << std::endl;
            exit(EXIT_FAILURE);
        } 
        else 
        {
            // 父进程
            if (waitForExit) 
            {
                int status;
                waitpid(pid, &status, 0);
                
                if (WIFEXITED(status))
                    return WEXITSTATUS(status);
                return -1;
            } 
            return 0;
        }
    }  
    
    
     // Unix/Linux/macOS 服务管理实现
    static std::string getServicePid(const std::string& serviceName) 
    {
        std::string pid;
        
        // 尝试多种方法获取PID
        std::vector<std::string> commands = 
        {
            "pidof " + serviceName,
            "pgrep -f " + serviceName,
            "ps aux | grep \"" + serviceName + "\" | grep -v grep | awk '{print $2}'"
        };
        
        for (const auto& cmd : commands) 
        {
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) 
            {
                char buffer[64];
                if (fgets(buffer, sizeof(buffer), pipe) != NULL)
                {
                    pid = buffer;
                    pid.erase(pid.find_last_not_of(" \t\n\r") + 1);
                    if (!pid.empty()) 
                    {
                        pclose(pipe);
                        return pid;
                    }
                }
                pclose(pipe);
            }
        }
        
        return "";
    }
    
    static PocessMange::ServiceStatus getUnixServiceStatus(const std::string& serviceName) 
    {
        std::string command;
        std::string checkOutput;
        
        if (access("/bin/systemctl", X_OK) == 0) 
        {
            command = "systemctl is-active " + serviceName + " 2>/dev/null";
            FILE* pipe = popen(command.c_str(), "r");
            if (pipe) 
            {
                char buffer[128];
                if (fgets(buffer, sizeof(buffer), pipe) != NULL) 
                    checkOutput = buffer;

                pclose(pipe);
            }
            
            if (checkOutput.find("active") != std::string::npos) 
                return PocessMange::ServiceStatus::RUNNING;
            else if (checkOutput.find("inactive") != std::string::npos)
                return PocessMange::ServiceStatus::STOPPED;
        }
        
        // 
        if (access("/sbin/service", X_OK) == 0) 
            command = "service " + serviceName + " status 2>&1";
        else if (access(std::string("/etc/init.d/" + serviceName).c_str(), X_OK) == 0) 
            command = "/etc/init.d/" + serviceName + " status 2>&1";
        else{
            //
            std::string pid = getServicePid(serviceName);
            return pid.empty() ? PocessMange::ServiceStatus::STOPPED : PocessMange::ServiceStatus::RUNNING;
        }
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) 
            return PocessMange::ServiceStatus::UNKNOWN_ERROR;
        
        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
            output += buffer;

        pclose(pipe);
        
        if (output.find("running") != std::string::npos ||
            output.find("active") != std::string::npos ||
            output.find("started") != std::string::npos) 
            return PocessMange::ServiceStatus::RUNNING;

        else if (output.find("stopped") != std::string::npos ||
                  output.find("inactive") != std::string::npos) 
            return PocessMange::ServiceStatus::STOPPED;
        
        return PocessMange::ServiceStatus::UNKNOWN_ERROR;
    }
    

    static bool startUnixService(const std::string& serviceName, 
                               int timeoutSeconds,
                               const std::vector<std::string>& arguments) {
        std::string command;
        if (access("/bin/systemctl", X_OK) == 0)                                    // systemd (Linux)
            command = "systemctl start " + serviceName;
        else if (access("/sbin/service", X_OK) == 0)                                // SysV init
            command = "service " + serviceName + " start";
        else if (access(std::string("/etc/init.d/" + serviceName).c_str(), X_OK) == 0)           // init.d            
            command = "/etc/init.d/" + serviceName + " start";
        else if (access("/usr/sbin/service", X_OK) == 0)                            // macOS launchctl
            command = "sudo launchctl start " + serviceName;
        else 
        {
            command = serviceName;
            if (!arguments.empty()) 
            {
                for (const auto& arg : arguments) 
                    command += " " + arg;
            }
            command += " &";  // 
        }
        
        int result = system(command.c_str());
        
        if (result != 0 && !arguments.empty()) 
        {
            // 
            std::string directCommand = serviceName;
            for (const auto& arg : arguments) 
                directCommand += " \"" + arg + "\"";

            directCommand += " &";

            result = system(directCommand.c_str());
        }
        
        if (result == 0) 
        {
            // 
            auto startTime = std::chrono::steady_clock::now();
            while (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count() < timeoutSeconds) 
            {
                
                if (getUnixServiceStatus(serviceName) == PocessMange::ServiceStatus::RUNNING) 
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        
        return false;
    }
    
    static bool stopUnixService(const std::string& serviceName, int timeoutSeconds) 
    {
        std::string command;
        
        if (access("/bin/systemctl", X_OK) == 0) 
            command = "systemctl stop " + serviceName;
        else if (access("/sbin/service", X_OK) == 0) 
            command = "service " + serviceName + " stop";
        else if (access(std::string("/etc/init.d/" + serviceName).c_str(), X_OK) == 0) 
            command = "/etc/init.d/" + serviceName + " stop";
        else if (access("/usr/sbin/service", X_OK) == 0) 
            command = "sudo launchctl stop " + serviceName;
        else 
        {
            // 
            std::string pid = getServicePid(serviceName);
            if (!pid.empty())
                command = "kill " + pid;
            else
                return false;
        }
        
        int result = system(command.c_str());
        
        if (result == 0) 
        {
            // 
            auto startTime = std::chrono::steady_clock::now();
            while (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count() < timeoutSeconds) 
            {
                
                if (getUnixServiceStatus(serviceName) == PocessMange::ServiceStatus::STOPPED) 
                    return true;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        
        return false;
    }

    static PocessMange::ServiceInfo getUnixServiceInfo(const std::string& serviceName) {
        PocessMange::ServiceInfo info;
        info.name = serviceName;
        info.status = getUnixServiceStatus(serviceName);
        
        // 
        info.displayName = serviceName;
        std::string pid = getServicePid(serviceName);
        if (!pid.empty()) 
            info.processId = std::stoi(pid);
        
        switch (info.status) 
        {
            case PocessMange::ServiceStatus::RUNNING:
                info.statusText = "Running";
                break;
            case PocessMange::ServiceStatus::STOPPED:
                info.statusText = "Stopped";
                break;
            default:
                info.statusText = "Unknown";
        }
        
        return info;
    }
    
    static bool unixServiceExists(const std::string& serviceName) 
    {
        if (access("/bin/systemctl", X_OK) == 0) 
        {
            std::string command = "systemctl list-unit-files | grep -q \"^" + serviceName + "\\.service\"";
            return system(command.c_str()) == 0;
        }
        else if (access(std::string("/etc/init.d/" + serviceName).c_str(), X_OK) == 0) 
            return true;
        else if (access("/sbin/service", X_OK) == 0)
        {
            std::string command = "service --status-all 2>&1 | grep -q " + serviceName;
            return system(command.c_str()) == 0;
        }

        return false;
    }
    
    static std::vector<PocessMange::ServiceInfo> listUnixServices() 
    {
        std::vector<PocessMange::ServiceInfo> services;
        std::string command;
        
        if (access("/bin/systemctl", X_OK) == 0)
            command = "systemctl list-units --type=service --all --no-pager";
       else if (access("/sbin/service", X_OK) == 0) 
            command = "service --status-all 2>&1";
        else 
            command = "ls /etc/init.d/ 2>/dev/null";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) 
            return services;
        
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
        {
            std::string line(buffer);
            
            PocessMange::ServiceInfo info;
            if (access("/bin/systemctl", X_OK) == 0)
            {
                // 解析systemctl输出
                size_t dotPos = line.find('.');
                if (dotPos != std::string::npos) 
                {
                    info.name = line.substr(0, dotPos);
                    
                    if (line.find("running") != std::string::npos) 
                        info.status = PocessMange::ServiceStatus::RUNNING;

                    else if (line.find("stopped") != std::string::npos) 
                        info.status = PocessMange::ServiceStatus::STOPPED;

                    
                    services.push_back(info);
                }
            } 
            else
            {

                info.name = line;
                info.name.erase(info.name.find_last_not_of(" \t\n\r") + 1);
                info.status = PocessMange::ServiceStatus::UNKNOWN_ERROR;
                services.push_back(info);
            }
        }
        
        pclose(pipe);
        return services;
    }
    
    static bool installUnixService(const std::string& serviceName,
                                  const std::string& displayName,
                                  const std::string& executablePath,
                                  const std::string& description) {
        // 创建systemd服务文件
        std::string serviceFile = "/etc/systemd/system/" + serviceName + ".service";
        
        // 检查是否有写入权限
        if (access("/etc/systemd/system", W_OK) != 0) {
            std::cerr << "No permission to write service file. Run as root." << std::endl;
            return false;
        }
        
        std::string serviceContent = 
            "[Unit]\n"
            "Description=" + description + "\n"
            "After=network.target\n\n"
            "[Service]\n"
            "Type=simple\n"
            "ExecStart=" + executablePath + "\n"
            "Restart=on-failure\n"
            "RestartSec=10\n\n"
            "[Install]\n"
            "WantedBy=multi-user.target\n";
        
        // 写入服务文件
        FILE* fp = fopen(serviceFile.c_str(), "w");
        if (!fp) {
            return false;
        }
        
        fputs(serviceContent.c_str(), fp);
        fclose(fp);
        
        // 重新加载systemd
        system("systemctl daemon-reload");
        
        // 启用服务
        std::string enableCmd = "systemctl enable " + serviceName;
        return system(enableCmd.c_str()) == 0;
    }
    
    static bool uninstallUnixService(const std::string& serviceName) 
    {
        // 停止服务
        stopUnixService(serviceName, 10);
        
        // 禁用服务
        if (access("/bin/systemctl", X_OK) == 0) {
            std::string disableCmd = "systemctl disable " + serviceName;
            system(disableCmd.c_str());
            
            // 删除服务文件
            std::string serviceFile = "/etc/systemd/system/" + serviceName + ".service";
            std::string rmCmd = "rm -f " + serviceFile;
            system(rmCmd.c_str());
            
            system("systemctl daemon-reload");
            return true;
        }
        
        return false;
    }

    std::string serviceStatusToString(PocessMange::ServiceStatus status) {
        switch (status) {
            case PocessMange::ServiceStatus::RUNNING:
                return "RUNNING";
            case PocessMange::ServiceStatus::STOPPED:
                return "STOPPED";
            case PocessMange::ServiceStatus::PAUSED:
                return "PAUSED";
            case PocessMange::ServiceStatus::START_PENDING:
                return "START_PENDING";
            case PocessMange::ServiceStatus::STOP_PENDING:
                return "STOP_PENDING";
            case PocessMange::ServiceStatus::NOT_FOUND:
                return "NOT_FOUND";
            case PocessMange::ServiceStatus::ACCESS_DENIED:
                return "ACCESS_DENIED";
            default:
                return "UNKNOWN_ERROR";
        }
    }   
#endif
/********************************************************************************************************************************************************************* */
bool PocessMange::killByName(const std::string& strProcName,bool bForce)
{
#ifdef _WIN32
    return OnKillWindowsProcess(strProcName);
#else
    return OnKillUnixProcess(strProcName, bForce);
#endif
}

void PocessMange::listProcesses() 
{
#ifdef _WIN32
    OnListWindowsProcesses();
#else
    OnListUnixProcesses();
#endif
}

bool PocessMange::createProcWithArg(const std::string& strProcName, const std::vector<std::string>& args, bool waitForExit)
{
#ifdef _WIN32
    return OnLaunchWindowsWithArgs(strProcName, args, waitForExit);
#else
    return OnLaunchUnixWithArgs(strProcName, args, waitForExit);
#endif
}

PocessMange::ServiceStatus PocessMange::getServiceStatus(const std::string& serviceName)
{
#ifdef _WIN32
    return getWindowsServiceStatus(serviceName);
#else
    return getUnixServiceStatus(serviceName);
#endif
}

PocessMange::ServiceInfo PocessMange::getServiceInfo(const std::string& serviceName)
{
#ifdef _WIN32
    return getWindowsServiceInfo(serviceName);
#else
    return getUnixServiceInfo(serviceName);
#endif
}

bool PocessMange::serviceExists(const std::string& serviceName)
{
#ifdef _WIN32
    return windowsServiceExists(serviceName);
#else
    return unixServiceExists(serviceName);
#endif
}

std::vector<PocessMange::ServiceInfo> PocessMange::listServices()
{
#ifdef _WIN32
    return listWindowsServices();
#else
    return listUnixServices();
#endif
}

bool PocessMange::installService(const std::string& serviceName, const std::string& displayName,  const std::string& executablePath,
                            const std::string& description , const std::string& dependencies ,  const std::string& account,
                            const std::string& password )
{
#ifdef _WIN32
    return installWindowsService(serviceName, displayName, executablePath, 
                                description, dependencies, account, password);
#else
    return installUnixService(serviceName, displayName, executablePath, description);
#endif    
}

bool PocessMange::uninstallService(const std::string& serviceName)
{
#ifdef _WIN32
    return uninstallWindowsService(serviceName);
#else
    return uninstallUnixService(serviceName);
#endif
} 

bool PocessMange::startService(const std::string& serviceName, int timeoutSeconds, const std::vector<std::string>& arguments)
{
#ifdef _WIN32
        return OnStartWindowsService(serviceName, timeoutSeconds);
#else
        return startUnixService(serviceName, timeoutSeconds, arguments);
#endif
}

bool PocessMange::stopService(const std::string& serviceName,  int timeoutSeconds)
{
#ifdef _WIN32
        return stopWindowsService(serviceName, timeoutSeconds);
#else
        return stopUnixService(serviceName, timeoutSeconds);
#endif
}

bool PocessMange::restartService(const std::string& serviceName, int stopTimeout, int startTimeout)
{
    if (stopService(serviceName, stopTimeout)) 
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return startService(serviceName, startTimeout);
    }
    return false;
}