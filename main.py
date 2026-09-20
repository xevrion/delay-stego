import time
from scapy.all import IP, UDP, Raw, send

SHORT = 0.005
LONG = 0.015

def str_to_bits(s):
    bits = []
    for c in s:
        bits.extend(format(ord(c), '08b'))
    return bits

message = "chish qunati"
payload = message.encode()

header = format(len(payload), '016b')
body = ''.join(format(b, '08b') for b in payload)
bits = list(header + body)
print(f"Sending {len(payload)} bytes as {len(bits)} bits")

def emit(seq):
    pkt = IP(dst="127.0.0.1") / UDP(sport=40000, dport=9999) / Raw(load=f"{seq:06d}".encode())
    send(pkt, verbose=False)

emit(0)
for i, bit in enumerate(bits):
    time.sleep(SHORT if bit == '0' else LONG)
    emit(i + 1)
    print(f"sent marker for bit {bit} ({i + 1}/{len(bits)})")
