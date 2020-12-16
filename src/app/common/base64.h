#ifndef BASE64_H
#define BASE64_H

#include <sstream>

class base64
{
private:
    static const char encode_character_table[];
    static const char decode_character_table[];
public:
    base64();
    ~base64();

    void encode(std::istream &in, std::ostringstream &out);
    void decode(std::istringstream &in, std::ostream &out);
};

#endif // BASE64_H
