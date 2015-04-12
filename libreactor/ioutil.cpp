#include "ioutil.h"
#include "misc.h"
#include <memory>

namespace ioutil {

Future<Buffer> read(FD* fd, int size) {
    std::shared_ptr<int> pointer {new int};
    ImmediateCompleter<AllocBuffer, CastAsBuffer<AllocBuffer> > result {AllocBuffer(size)};
    *pointer = 0;

    fd->set_on_read_ready([=]() {
        while(true) {
            Buffer r = fd->read(result.value().as_buffer().slice(*pointer));
            if(r.size == 0) break;
            *pointer += r.size;
            if(*pointer == size) {
                fd->set_on_read_ready(nothing);
                result.result();
            }
        }
    });

    return result.future();
}

}
