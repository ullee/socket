#ifndef APP_SOCKET_CRYPT_H
#define APP_SOCKET_CRYPT_H

#include <string>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <boost/algorithm/string.hpp>

class Crypt
{
public:
    Crypt(const uint32_t division);
    ~Crypt();

    std::string encrypt_base64(const std::string &text);
    std::string decrypt_base64(const std::string &cipher);

protected:
    bool init(const unsigned char *key_data, int key_data_len);
    void set_key();
    void clear();

private:
    uint32_t _division;
    std::string _key;
    EVP_CIPHER_CTX e_enc_, e_dec_;
};

#endif // CRYPT_H
