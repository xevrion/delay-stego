#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

static const auto SHORT = std::chrono::microseconds(5000);
static const auto LONG = std::chrono::microseconds(15000);

static std::vector<int> to_bits(const std::string &msg) {
    std::vector<int> bits;
    uint16_t len = static_cast<uint16_t>(msg.size());
    for (int i = 15; i >= 0; --i) bits.push_back((len >> i) & 1);
    for (unsigned char c : msg)
        for (int i = 7; i >= 0; --i) bits.push_back((c >> i) & 1);
    return bits;
}

int main(int argc, char **argv) {
    std::string message = argc > 1 ? argv[1] : "chishie fishie sussies quanti";
    std::vector<int> bits = to_bits(message);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr);

    auto emit = [&](uint32_t seq) {
        uint32_t net = htonl(seq);
        sendto(sock, &net, sizeof(net), 0,
               reinterpret_cast<sockaddr *>(&dst), sizeof(dst));
    };

    printf("Sending %zu bytes as %zu bits\n", message.size(), bits.size());
    emit(0);
    for (size_t i = 0; i < bits.size(); ++i) {
        std::this_thread::sleep_for(bits[i] ? LONG : SHORT);
        emit(static_cast<uint32_t>(i + 1));
    }
    printf("done\n");
    close(sock);
    return 0;
}
