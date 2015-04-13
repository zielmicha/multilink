#include "ioutil.h"
#include "misc.h"
#include <memory>

namespace ioutil {

    Future<Buffer> read(Stream* fd, int size) {
        std::shared_ptr<int> pointer = std::make_shared<int>(0);
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

    Future<unit> write(Stream* fd, Buffer data) {
        std::shared_ptr<AllocBuffer> buffer = std::make_shared<AllocBuffer>(
            AllocBuffer::copy(data));
        std::shared_ptr<int> pointer = std::make_shared<int>(0);
        Completer<unit> completer;

        fd->set_on_write_ready([=]() {
            while(true) {
                int wrote = fd->write(buffer->as_buffer().slice(*pointer));
                if(wrote == 0) break;
                *pointer += wrote;
                if(*pointer == buffer->as_buffer().size) {
                    fd->set_on_write_ready(nothing);
                    completer.result({});
                }
            }
        });

        return completer.future();
    }

}
