#include "streamutil.h"
#include <memory>

Future<unit> write_all(Reactor& reactor, Stream* out, Buffer data) {
    std::shared_ptr<Buffer> datap = std::make_shared<Buffer>(data);
    Future<unit> future;

    auto write_ready = [future, datap, out]() {
        if(datap->size == 0) return;

        size_t len = out->write(*datap);
        *datap = datap->slice(len);

        if(datap->size == 0)
            future.result(unit());
    };

    out->set_on_write_ready(write_ready);

    reactor.schedule(write_ready);

    return future;
}

void read_all(Stream* in, std::function<void(Buffer)> callback) {
    in->set_on_read_ready([callback, in]() {
        while(true) {
            StackBuffer<1024> buff;
            Buffer data = in->read(buff);
            callback(data);
            if(data.size == 0) break;
        }
    });
}
