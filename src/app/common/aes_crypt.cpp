#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include "aes_crypt.h"
#include "base64.h"

aes_crypt::aes_crypt(const std::string &key)
{
    unsigned int salt[] = {12345, 54321};

    const unsigned char *key_data = (const unsigned char *)key.c_str();
    int key_data_len = strlen(key.c_str());

    if (!aes_init(key_data, key_data_len, (unsigned char *)&salt)) {
        std::cout << "Couldn't initialize AES cipher" << std::endl;
    }
}

aes_crypt::~aes_crypt()
{
    clear();
}

std::string aes_crypt::encrypt_to_base64(const std::string &text)
{
    int c_len = text.size() + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = new unsigned char[c_len];

    EVP_EncryptInit_ex(&e_enc_, NULL, NULL, NULL, NULL);
    EVP_EncryptUpdate(&e_enc_, ciphertext, &c_len,
                      (unsigned char*)text.c_str(), text.size());
    EVP_EncryptFinal_ex(&e_enc_, ciphertext + c_len, &f_len);

    base64 base;
    std::istringstream in(std::string((const char*)ciphertext, c_len + f_len));
    std::ostringstream out(std::ostringstream::out);
    base.encode(in, out);

    delete[] ciphertext;

    return out.str();
}

std::string aes_crypt::decrypt_from_base64(const std::string &cipher)
{
    base64 base;
    std::istringstream in(cipher);
    std::ostringstream out;
    base.decode(in, out);

    std::string text = out.str();

    int p_len = text.size(), f_len = 0;
    unsigned char *plaintext = new unsigned char[p_len];

    EVP_DecryptInit_ex(&e_dec_, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(&e_dec_, plaintext, &p_len,
                      (unsigned char*)text.c_str(), text.size());
    EVP_DecryptFinal_ex(&e_dec_, plaintext + p_len, &f_len);

    std::string plain((const char*)plaintext, p_len + f_len);
    delete[] plaintext;
    return plain;
}

unsigned char *aes_crypt::encrypt(const unsigned char *plaintext, int *len)
{
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = new unsigned char[c_len];

    EVP_EncryptInit_ex(&e_enc_, NULL, NULL, NULL, NULL);
    EVP_EncryptUpdate(&e_enc_, ciphertext, &c_len, plaintext, *len);
    EVP_EncryptFinal_ex(&e_enc_, ciphertext+c_len, &f_len);

    *len = c_len + f_len;
    return ciphertext;
}

unsigned char *aes_crypt::decrypt(const unsigned char *ciphertext, int *len)
{
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = new unsigned char[p_len];

    EVP_DecryptInit_ex(&e_dec_, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(&e_dec_, plaintext, &p_len, ciphertext, *len);
    EVP_DecryptFinal_ex(&e_dec_, plaintext+p_len, &f_len);

    *len = p_len + f_len;
    return plaintext;
}

std::string aes_crypt::to_hex(const unsigned char *data, int len)
{
    std::stringstream sstream;
    for (int i = 0; i < len; i++) {
        sstream << std::setw(2) << std::setfill('0') << std::hex << (int)data[i];
    }
    std::string hex = sstream.str();
    boost::algorithm::to_upper(hex);
    return hex;
}

bool aes_crypt::aes_init(const unsigned char *key_data, int key_data_len,
                         unsigned char *salt)
{
    int i, nrounds = 5;
    unsigned char key[32], iv[32];

    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
    if (i != 32) {
        printf("Key size is %d bits - should be 256 bits\n", i);
        return false;
    }

    EVP_CIPHER_CTX_init(&e_enc_);
    EVP_EncryptInit_ex(&e_enc_, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(&e_dec_);
    EVP_DecryptInit_ex(&e_dec_, EVP_aes_256_cbc(), NULL, key, iv);

    return true;
}

void aes_crypt::clear()
{
    EVP_CIPHER_CTX_cleanup(&e_enc_);
    EVP_CIPHER_CTX_cleanup(&e_dec_);
}
