from scapy.all import *

def handle(pkt):
    print(f"Received at {time.time():.4f} | payload: {pkt[Raw].load}")
    
import time
sniff(iface="lo", filter="udp port 9999", prn=handle, count=5)