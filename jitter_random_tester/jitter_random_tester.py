#!/usr/bin/env python
# In support of evaluating JitterRandom, this python program tests whether
# Walter Anderson's program (as I've implemented it) really makes use
# of more than 4 bytes of input. It appears to me that it doesn't, which
# is to say that the hash of the 32 TCNT1L values is really only of the 
# last 4 values.
#
# ANSWER: Yup, only the last 4 values are used. Sigh. So the algo on which
# Mr. Anderson based his program (https://gist.github.com/endolith/2568571),
# which used rotate-left rather than shift-left, made better use (any use
# at all actually) of the initial TCNT1L values.

import random


def generate_random_bytes(num_bytes):
    random_bytes = [0] * num_bytes
    for i in range(num_bytes):
        random_bytes[i] = random.getrandbits(8)
    return random_bytes


def mask32(v):
    return int(v) & 0xffffffff


def mask8(v):
    return int(v) & 0xff


def jitter_random_hash(data):
    # Hash according to the JitterRandom approach.
    accumulator = 0
    for b in data:
        accumulator = mask32((accumulator << 8) ^ mask8(b))
    return accumulator


def endolith_hash(data):
    # Hash according to the endolith gist (but 32 bits rather than 8 bits).
    accumulator = 0
    for b in data:
        # Rotate accumulator first:
        accumulator = mask32(accumulator << 1) + mask32(accumulator >> 31)
        accumulator = mask32(accumulator ^ mask8(b))
    return accumulator


def djb2_hash(data):
    # Hash according to the Dan Bernstein function:
    accumulator = 0
    for b in data:
        accumulator = mask32((accumulator * 33) + mask8(b))
    return accumulator


def main():
#    for _ in range()
    rand_bytes = generate_random_bytes(32)
    print('Based on https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library')
    for i in range(len(rand_bytes) - 3):
        print("[%2d] 0x%08x" % (i, jitter_random_hash(rand_bytes[i:])))
    print()

    print('Based on https://gist.github.com/endolith/2568571')
    for i in range(len(rand_bytes) - 3):
        print("[%2d] 0x%08x" % (i, endolith_hash(rand_bytes[i:])))
    print()

    print('Based on DJB2 hash function')
    for i in range(len(rand_bytes) - 3):
        print("[%2d] 0x%08x" % (i, djb2_hash(rand_bytes[i:])))



if __name__ == "__main__":
    main()