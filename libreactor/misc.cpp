#include "libreactor/misc.h"
#include "libreactor/common.h"
#include <random>
#include <sstream>
#include <iomanip>

std::vector<FDPtr> fd_pair(Reactor& reactor) {
    int rawfds[2];
    socketpair(PF_UNIX, SOCK_STREAM, 0, rawfds);

    return {
        reactor.take_fd(rawfds[0]),
        reactor.take_fd(rawfds[1])
    };
}



void set_recv_buffer(FDPtr fd, int size) {
    int ret = setsockopt(fd->fileno(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    if(ret < 0)
        errno_to_exception();
}

const char* hex_digits = "0123456789abcdef";

#include "libreactor/logging.h"
std::string random_hex_string(int length) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 15);
    std::string out;
    out.resize(length);

    for (int i=0; i < length; i ++) {
        out[i] = hex_digits[dist(rd)];
    }

    return out;
}

int from_hex_digit(char d) {
    d = tolower(d);
    for (int i=0; i < 16; i ++)
        if (d == hex_digits[i])
            return i;

    throw std::runtime_error("bad hex digit");
}

std::string hex_decode(std::string s) {
    if (s.size() % 2 != 0) throw std::runtime_error("odd length hex string");

    std::string ret;
    ret.resize(s.size() / 2);
    for (int i=0; i < s.size(); i += 2) {
        int a = from_hex_digit(s[i]);
        int b = from_hex_digit(s[i + 1]);
        ret[i / 2] = (a << 8) | b;
    }
    return ret;
}

std::string hex_encode(std::string value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for(auto c: value) {
        escaped << std::setw(2) << int((unsigned char) c);
    }

    return escaped.str();
}
