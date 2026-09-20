import time
from scapy.all import *

for i in range(5):
    pkt = IP(dst="127.0.0.1") / UDP(dport=9999) / Raw(load=f"packet {i}")
    send(pkt, verbose=False)
    print(f"Send packet {i} at {time.time():.4f}")
    time.sleep(0.5)