#ifndef AES_CRYPT_H
#define AES_CRYPT_H

#include <string>
#include <openssl/evp.h>
#include <openssl/aes.h>

class aes_crypt
{
public:
    aes_crypt(const std::string &key);
    ~aes_crypt();

    std::string encrypt_to_base64(const std::string &text);
    std::string decrypt_from_base64(const std::string &cipher);

    unsigned char *encrypt(const unsigned char *plaintext, int *len);
    unsigned char *decrypt(const unsigned char *ciphertext, int *len);

public:
    static std::string to_hex(const unsigned char* data, int len);

protected:
    bool aes_init(const unsigned char *key_data, int key_data_len,
                  unsigned char *salt);
    void clear();

private:
    EVP_CIPHER_CTX e_enc_, e_dec_;
};

#endif // AES_CRYPT_H
