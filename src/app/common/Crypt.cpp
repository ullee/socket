#include <iostream>
#include <sstream>
#include <iomanip>

#include "Constants.h"
#include "Crypt.h"
#include "base64.h"

Crypt::Crypt(const uint32_t division)
{
    _division = division;
    set_key();

    const unsigned char *key_data = (const unsigned char *)_key.c_str();
    int key_data_len = strlen(_key.c_str());

    if (!init(key_data, key_data_len)) {
        std::cout << "Couldn't initialize AES cipher" << std::endl;
    }
}

Crypt::~Crypt()
{
    clear();
}

void Crypt::set_key()
{
    std::string env = getenv("APP_ENV");

    if (_division == SOCKET_CRYPT_KEY) {
        if (env.compare("production") == 0) {
            _key = "Td3Y3rbhyxKUJjoi/VRscY3Dg3x,abwE";
        } else if (env.compare("staging") == 0) {
            _key = "ideY32PhyCKUJ/oibVRsc[3Dg3x,FWwL";
        } else {
            _key = "ideY32PhyCKUJ/oibVRsc[3Dg3x,FWwL";
        }
    } else if (_division == WEB_SOCKET_CRYPT_KEY) {
        if (env.compare("production") == 0) {
            _key = "n6j3rY8g53FxJEzo2BobHL1VB4QBICDe";
        } else if (env.compare("staging") == 0) {
            _key = "19j3rH8g53FxJEQo2BobiL1VB4QBICDT";
        } else {
            _key = "19j3rH8g53FxJEQo2BobiL1VB4QBICDT";
        }
    } else {
        std::cout << "division does not exist" << std::endl;
    }
}

std::string Crypt::encrypt_base64(const std::string &text)
{
    int c_len = text.size() + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = new unsigned char[c_len];

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

std::string Crypt::decrypt_base64(const std::string &cipher)
{
    base64 base;
    std::istringstream in(cipher);
    std::ostringstream out;
    base.decode(in, out);

    std::string text = out.str();

    int p_len = text.size(), f_len = 0;
    unsigned char *plaintext = new unsigned char[p_len];

    EVP_DecryptUpdate(&e_dec_, plaintext, &p_len,
                      (unsigned char*)text.c_str(), text.size());
    EVP_DecryptFinal_ex(&e_dec_, plaintext + p_len, &f_len);

    std::string plain((const char*)plaintext, p_len + f_len);
    delete[] plaintext;
    return plain;
}

bool Crypt::init(const unsigned char *key, int key_data_len)
{
    unsigned char iv[AES_BLOCK_SIZE];
    std::string iv_temp(reinterpret_cast<char const *>(key), AES_BLOCK_SIZE);
    strcpy((char *)iv, iv_temp.c_str());
    
    EVP_CIPHER_CTX_init(&e_enc_);
    EVP_EncryptInit_ex(&e_enc_, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(&e_dec_);
    EVP_DecryptInit_ex(&e_dec_, EVP_aes_256_cbc(), NULL, key, iv);
    
    return true;
}

void Crypt::clear()
{
    EVP_CIPHER_CTX_cleanup(&e_enc_);
    EVP_CIPHER_CTX_cleanup(&e_dec_);
}
