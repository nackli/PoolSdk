#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

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
#endif
class PocessMange {
public:
    // 跨平台杀死进程
    static bool killByName(const std::string& processName) {
#ifdef _WIN32
        return killWindowsProcess(processName);
#else
        return killUnixProcess(processName);
#endif
    }

    // 列出所有进程
    static void listProcesses() {
#ifdef _WIN32
        listWindowsProcesses();
#else
        listUnixProcesses();
#endif
    }

private:
#ifdef _WIN32
    static bool killWindowsProcess(const std::string& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            return false;
        }

        bool killed = false;
        do {
            std::string currentProcess(pe32.szExeFile);
            if (currentProcess == processName) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
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

    static void listWindowsProcesses() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32)) {
            do {
                std::cout << "PID: " << pe32.th32ProcessID 
                          << "\tName: " << pe32.szExeFile << std::endl;
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

#else
    static bool killUnixProcess(const std::string& processName) {
        DIR* dir;
        struct dirent* ent;
        bool killed = false;

        if ((dir = opendir("/proc")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                // 检查是否是进程目录（纯数字）
                if (ent->d_type == DT_DIR && isNumber(ent->d_name)) {
                    std::string pid = ent->d_name;
                    std::string cmdlinePath = "/proc/" + pid + "/comm";
                    
                    FILE* fp = fopen(cmdlinePath.c_str(), "r");
                    if (fp) {
                        char name[256];
                        if (fgets(name, sizeof(name), fp)) {
                            // 移除换行符
                            name[strcspn(name, "\n")] = 0;
                            if (std::string(name) == processName) {
                                kill(atoi(pid.c_str()), SIGKILL);
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

    static void listUnixProcesses() {
        DIR* dir;
        struct dirent* ent;

        if ((dir = opendir("/proc")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_DIR && isNumber(ent->d_name)) {
                    std::string pid = ent->d_name;
                    std::string cmdlinePath = "/proc/" + pid + "/comm";
                    
                    FILE* fp = fopen(cmdlinePath.c_str(), "r");
                    if (fp) {
                        char name[256];
                        if (fgets(name, sizeof(name), fp)) {
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

    static bool isNumber(const char* str) {
        for (int i = 0; str[i]; i++) {
            if (!isdigit(str[i])) return false;
        }
        return true;
    }
#endif
};