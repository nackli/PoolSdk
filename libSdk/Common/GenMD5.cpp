#include <iomanip>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "GenMD5.h"

static const uint32_t S[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static const uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & ~z);
}

static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | ~z);
}

static inline uint32_t rotate_left(uint32_t x, uint32_t n) {
    return (x << n) | (x >> (32 - n));
}

// Every step of the transformation
static inline void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
    uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + F(b, c, d) + x + ac, s) + b;
}

static inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
    uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + G(b, c, d) + x + ac, s) + b;
}

static inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
    uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + H(b, c, d) + x + ac, s) + b;
}

static inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
    uint32_t x, uint32_t s, uint32_t ac) {
    a = rotate_left(a + I(b, c, d) + x + ac, s) + b;
}
GenMD5::GenMD5()
{
    initsState();
}
// Process a 64-byte block
void GenMD5::transform(const uint8_t block[64]) 
{
    uint32_t a = m_uState[0];
    uint32_t b = m_uState[1];
    uint32_t c = m_uState[2];
    uint32_t d = m_uState[3];
    uint32_t x[16];

    // Convert a byte array to a 32-bit integer array (Note: MD5 uses little-endian)
    for (int i = 0, j = 0; i < 16; ++i, j += 4) 
    {
        x[i] = ((uint32_t)block[j]) |
            (((uint32_t)block[j + 1]) << 8) |
            (((uint32_t)block[j + 2]) << 16) |
            (((uint32_t)block[j + 3]) << 24);
    }

    // first
    FF(a, b, c, d, x[0], S[0], K[0]);
    FF(d, a, b, c, x[1], S[1], K[1]);
    FF(c, d, a, b, x[2], S[2], K[2]);
    FF(b, c, d, a, x[3], S[3], K[3]);
    FF(a, b, c, d, x[4], S[4], K[4]);
    FF(d, a, b, c, x[5], S[5], K[5]);
    FF(c, d, a, b, x[6], S[6], K[6]);
    FF(b, c, d, a, x[7], S[7], K[7]);
    FF(a, b, c, d, x[8], S[8], K[8]);
    FF(d, a, b, c, x[9], S[9], K[9]);
    FF(c, d, a, b, x[10], S[10], K[10]);
    FF(b, c, d, a, x[11], S[11], K[11]);
    FF(a, b, c, d, x[12], S[12], K[12]);
    FF(d, a, b, c, x[13], S[13], K[13]);
    FF(c, d, a, b, x[14], S[14], K[14]);
    FF(b, c, d, a, x[15], S[15], K[15]);

    // sec
    GG(a, b, c, d, x[1], S[16], K[16]);
    GG(d, a, b, c, x[6], S[17], K[17]);
    GG(c, d, a, b, x[11], S[18], K[18]);
    GG(b, c, d, a, x[0], S[19], K[19]);
    GG(a, b, c, d, x[5], S[20], K[20]);
    GG(d, a, b, c, x[10], S[21], K[21]);
    GG(c, d, a, b, x[15], S[22], K[22]);
    GG(b, c, d, a, x[4], S[23], K[23]);
    GG(a, b, c, d, x[9], S[24], K[24]);
    GG(d, a, b, c, x[14], S[25], K[25]);
    GG(c, d, a, b, x[3], S[26], K[26]);
    GG(b, c, d, a, x[8], S[27], K[27]);
    GG(a, b, c, d, x[13], S[28], K[28]);
    GG(d, a, b, c, x[2], S[29], K[29]);
    GG(c, d, a, b, x[7], S[30], K[30]);
    GG(b, c, d, a, x[12], S[31], K[31]);

    // Third
    HH(a, b, c, d, x[5], S[32], K[32]);
    HH(d, a, b, c, x[8], S[33], K[33]);
    HH(c, d, a, b, x[11], S[34], K[34]);
    HH(b, c, d, a, x[14], S[35], K[35]);
    HH(a, b, c, d, x[1], S[36], K[36]);
    HH(d, a, b, c, x[4], S[37], K[37]);
    HH(c, d, a, b, x[7], S[38], K[38]);
    HH(b, c, d, a, x[10], S[39], K[39]);
    HH(a, b, c, d, x[13], S[40], K[40]);
    HH(d, a, b, c, x[0], S[41], K[41]);
    HH(c, d, a, b, x[3], S[42], K[42]);
    HH(b, c, d, a, x[6], S[43], K[43]);
    HH(a, b, c, d, x[9], S[44], K[44]);
    HH(d, a, b, c, x[12], S[45], K[45]);
    HH(c, d, a, b, x[15], S[46], K[46]);
    HH(b, c, d, a, x[2], S[47], K[47]);

    // Fourth
    II(a, b, c, d, x[0], S[48], K[48]);
    II(d, a, b, c, x[7], S[49], K[49]);
    II(c, d, a, b, x[14], S[50], K[50]);
    II(b, c, d, a, x[5], S[51], K[51]);
    II(a, b, c, d, x[12], S[52], K[52]);
    II(d, a, b, c, x[3], S[53], K[53]);
    II(c, d, a, b, x[10], S[54], K[54]);
    II(b, c, d, a, x[1], S[55], K[55]);
    II(a, b, c, d, x[8], S[56], K[56]);
    II(d, a, b, c, x[15], S[57], K[57]);
    II(c, d, a, b, x[6], S[58], K[58]);
    II(b, c, d, a, x[13], S[59], K[59]);
    II(a, b, c, d, x[4], S[60], K[60]);
    II(d, a, b, c, x[11], S[61], K[61]);
    II(c, d, a, b, x[2], S[62], K[62]);
    II(b, c, d, a, x[9], S[63], K[63]);

    m_uState[0] += a;
    m_uState[1] += b;
    m_uState[2] += c;
    m_uState[3] += d;
}

// Fill the data and process the final block
void GenMD5::finalize() {
    uint8_t bits[8] = {0};
    uint64_t bitCount = m_uCount * 8;  // convert to bit

    // Save original length (little-endian)
    for (int i = 0; i < 8; ++i) {
        bits[i] = (uint8_t)(bitCount >> (i * 8));
    }

    // fill 1 bit
    uint8_t padding = 0x80;
    updateValue(&padding, 1);

    // Pad with 0 bits until the length satisfies mod 56 bytes
    padding = 0;
    while ((m_uCount % 64) != 56) {
        updateValue(&padding, 1);
    }

    // add data leng
    updateValue(bits, 8);
}

void GenMD5::initsState()
{
    m_uState[0] = 0x67452301;
    m_uState[1] = 0xefcdab89;
    m_uState[2] = 0x98badcfe;
    m_uState[3] = 0x10325476;
    m_uCount = 0;
}

void GenMD5::updateValue(const uint8_t* input, size_t length)
{
    if (!input || !length)
        return;

    size_t index = m_uCount % 64;
    m_uCount += length;

    // todo cur first half
    if (index) {
        size_t partLen = 64 - index;
        if (length >= partLen) {
            memcpy(&m_uBuffer[index], input, partLen);
            transform(m_uBuffer);
            input += partLen;
            length -= partLen;
        }
        else {
            memcpy(&m_uBuffer[index], input, length);
            return;
        }
    }

    // todo 64
    while (length >= 64) {
        transform(input);
        input += 64;
        length -= 64;
    }

    // save last data
    if (length) {
        memcpy(m_uBuffer, input, length);
    }
}

void GenMD5::updateValue(const std::string& str)
{
    updateValue((const uint8_t*)str.c_str(), str.length());
}

std::string GenMD5::finalResult()
{
    finalize();
    // state change hex
    uint8_t digest[16] = {0};
    for (int i = 0; i < 4; ++i) {
        digest[i * 4] = m_uState[i] & 0xFF;
        digest[i * 4 + 1] = (m_uState[i] >> 8) & 0xFF;
        digest[i * 4 + 2] = (m_uState[i] >> 16) & 0xFF;
        digest[i * 4 + 3] = (m_uState[i] >> 24) & 0xFF;
    }

    // change hex
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    // reset status
    initsState();

    return oss.str();
}

std::string GenMD5::calcFileMd5(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) 
        return "";

    GenMD5 md5;
    char buffer[4096] = { 0 };

    while (file.read(buffer, sizeof(buffer))) 
        md5.updateValue((const uint8_t*)buffer, (size_t)file.gcount());

    md5.updateValue((const uint8_t*)buffer, (size_t)file.gcount());

    return md5.finalResult();
}

std::string GenMD5::calcStringMd5(const std::string& str)
{
    GenMD5 md5;
    md5.updateValue(str);
    return md5.finalResult();
}

std::string GenMD5::calcMemDataMd5(const void* pData, uint32_t uSize)
{
    GenMD5 md5;
    md5.updateValue((const uint8_t*)pData, uSize);
    return md5.finalResult();
}