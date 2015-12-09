
class HeaderPacketStream : public AbstractPacketStream, public std::enable_shared_from_this<HeaderPacketStream> {
    typedef std::function<Future<PacketStreamPtr>(Buffer)> Callback;
    Callback callback;
    PacketStreamPtr new_stream = nullptr;

public:
    HeaderPacketStream(Callback callback): callback(callback) {}

    void set_on_recv_ready(std::function<void()> f) {
        if (new_stream) new_stream->set_on_recv_ready(f);
        AbstractPacketStream::set_on_recv_ready(f);
    }

    void set_on_send_ready(std::function<void()> f) {
        if (new_stream) new_stream->set_on_send_ready(f);
        AbstractPacketStream::set_on_send_ready(f);
    }

    void set_on_error(std::function<void()> f) {
        if (new_stream) new_stream->set_on_error(f);
        AbstractPacketStream::set_on_error(f);
    }

    size_t get_send_offset() {
        return 0;
    }

    void close() { // FIXME: race condition
        if (new_stream)
            new_stream->close();
    }

    void send(Buffer data) {
        if (new_stream) {
            new_stream->send(data);
        } else {
            auto self = this->shared_from_this();
            callback(data).then([self](PacketStreamPtr stream) -> unit {
                stream->set_on_send_ready(self->on_send_ready);
                stream->set_on_recv_ready(self->on_recv_ready);
                stream->set_on_error(self->on_error);
                self->new_stream = stream;
                self->on_send_ready();
                self->on_recv_ready();
                return {};
            }).ignore();
            callback = nullptr;
        }
    }

    void send_with_offset(Buffer data) {
        send(data);
    }

    bool is_send_ready() {
        if (new_stream)
            return new_stream->is_send_ready();
        else
            return callback ? true : false;
    }

    optional<Buffer> recv() {
        if (new_stream)
            return new_stream->recv();
        else
            return optional<Buffer>();
    }
};


class ReadHeaderPacketStream : public PacketStream {
    ByteString header;
    PacketStreamPtr new_stream;
    bool header_recved = false;

public:
    ReadHeaderPacketStream(ByteString header, PacketStreamPtr new_stream): header(header), new_stream(new_stream) {}

    void set_on_recv_ready(std::function<void()> f) {
        new_stream->set_on_recv_ready(f);
    }

    void set_on_send_ready(std::function<void()> f) {
        new_stream->set_on_send_ready(f);
    }

    void set_on_error(std::function<void()> f) {
        new_stream->set_on_error(f);
    }

    size_t get_send_offset() {
        return new_stream->get_send_offset();
    }

    void close() { // FIXME: race condition
        if (new_stream)
            new_stream->close();
    }

    void send(Buffer data) {
        new_stream->send(data);
    }

    void send_with_offset(Buffer data) {
        return new_stream->send_with_offset(data);
    }

    bool is_send_ready() {
        return new_stream->is_send_ready();
    }

    optional<Buffer> recv() {
        if (!header_recved) {
            header_recved = true;
            return Buffer(header);
        } else {
            return new_stream->recv();
        }
    }
};
