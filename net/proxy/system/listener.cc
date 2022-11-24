#include "net/proxy/system/listener.h"

#include "base/logging.h"
#include "net/proxy/system/tcp-socket-stream.h"

namespace net {
namespace proxy {
namespace system {

Listener::Listener(
    const any_io_executor &executor,
    const Endpoint &endpoint,
    Handler &handler)
    : executor_(executor),
      tcp_acceptor_(executor, endpoint),
      handler_(handler) { accept(); }

void Listener::accept() {
    auto stream = std::make_unique<TcpSocketStream>(executor_);
    tcp::socket &socket = stream->socket();
    tcp_acceptor_.async_accept(
        socket,
        [this, stream = std::move(stream)](std::error_code ec) mutable {
            if (ec) {
                LOG(fatal) << "accept failed: " << ec;
                return;
            }
            Stream &stream_ref = *stream;
            handler_.handle_stream(
                stream_ref,
                [stream = std::move(stream)](std::error_code) {});
            accept();
        });
}

}  // namespace system
}  // namespace proxy
}  // namespace net
