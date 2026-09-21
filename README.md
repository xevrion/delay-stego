# delay-stego

hiding a message in the *gaps between packets* instead of inside the packets. the
packets themselves carry nothing secret, they're just empty pings with a sequence
number, so if you sniff the wire you see a boring stream of tiny udp datagrams and
none of them contain the message. the whole message lives in *when* they arrive.
short gap means 0, long gap means 1. that's the entire trick.

this is called a timing channel (or a covert timing channel) and it's a real thing
people use to smuggle data past monitoring that only looks at packet contents. i
wanted to see how small the actual code for one is, and it turns out you can do a
working one in like 50 lines of c++ per side.

## how it works

a normal covert channel hides bytes *in* the traffic, like stuffing bits into an
unused header field. a monitor that reads packet contents can eventually find that.
a timing channel hides the data in a thing most monitors don't even record: the
delay between one packet and the next.

sender side, the whole thing is this:

```cpp
static const auto SHORT = std::chrono::microseconds(5000);   // 5ms  gap = bit 0
static const auto LONG  = std::chrono::microseconds(15000);  // 15ms gap = bit 1

emit(0);                                  // first ping, starts the clock
for (size_t i = 0; i < bits.size(); ++i) {
    std::this_thread::sleep_for(bits[i] ? LONG : SHORT);
    emit(i + 1);                          // next ping, the GAP before it is the bit
}
```

so to send one bit you sleep for either 5ms or 15ms and then fire an empty packet.
n bits means n+1 packets, and the payload of every packet is just a sequence number
so the receiver can put them back in order (udp can reorder or drop, more on that
below).

receiver side does the mirror. it timestamps each packet as it lands, sorts them by
sequence number, then walks the timestamps and measures each gap. anything longer
than the midpoint (10ms) is a 1, anything shorter is a 0:

```cpp
static const double THRESHOLD = (SHORT + LONG) / 2;   // 10ms halfway point
bits.push_back(m[i] - m[i - 1] > THRESHOLD ? 1 : 0);
```

### the length header

the receiver doesn't know how long the message is up front, so the first 16 bits
are a length header (a uint16, big-endian) that says how many bytes follow. once
the receiver has decoded those 16 gaps it knows exactly how many more packets to
wait for, then it stops. after the header it just chops the remaining bits into
bytes, 8 at a time, and rebuilds the string.

### why udp and a sequence number

udp because i don't want tcp's own retransmit and congestion timers messing with my
gaps, i want to control the timing myself. but udp can deliver packets out of order,
so each packet carries a 4-byte sequence number and the receiver sorts on it before
measuring gaps. the packets are otherwise empty. that's the point, the bytes are
meaningless, only the schedule matters.

## usage

```sh
make                       # builds ./sender and ./receiver

# terminal 1: start listening first
./receiver

# terminal 2: send a message (default is a silly one if you omit it)
./sender "hello timing channel"
```

or just run the demo script which does both sides for you:

```sh
./demo.sh "your secret message here"
```

receiver prints `Received message: hello timing channel` and exits. both sides talk
over `127.0.0.1:9999` so this runs entirely on localhost, no real network needed to
play with it.

## limits

this is a learning toy, not a smuggling tool you'd actually trust. honest list:

| thing | reality |
| --- | --- |
| speed | ~100 bits/sec measured, so ~12 bytes/sec. a tweet takes ~20s |
| noise | tuned for loopback. real network jitter can blur 5ms vs 15ms and flip bits |
| loss | no retransmit. a dropped packet shifts every gap after it and corrupts the rest |
| secrecy | timing is *statistically* detectable, a monitor watching inter-packet delays would see two suspiciously clean clusters at 5ms and 15ms |
| crypto | none. it hides *that* there's a message poorly and doesn't encrypt the message at all, so encrypt first if you care |
| header | 16-bit length, so max message is 65535 bytes |

to make it real you'd want wider tolerance bands or many samples per bit, forward
error correction so one dropped packet doesn't wreck everything, randomized gaps
that still decode (to beat the "two clean clusters" tell), and encryption on top.
see notes.md for what i measured and what i'd try next.

## files

```
sender.cpp    encodes a string into inter-packet delays, fires empty udp pings
receiver.cpp  timestamps packets, measures gaps, decodes back to the string
Makefile      make, make clean
demo.sh       runs receiver + sender together for a quick round trip
notes.md      what i measured, what broke, what i'd change
```
