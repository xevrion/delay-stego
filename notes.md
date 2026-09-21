# notes

learning log for delay-stego. what i measured, what bit me, what i'd try next.
first person, not documentation.

## the core idea in one line

don't hide the data in the packets, hide it in the *time between* packets. short
gap = 0, long gap = 1. the packets are empty pings, only their schedule carries the
message.

## measured numbers

all on loopback (127.0.0.1), gaps set to 5ms for 0 and 15ms for 1, threshold 10ms.

| message | bytes | bits (incl 16-bit header) | wall time | round trip |
| --- | --- | --- | --- | --- |
| "hi" | 2 | 32 | 0.25s | perfect |
| "hello timing channel" | 20 | 176 | 1.70s | perfect |
| long fox sentence | 74 | 608 | 5.93s | perfect |

so roughly 100 bits/sec, ~12 bytes/sec. the average gap is (5+15)/2 = 10ms which
predicts 100 bits/sec and that's basically what i got. makes sense, the time is
almost entirely the sleeps, the actual send is nothing.

empty string and single char both round trip fine, so the length header handles the
degenerate sizes (0 bytes = just the 16 header bits).

## what i assumed wrong

### i thought i'd need real timestamps synced between the two sides

nope. the receiver never needs the sender's clock. it only measures gaps *between
its own arrival timestamps*, so it's all relative. the two machines don't have to
agree on what time it is, they only have to agree on what "short" and "long" mean
(the 5/15ms constants and the 10ms threshold). that simplified everything.

### i thought udp reordering would be rare enough to ignore on loopback

mostly true on loopback but it's the wrong thing to lean on. the fix is cheap so i
did it anyway: every packet carries a 4-byte sequence number and the receiver sorts
by it (`std::map<uint32_t,double>`) before measuring any gaps. so even if packets
land out of order the gaps come out right. dropping a packet is the real killer, not
reordering, because a drop merges two gaps into one and shifts everything after it.

## the bug that cost time

the receiver has to know when to *stop*. first version just read forever and hung.
the symptom was the receiver sitting there after the sender clearly said "done",
never printing anything. the sender had finished, the receiver was still blocked in
recv() waiting for a packet that was never coming.

fixed it two ways together:
1. decode the 16-bit length header as soon as the first 16 gaps are in, which tells
   the receiver exactly how many total packets to expect, then break once they're
   all seen.
2. a 5-second `SO_RCVTIMEO` on the socket as a backstop, so if the sender never
   starts or dies mid-message the receiver gives up instead of hanging forever.

before the header logic i tried guessing the end from a long idle gap and it was
flaky, a slow packet looked like the end. the explicit length header is the clean
answer.

## why some non-obvious choices

- **udp not tcp:** tcp adds its own delays (nagle, delayed acks, retransmit,
  congestion control) and those would stomp all over the gaps i'm trying to control.
  udp lets me own the timing.
- **microsecond sleeps via `std::this_thread::sleep_for`:** simple and good enough
  on loopback. it's not hard-realtime, the os can oversleep, but 5 vs 15ms has a
  10ms margin so a little slop doesn't flip a bit. on a real network that margin is
  the thing that would need widening.
- **sequence number as the *only* payload:** the packets are deliberately empty of
  message data. the whole point is that a content-only sniffer sees nothing. so the
  4 bytes are just ordering, not information.
- **midpoint threshold instead of anything fancy:** with two fixed gap sizes the
  decision boundary is just the average. no need for clustering or calibration at
  this scale.

## to try later

- **wider bands + oversampling** so it survives real network jitter, decide a bit
  from several samples instead of one gap.
- **forward error correction** (even simple repetition or hamming) so one dropped
  packet doesn't corrupt the whole tail.
- **randomized gaps that still decode**, right now a monitor watching inter-packet
  timing sees two dead-clean clusters at 5ms and 15ms, which screams covert channel.
  spreading the delays around two means values with noise would hide the tell.
- **encrypt the message first**, this hides *that* a message exists (badly) but does
  nothing to hide the contents if someone does decode the timing.
- **run it across two real machines** and re-measure the table above, i expect the
  bit error rate to jump once there's actual jitter and queuing in the path.
