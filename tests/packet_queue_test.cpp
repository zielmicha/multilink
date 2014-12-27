#include "packet_queue.h"
#include "logging.h"

int main() {
    PacketQueue queue(20);
    while(true) {
        if(!queue.push_back(Buffer::from_cstr("aaa"))) break;
    }
    queue.pop_front();
    assert(queue.push_back(Buffer::from_cstr("bbb")));
    assert(!queue.push_back(Buffer::from_cstr("bbb")));
    queue.pop_front();
    assert(!queue.push_back(Buffer::from_cstr("bbbb")));
    assert(queue.push_back(Buffer::from_cstr("bbb")));
}
