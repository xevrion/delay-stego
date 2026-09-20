#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static const double SHORT = 0.005;
static const double LONG = 0.015;
static const double THRESHOLD = (SHORT + LONG) / 2;
static const int HEADER_BITS = 16;

using clk = std::chrono::steady_clock;

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    timeval tv{5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Listening on 127.0.0.1:9999 ...\n");

    std::map<uint32_t, double> seen;
    long total_bits = -1;
    auto t0 = clk::now();

    auto secs = [&]() {
        return std::chrono::duration<double>(clk::now() - t0).count();
    };
    auto ordered = [&]() {
        std::vector<double> v;
        for (auto &kv : seen) v.push_back(kv.second);
        return v;
    };
    auto gaps_to_bits = [&](const std::vector<double> &m, int from, int count) {
        std::vector<int> b;
        for (int i = from + 1; i <= from + count && i < (int)m.size(); ++i)
            b.push_back(m[i] - m[i - 1] > THRESHOLD ? 1 : 0);
        return b;
    };

    while (true) {
        uint32_t net;
        ssize_t n = recv(sock, &net, sizeof(net), 0);
        if (n == sizeof(net)) {
            uint32_t seq = ntohl(net);
            if (!seen.count(seq)) seen[seq] = secs();

            if (total_bits < 0 && (long)seen.size() - 1 >= HEADER_BITS) {
                auto m = ordered();
                auto hdr = gaps_to_bits(m, 0, HEADER_BITS);
                int len = 0;
                for (int bit : hdr) len = (len << 1) | bit;
                total_bits = HEADER_BITS + len * 8;
            }
            if (total_bits >= 0 && (long)seen.size() >= total_bits + 1) break;
        } else {
            break;  // timeout: sender finished or never started
        }
    }

    auto m = ordered();
    std::vector<int> bits;
    for (size_t i = 1; i < m.size(); ++i)
        bits.push_back(m[i] - m[i - 1] > THRESHOLD ? 1 : 0);

    std::string out;
    for (size_t i = HEADER_BITS; i + 8 <= bits.size(); i += 8) {
        int c = 0;
        for (int j = 0; j < 8; ++j) c = (c << 1) | bits[i + j];
        out.push_back(static_cast<char>(c));
    }

    printf("Received message: %s\n", out.c_str());
    close(sock);
    return 0;
}
