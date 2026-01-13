#pragma once
#include <cstdint>
#include <string>
class GenMD5 {
public:
    GenMD5();
    void initsState();
    void updateValue(const uint8_t* input, size_t length);
    void updateValue(const std::string& str);
    std::string finalResult();
    static std::string calcFileMd5(const std::string& filename);
    static std::string calcStringMd5(const std::string& str);
    static std::string calcMemDataMd5(const void* pData, uint32_t uSize);
private:
    uint32_t m_uState[4];
    uint64_t m_uCount;
    uint8_t m_uBuffer[64];
    void transform(const uint8_t block[64]);
    void finalize();
};
