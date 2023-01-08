#include "net/proxy/misc/random-handler.h"

#include <openssl/rand.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include "absl/container/fixed_array.h"

namespace net {
namespace proxy {
namespace misc {
namespace {

class StreamConnection : public boost::intrusive_ref_counter<
    StreamConnection, boost::thread_unsafe_counter> {
public:
    explicit StreamConnection(std::unique_ptr<Stream> stream);

    void start() { read(); write(); }

private:
    void read();
    void write();
    void close() { stream_.reset(); }

    std::unique_ptr<Stream> stream_;
    absl::FixedArray<uint8_t, 0> read_buffer_;
    absl::FixedArray<uint8_t, 0> write_buffer_;
};

StreamConnection::StreamConnection(std::unique_ptr<Stream> stream)
    : stream_(std::move(stream)),
      read_buffer_(8192),
      write_buffer_(8192) {
    RAND_bytes(write_buffer_.data(), write_buffer_.size());
}

void StreamConnection::read() {
    if (!stream_) {
        return;
    }
    stream_->async_read_some(
        buffer(read_buffer_.data(), read_buffer_.size()),
        [connection = boost::intrusive_ptr<StreamConnection>(this)](
            std::error_code ec, size_t) {
            if (ec) {
                connection->close();
                return;
            }
            connection->read();
        });
}

void StreamConnection::write() {
    if (!stream_) {
        return;
    }
    stream_->async_write_some(
        const_buffer(write_buffer_.data(), write_buffer_.size()),
        [connection = boost::intrusive_ptr<StreamConnection>(this)](
            std::error_code ec, size_t size) {
            if (ec) {
                connection->close();
                return;
            }
            RAND_bytes(connection->write_buffer_.data(), size);
            connection->write();
        });
}

}  // namespace

void RandomHandler::handle_stream(std::unique_ptr<Stream> stream) {
    boost::intrusive_ptr<StreamConnection> connection(
        new StreamConnection(std::move(stream)));
    connection->start();
}

}  // namespace misc
}  // namespace proxy
}  // namespace net
