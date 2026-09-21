# delay-stego

what if the secret isn't *in* the message, it's in the *pauses*?

that's the whole idea here. two programs talk over the network, but the packets
they send are empty. no secret bytes anywhere. the message is hidden in the timing
between them: a short pause means 0, a long pause means 1. sniff the wire and all
you see is boring empty pings. the real message is in the rhythm.

it's like blinking morse code, except the blinks are network packets.

## see it

```sh
make
./demo.sh "meet me at midnight"
```

that runs both sides and prints the message back out, decoded purely from the gaps.

want two terminals instead:

```sh
./receiver                      # terminal 1, starts listening
./sender "meet me at midnight"  # terminal 2, sends
```

## the trick, in 4 lines

sender sleeps a little or a lot before each empty packet:

```cpp
sleep(bit ? 15ms : 5ms);   // long pause = 1, short pause = 0
send(empty_ping);          // the packet says nothing, the WAIT said everything
```

receiver just times the gaps and turns them back into bits. that's it. under 50
lines of c++ on each side.

## the catch

it works, but it's a toy, not a real spy tool. it's slow (~100 bits/sec, so a
sentence takes a few seconds), it breaks if the network gets jittery or drops a
packet, and honestly the timing pattern is easy to spot if someone's looking for
it. i wrote up the numbers i measured and everything that broke in
[notes.md](notes.md).
