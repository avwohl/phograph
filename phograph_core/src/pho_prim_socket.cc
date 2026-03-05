#include "pho_prim.h"
#include <cstring>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace pho {

// Simple handle table for open sockets
static constexpr int kMaxSockets = 64;
struct PhoSocket {
    int fd;
    bool active;
    int sockType; // SOCK_STREAM or SOCK_DGRAM
};
static PhoSocket gSockets[kMaxSockets] = {};

static int allocSocket(int fd, int sockType) {
    for (int i = 0; i < kMaxSockets; i++) {
        if (!gSockets[i].active) {
            gSockets[i] = {fd, true, sockType};
            return i;
        }
    }
    return -1;
}

static PhoSocket* getSocket(int handle) {
    if (handle < 0 || handle >= kMaxSockets) return nullptr;
    if (!gSockets[handle].active) return nullptr;
    return &gSockets[handle];
}

void register_socket_prims() {
    auto& r = PrimitiveRegistry::instance();

    // tcp-connect: host port -> handle
    r.register_prim("tcp-connect", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("tcp-connect: expected host string and port integer"));
        const char* host = in[0].as_string()->c_str();
        int port = (int)in[1].as_integer();

        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        char portStr[16];
        snprintf(portStr, sizeof(portStr), "%d", port);
        if (getaddrinfo(host, portStr, &hints, &res) != 0 || !res)
            return PrimResult::fail_with(Value::error("tcp-connect: DNS resolution failed"));

        int fd = socket(res->ai_family, SOCK_STREAM, 0);
        if (fd < 0) { freeaddrinfo(res); return PrimResult::fail_with(Value::error("tcp-connect: socket creation failed")); }

        if (connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
            close(fd); freeaddrinfo(res);
            return PrimResult::fail_with(Value::error("tcp-connect: connection failed"));
        }
        freeaddrinfo(res);

        int handle = allocSocket(fd, SOCK_STREAM);
        if (handle < 0) { close(fd); return PrimResult::fail_with(Value::error("tcp-connect: too many open sockets")); }
        return PrimResult::success(Value::integer(handle));
    });

    // tcp-listen: port -> handle
    r.register_prim("tcp-listen", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("tcp-listen: expected port integer"));
        int port = (int)in[0].as_integer();

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return PrimResult::fail_with(Value::error("tcp-listen: socket creation failed"));

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            close(fd); return PrimResult::fail_with(Value::error("tcp-listen: bind failed"));
        }
        if (listen(fd, 8) != 0) {
            close(fd); return PrimResult::fail_with(Value::error("tcp-listen: listen failed"));
        }

        int handle = allocSocket(fd, SOCK_STREAM);
        if (handle < 0) { close(fd); return PrimResult::fail_with(Value::error("tcp-listen: too many open sockets")); }
        return PrimResult::success(Value::integer(handle));
    });

    // tcp-accept: server-handle -> handle
    r.register_prim("tcp-accept", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("tcp-accept: expected handle"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::fail_with(Value::error("tcp-accept: invalid handle"));

        struct sockaddr_in client = {};
        socklen_t len = sizeof(client);
        int cfd = accept(s->fd, (struct sockaddr*)&client, &len);
        if (cfd < 0) return PrimResult::fail_with(Value::error("tcp-accept: accept failed"));

        int handle = allocSocket(cfd, SOCK_STREAM);
        if (handle < 0) { close(cfd); return PrimResult::fail_with(Value::error("tcp-accept: too many open sockets")); }
        return PrimResult::success(Value::integer(handle));
    });

    // tcp-send: handle data -> integer (bytes sent)
    r.register_prim("tcp-send", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("tcp-send: expected handle"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::fail_with(Value::error("tcp-send: invalid handle"));

        const void* data;
        size_t len;
        if (in[1].is_string()) {
            data = in[1].as_string()->c_str();
            len = in[1].as_string()->length();
        } else if (in[1].is_data()) {
            data = in[1].as_data()->bytes().data();
            len = in[1].as_data()->length();
        } else {
            return PrimResult::fail_with(Value::error("tcp-send: expected string or data"));
        }

        ssize_t sent = send(s->fd, data, len, 0);
        if (sent < 0) return PrimResult::fail_with(Value::error("tcp-send: send failed"));
        return PrimResult::success(Value::integer(sent));
    });

    // tcp-recv: handle max-bytes -> string
    r.register_prim("tcp-recv", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("tcp-recv: expected handle and max-bytes"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::fail_with(Value::error("tcp-recv: invalid handle"));

        int maxBytes = (int)in[1].as_integer();
        if (maxBytes <= 0 || maxBytes > 1048576)
            return PrimResult::fail_with(Value::error("tcp-recv: max-bytes must be 1..1048576"));

        std::vector<char> buf(maxBytes);
        ssize_t got = recv(s->fd, buf.data(), maxBytes, 0);
        if (got < 0) return PrimResult::fail_with(Value::error("tcp-recv: recv failed"));
        return PrimResult::success(Value::string(std::string(buf.data(), got)));
    });

    // tcp-close: handle -> boolean
    r.register_prim("tcp-close", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("tcp-close: expected handle"));
        int h = (int)in[0].as_integer();
        auto* s = getSocket(h);
        if (!s) return PrimResult::fail_with(Value::error("tcp-close: invalid handle"));
        close(s->fd);
        s->active = false;
        return PrimResult::success(Value::boolean(true));
    });

    // udp-create: -> handle
    r.register_prim("udp-create", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return PrimResult::fail_with(Value::error("udp-create: socket creation failed"));
        int handle = allocSocket(fd, SOCK_DGRAM);
        if (handle < 0) { close(fd); return PrimResult::fail_with(Value::error("udp-create: too many open sockets")); }
        return PrimResult::success(Value::integer(handle));
    });

    // udp-send: handle data host port -> integer (bytes sent)
    r.register_prim("udp-send", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[2].is_string() || !in[3].is_integer())
            return PrimResult::fail_with(Value::error("udp-send: expected handle, data, host, port"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::fail_with(Value::error("udp-send: invalid handle"));

        const void* data;
        size_t len;
        if (in[1].is_string()) { data = in[1].as_string()->c_str(); len = in[1].as_string()->length(); }
        else if (in[1].is_data()) { data = in[1].as_data()->bytes().data(); len = in[1].as_data()->length(); }
        else return PrimResult::fail_with(Value::error("udp-send: expected string or data"));

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons((int)in[3].as_integer());
        inet_pton(AF_INET, in[2].as_string()->c_str(), &addr.sin_addr);

        ssize_t sent = sendto(s->fd, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
        if (sent < 0) return PrimResult::fail_with(Value::error("udp-send: sendto failed"));
        return PrimResult::success(Value::integer(sent));
    });

    // udp-recv: handle max-bytes -> string
    r.register_prim("udp-recv", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("udp-recv: expected handle and max-bytes"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::fail_with(Value::error("udp-recv: invalid handle"));

        int maxBytes = (int)in[1].as_integer();
        std::vector<char> buf(maxBytes);
        ssize_t got = recvfrom(s->fd, buf.data(), maxBytes, 0, nullptr, nullptr);
        if (got < 0) return PrimResult::fail_with(Value::error("udp-recv: recvfrom failed"));
        return PrimResult::success(Value::string(std::string(buf.data(), got)));
    });

    // dns-lookup: hostname -> string (IP address)
    r.register_prim("dns-lookup", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("dns-lookup: expected string"));
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(in[0].as_string()->c_str(), nullptr, &hints, &res) != 0 || !res)
            return PrimResult::fail_with(Value::error("dns-lookup: resolution failed"));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ip, sizeof(ip));
        freeaddrinfo(res);
        return PrimResult::success(Value::string(ip));
    });

    // dns-reverse: ip -> string (hostname)
    r.register_prim("dns-reverse", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("dns-reverse: expected string"));
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, in[0].as_string()->c_str(), &addr.sin_addr);
        char host[NI_MAXHOST];
        if (getnameinfo((struct sockaddr*)&addr, sizeof(addr), host, sizeof(host), nullptr, 0, 0) != 0)
            return PrimResult::fail_with(Value::error("dns-reverse: lookup failed"));
        return PrimResult::success(Value::string(host));
    });

    // local-address: -> string
    r.register_prim("local-address", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != 0)
            return PrimResult::fail_with(Value::error("local-address: gethostname failed"));
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(hostname, nullptr, &hints, &res) != 0 || !res)
            return PrimResult::success(Value::string("127.0.0.1"));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ip, sizeof(ip));
        freeaddrinfo(res);
        return PrimResult::success(Value::string(ip));
    });

    // socket-status: handle -> string
    r.register_prim("socket-status", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("socket-status: expected handle"));
        auto* s = getSocket((int)in[0].as_integer());
        if (!s) return PrimResult::success(Value::string("closed"));
        // Check if socket is still connected
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error != 0) return PrimResult::success(Value::string("error"));
        return PrimResult::success(Value::string("connected"));
    });
}

} // namespace pho
