from scapy.all import sniff, Raw
import time

SHORT = 0.005
LONG = 0.015
THRESHOLD = (SHORT + LONG) / 2
HEADER_BITS = 16

seen = {}
total_bits = [None]

def bits_to_int(bits):
    return int(''.join(map(str, bits)), 2)

def handle(pkt):
    if not pkt.haslayer(Raw):
        return
    try:
        seq = int(bytes(pkt[Raw].load).decode())
    except ValueError:
        return
    seen.setdefault(seq, time.time())

    markers = [seen[s] for s in sorted(seen)]
    ngaps = len(markers) - 1
    if total_bits[0] is None and ngaps >= HEADER_BITS:
        hdr = [1 if markers[i] - markers[i - 1] > THRESHOLD else 0
               for i in range(1, HEADER_BITS + 1)]
        total_bits[0] = HEADER_BITS + bits_to_int(hdr) * 8

def done(pkt):
    return total_bits[0] is not None and len(seen) >= total_bits[0] + 1

sniff(iface="lo", filter="udp dst port 9999", prn=handle,
      stop_filter=done, timeout=300)

markers = [seen[s] for s in sorted(seen)]
bits = [1 if markers[i] - markers[i - 1] > THRESHOLD else 0
        for i in range(1, len(markers))]

body = bits[HEADER_BITS:]
chars = []
for i in range(0, len(body), 8):
    byte = body[i:i + 8]
    if len(byte) == 8:
        chars.append(chr(bits_to_int(byte)))

print(f"Received message: {''.join(chars)}")
