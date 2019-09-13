#include "rsocket.h"

RSocket::RSocket() {
    RSocket(true, 10);
}

RSocket::RSocket(bool isTcp, int secs) {
    err = "";
    this->isTcp = isTcp;
    this->isTimeout = false;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock<1) {
        setError("can not create a socket.");
        return;
    }

    setTimeout(secs);
    setNonBlocking();
}

bool RSocket::ok() {
    return (err.size() == 0);
}

const char *RSocket::getError() {
    return err.c_str();
}

void RSocket::setError(std::string msg) {
    if (ok())
        err = msg;
}

void RSocket::setNonBlocking() {
    int opt;

    if ((opt = fcntl (sock, F_GETFL, NULL)) < 0) {
        setError("F_GETFL error");
        return;
    }

    if (fcntl (sock, F_SETFL, opt | O_NONBLOCK) < 0) {
        setError("F_SETFL error");
        return;
    }
}

void RSocket::setTimeout(int secs) {
    tv.tv_usec = 0;
    tv.tv_sec = secs;
}

int RSocket::getSock() {
    return sock;
}

bool RSocket::timeout() {
    return isTimeout;
}

RSocket::~RSocket() {
    if (sock>=0) {
        close(sock);
        sock = -1;
     }
}

void RSocket::dial(std::string host, int port) {
    int r;

    struct hostent *h = gethostbyname(host.c_str());
    if (!h) {
        setError("Cannot resolve the remote host!");
        return;
    }

    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(port);
    endpoint.sin_addr = *(struct in_addr *)(h->h_addr);

    r = connect(sock, (struct sockaddr *)&endpoint, sizeof(endpoint));
    if (r<0) {
        if (errno == EINPROGRESS) {
            fd_set wait_set;
            FD_ZERO (&wait_set);
            FD_SET (sock, &wait_set);

            r = select(sock + 1, &wait_set, &wait_set, NULL, &tv);
            if (r <= 0) {
                setError("can not connect.");
                return;
            }
        } else {
            setError("can not connect");
            return;
        }
    }

    return;
}

void RSocket::serve(int port) {
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(port);
    endpoint.sin_addr.s_addr = INADDR_ANY;
    setError("not implemented");
    return;
}

void RSocket::shutdown() {
    if (sock >= 0) {
        close(sock);
        sock = -1;
     }
}

long int RSocket::push(char *data, size_t sz) {
    long int n;
    int s;
    setError("");

    fd_set wait_set;
    FD_ZERO (&wait_set);
    FD_SET (sock, &wait_set);
    s = select(sock + 1, NULL, &wait_set, NULL, &tv);
    if (s < 0) {
        setError("RSocket error");
        return 0;
    }
    if (s == 0) {
        isTimeout = true;
        return 0;
    }

    if (isTcp)
        n = send(sock, data, sz, 0);

    if (n < 0) {
        setError("cant sent data");
        return 0;
    }

    return n;
}


long int RSocket::pop(char *data, size_t sz) {
    long int n;
    int s;
    setError("");
    isTimeout = false;

    fd_set wait_set;
    FD_ZERO (&wait_set);
    FD_SET (sock, &wait_set);
    s = select(sock + 1, &wait_set, NULL,  NULL, &tv);
    if (s < 0) {
        setError("RSocket error");
        return 0;
    }
    if (s == 0) {
        isTimeout = true;
        return 0;
    }

    memset(data, 0, sz);

    if (isTcp)
        n = recv(sock, data, sz, 0);

    if (n < 0) {
        setError("cant receive data");
        return 0;
    }

    return n;
}
