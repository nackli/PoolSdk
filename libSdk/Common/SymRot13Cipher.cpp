#include "SymRot13Cipher.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
/*********************************************************************************************/

static inline std::string expandKey(const std::string& data, const std::string& key)
{
    std::string expandedKey;
    int keyLength = key.length();

    for (size_t i = 0; i < data.length(); i++) 
        expandedKey += key[i % keyLength];
    return expandedKey;
}


static inline char rot13(char c) 
{
    if (c >= 'a' && c <= 'z') 
        return 'a' + (c - 'a' + 13) % 26;
    else if (c >= 'A' && c <= 'Z') 
        return 'A' + (c - 'A' + 13) % 26;
    return c;
}


char rot13Decrypt(char c) {
    return rot13(c); 
}
/*********************************************************************************************/

SymRot13Cipher::SymRot13Cipher(const std::string& encryptionKey) : m_strKey(encryptionKey)
{
    if (m_strKey.empty()) {
        m_strKey = "defaultKey123";
    }
}

std::string SymRot13Cipher::encrypt(const std::string& plaintext)
{
    if (plaintext.empty()) return "";

    std::string ciphertext = plaintext;
    std::string expandedKey = expandKey(plaintext, m_strKey);


    for (char& c : ciphertext) 
        c = rot13(c);

    for (size_t i = 0; i < ciphertext.length(); i++) 
        ciphertext[i] = ciphertext[i] ^ expandedKey[i];

    std::reverse(ciphertext.begin(), ciphertext.end());

    return ciphertext;
}

std::string SymRot13Cipher::decrypt(const std::string& ciphertext) {
    if (ciphertext.empty()) return "";

    std::string text = ciphertext;
    std::reverse(text.begin(), text.end());
    std::string expandedKey = expandKey(text, m_strKey);
    for (size_t i = 0; i < text.length(); i++)
        text[i] = text[i] ^ expandedKey[i];

    for (char& c : text) 
        c = rot13Decrypt(c);

    return text;
}

std::string SymRot13Cipher::encryptAdvanced(const std::string& plaintext)
{
    if (plaintext.empty()) 
        return "";

    std::string ciphertext;
    std::string expandedKey = expandKey(plaintext, m_strKey);

    for (size_t i = 0; i < plaintext.length(); i++) 
    {
        char encryptedChar = plaintext[i];
        encryptedChar = encryptedChar ^ expandedKey[i];
        encryptedChar = rot13(encryptedChar);
        encryptedChar = encryptedChar ^ (expandedKey[i] + i) % 256;

        ciphertext += encryptedChar;
    }

    return ciphertext;
}
std::string SymRot13Cipher::decryptAdvanced(const std::string& ciphertext)
{
    if (ciphertext.empty()) 
        return "";

    std::string plaintext;
    std::string expandedKey = expandKey(ciphertext, m_strKey);

    for (size_t i = 0; i < ciphertext.length(); i++) 
    {
        char decryptedChar = ciphertext[i];

        // 反向多重变换
        decryptedChar = decryptedChar ^ (expandedKey[i] + i) % 256;
        decryptedChar = rot13Decrypt(decryptedChar);
        decryptedChar = decryptedChar ^ expandedKey[i];

        plaintext += decryptedChar;
    }
    return plaintext;
}

bool SymRot13Cipher::isKeyStrong(const std::string& key)
{
    if (key.length() < 8) 
        return false;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (char c : key) 
    {
        if (c >= 'A' && c <= 'Z') 
            hasUpper = true;
        else if (c >= 'a' && c <= 'z')
            hasLower = true;
        else if (c >= '0' && c <= '9') 
            hasDigit = true;
        else 
            hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit;
}

