#pragma once
#include <string>
class SymRot13Cipher {
public:
    SymRot13Cipher(const std::string& encryptionKey);
    std::string encrypt(const std::string& plaintext);
    std::string decrypt(const std::string& ciphertext);
    std::string encryptAdvanced(const std::string& plaintext);
    std::string decryptAdvanced(const std::string& ciphertext);
    static bool isKeyStrong(const std::string& key);
private:
    std::string m_strKey;
};

