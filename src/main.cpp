#include <iostream>
#include <zlib.h>
#include <sstream>
#include <utility>
#include <string>
#include <vector>
#include <cstring>
#include <curl/curl.h>

#define CHUNK 32768

std::string decompress_string(const std::string &str)
{
    z_stream zs; // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
        throw(std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef *)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[CHUNK];
    std::string outstring;

    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (outstring.size() < zs.total_out)
        {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);
    if (ret != Z_STREAM_END)
    { // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") "
            << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

std::string base64_decode(const std::string &in)
{

    std::string out;

    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : in)
    {
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

auto main() -> int
{
    std::string test_str = "7Vjfk5owEP5reGwHiHj6qGLvnPEcR23vfIwQIW0gTggn3F/fRIL8Umtbrc5cn8x+2V2S/Xa/ATUwCJJHBjf+M3UR0UzdTTRga6Zp6JYufiSSZoj1ADLAY9hVTgUwx+8oj1RojF0UVRw5pYTjTRV0aBgih1cwyBjdVt3WlFSfuoEeagBzB5Im+oJd7mdoJ7+WxJ8Q9vz8yYaudgKYOysg8qFLtyUIDDUwYJTybBUkA0Rk8fK6ZHFfjuzuD8ZQyM8JSMfOujt17AVJkvhpMpkR+Pyppc7G0/zCyBX3VyZl3KceDSEZFmif0Th0kcyqC6vwGVO6EaAhwO+I81SRCWNOBeTzgKhdlGD+KsM/W8palnbsRGXeGWluhJylpSBpLst7RdjOyuOy+8lLHS2bgiIaMwedqFXefpB5iJ/wM/fkiqlANEDiPCKOIQI5fqueA6r29PZ+BYNioUj8DUJV3jdIYvWk2bBnLzWzTcSZ+ysmVp5cvfRGi9HkscF/ld2tjzmab+CuMFsx4lUm1cMQ4yg5Xd9mPVSA2VXzoQTCaCl7W4ybkc+QXxq1tn6lElo3mYnb97d5Zn8f4fPs/t6F9hiDaclhQ3HIo1LmqQRKbdKqtonZqandL/xzXT7mD6yT/mKRnbhoq/3V/7zT2v/V9+LdCW6pvmZDfb/1xiO7KbI+DVZx9E8EFhi1yTkgsJ0D+tq5lr4+fFB9Bfesr3X9a7VP66Vl/p3/XeoxaIzvoDddfJ0N7+4tCbRv/ZbUadRKb1RJ3JdXSwEJ9kKxdsS9EROArAoWn1s9tRFg180mHkX4Ha52qWSNVQOLvFZfs2yZSwx5lM27TB1xRn+gASVU5LVDGsosa0xIDboEG52apFpNNsABMsC1yOg2yDA+DBmgpix7+/JkCLP4Xs9Ep/jXAwx/Ag==";
    std::string decoded_str = base64_decode(test_str);
    std::string inflated_str = decompress_string(decoded_str);
    std::cout << "result: " << inflated_str << '\n';

    CURL *curl = curl_easy_init();
    if(curl) {
        char *decoded = curl_unescape(inflated_str.c_str(), inflated_str.length());
        if(decoded) {
            printf("Decoded: %s", decoded);
            curl_free(decoded);
        }
    }

    return 0;
}
