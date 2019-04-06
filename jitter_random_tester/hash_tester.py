#!/usr/bin/env python
# In support of evaluating JitterRandom, this python program evaluates the
# method of combining the TCNT1L values (8 bit values). I assume that these
# are randomly distributed, then check the randomness of the resulting values.

import argparse
import copy
import random
import statistics

import numpy as np
from scipy import stats

def mask32(v):
    return int(v) & 0xffffffff


def mask8(v):
    return int(v) & 0xff


def bits_subset(hashes, bit_offset, num_bits):
    mask = (1 << num_bits) - 1
    if bit_offset > 0:
        func = lambda v: (v >> bit_offset) & mask
    else:
        func = lambda v: v & mask
    return map(func, hashes)


def count_occurrences(hashes, num_buckets=None):
    # Turn an iterator into a list.
    if not isinstance(hashes, (list, tuple)):
        hashes = list(hashes)
    if not num_buckets:
        num_buckets = max(hashes) + 1
    buckets = [0] * num_buckets
    for h in hashes:
        buckets[h] += 1
    return buckets


def count_and_print_occurrences(hashes, num_buckets=None):
    buckets = count_occurrences(hashes, num_buckets=num_buckets)
    for i in range(len(buckets)):
        print('[%d] %6d' % (i, buckets[i]))
    return buckets


class Occurrences(object):
    """Compute the counts of unique values given a set of numbers."""
    def __init__(self, numbers):
        self.counts = count_occurrences(numbers)
        self.compute_stats()

    def add(self, other):
        # Sometimes of the counts is missing some values (i.e. if the highest
        # count wasn't produced by one of them).
        oc = other.counts
        if len(oc) > len(self.counts):
            self.counts.extend([0] * (len(oc) - len(self.counts)))
        for i in range(len(oc)):
            self.counts[i] += other.counts[i]
        self.compute_stats()

    def compute_stats(self):
        self.mean = statistics.mean(self.counts)
        self.median = statistics.median(self.counts)
        self.var = statistics.variance(self.counts) if len(self.counts) > 1 else 0
        self.spread = max(self.counts) - min(self.counts)
        self.spread_pct = 100 * self.spread / self.mean

    def print_summary(self):
        print('med=%d avg=%s var=%s p2p=%d p2p%%=%s' % (
            self.median, self.mean, self.var, self.spread, self.spread_pct))

    def print_count_table(self, num_per_row=16):
        count_strs = ["%d" % v for v in self.counts]
        max_digits = max(map(len, count_strs))
        num_rows = int((len(self.counts) + num_per_row - 1) / num_per_row)
        for r in range(num_rows):
            start = r * num_per_row
            print("[%2d]" % start, end='')
            for c in range(num_per_row):
                i = start + c
                if i >= len(self.counts):
                    break
                count_str = count_strs[i]
                print(" " * (max_digits - len(count_str)), count_str, end='')
            print()

    def print_chisquare(self):
        counts = np.array(self.counts)
        # print(counts)
        print(stats.chisquare(counts))
        # chi2_stat, p_val, dof, ex = stats.chi2_contingency(counts)
        # print("P-Value=%s" % p_val)


class HashEvaluator(object):
    def __init__(self, hash_func, name, random_bytes, bytes_per_hash=32):
        self.hash_func = hash_func
        self.name = name

        num_hashes = len(random_bytes) - bytes_per_hash + 1
        print('Computing %d hash values for %s' % (num_hashes, self.name), flush=True)
        self.hashes = [0] * num_hashes
        for i in range(num_hashes):
            data = random_bytes[i:i+bytes_per_hash]
            self.hashes[i] = hash_func(data)

        # Map from tuple (bit_offset, num_bits) to Occurrences
        self.bit_subset_counts = {}

        # Map from num_bits to combined Occurrences
        self.num_bits_counts = {}

    def occurrences_for_num_bits(self, num_bits):
        for bit_offset in range(33 - num_bits):
            self.eval_bit_subset(bit_offset, num_bits)
        return self.num_bits_counts[num_bits]

    def eval_bit_subset(self, bit_offset, num_bits):
        key = (bit_offset, num_bits)
        if key in self.bit_subset_counts:
            return self.bit_subset_counts[key]

        # Calculate the counts for various numbers of adjacent bits.
        numbers = bits_subset(self.hashes, bit_offset, num_bits)
        occurrences = Occurrences(numbers)
        self.bit_subset_counts[(bit_offset, num_bits)] = occurrences

        if num_bits not in self.num_bits_counts:
            self.num_bits_counts[num_bits] = copy.deepcopy(occurrences)
        else:
            self.num_bits_counts[num_bits].add(occurrences)
        return occurrences

#    def 




    # print('Counting occurrences of each value, for various subsets of the hashes')
    # for bits in range(1, 6):
    #     mask = (1 << bits) - 1
    #     jr_means = []
    #     djb2_means = []

    #     for bit in range(33 - bits):
    #         print('Counting %d bit occurrences at bit %d for JitterRandom (mask=0x%x)' % (
    #             bits, bit, mask))
    #         jr_buckets = count_and_print_occurrences(
    #             map(lambda v: ((v >> bit) & mask), jr_values))
    #         jr_means.append(statistics.mean(jr_buckets))






    # def count_occurrences(self, bit_offset, num_bits):
    #     hashes = bits_subset(self.hashes, bit_offset, num_bits)




    #     self.buckets = {}
    # def __init__(hashes, bit_offset, num_bits):
    #     hashes = bits_subset(hashes, bit_offset, num_bits)



    # print('Counting occurrences of each value, for various subsets of the hashes')
    # for bits in range(1, 6):
    #     mask = (1 << bits) - 1
    #     jr_means = []
    #     djb2_means = []

    #     for bit in range(33 - bits):
    #         print('Counting %d bit occurrences at bit %d for JitterRandom (mask=0x%x)' % (
    #             bits, bit, mask))
    #         jr_buckets = count_and_print_occurrences(
    #             map(lambda v: ((v >> bit) & mask), jr_values))
    #         jr_means.append(statistics.mean(jr_buckets))

    #         print('Counting %d bit occurrences at bit %d for DJB2 (mask=0x%x)' % (
    #             bits, bit, mask))
    #         djb2_buckets = count_and_print_occurrences(
    #             map(lambda v: ((v >> bit) & mask), djb2_values))
    #         djb2_means.append(statistics.mean(djb2_buckets))

    #     print()
    #     print('JitterRandom means:', jr_means)
    #     print('        DJB2 means:', djb2_means)
    #     print()







def jitter_random_hash(data):
    # Hash according to the JitterRandom approach.
    accumulator = 0
    for b in data:
        accumulator = mask32((accumulator << 8) ^ mask8(b))
    return accumulator


def djb2_hash(data):
    # Hash according to the Dan Bernstein function:
    accumulator = 0
    for b in data:
        accumulator = mask32((accumulator * 33) + mask8(b))
    return accumulator


def endolith_hash(data):
    # Hash according to the endolith gist (but 32 bits rather than 8 bits).
    accumulator = 0
    for b in data:
        # Rotate accumulator first:
        accumulator = mask32(accumulator << 1) + mask32(accumulator >> 31)
        accumulator = mask32(accumulator ^ mask8(b))
    return accumulator


def first_4bytes_hash(data):
    """Not a hash, just use the first 4 bytes.

    This acts as a point of comparison for the hashes above.
    """
    return mask32(data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3])


def bits_subset(hashes, bit_offset, num_bits):
    mask = (1 << num_bits) - 1
    if bit_offset > 0:
        func = lambda v: (v >> bit_offset) & mask
    else:
        func = lambda v: v & mask
    return map(func, hashes)


def generate_random_bytes(num_bytes):
    random_bytes = [0] * num_bytes
    for i in range(num_bytes):
        random_bytes[i] = random.getrandbits(8)
    return random_bytes


def main():
    parser = argparse.ArgumentParser(
        description='Test hashes of semi-random numbers.')
    parser.add_argument('integers', metavar='N', type=int, nargs='+',
                        help='an integer for the accumulator')
    parser.add_argument('--sum', dest='accumulate', action='store_const',
                        const=sum, default=max,
                        help='sum the integers (default: find the max)')



    bytes_per_hash = 32
    num_hashes = 100000
    num_bytes = num_hashes + bytes_per_hash - 1
    print('Generating %d random bytes' % num_bytes, flush=True)
    rand_bytes = generate_random_bytes(num_bytes)

    evaluator_configs = [
        (jitter_random_hash, "JitterRandom Hash"),
        (djb2_hash, "DJB2 Hash"),
        (endolith_hash, "Endolith Hash"),
        (first_4bytes_hash, "4RandomBytes"),
    ]

    # evaluators = [
    #     HashEvaluator(jitter_random_hash, "JitterRandom Hash", rand_bytes),
    #     HashEvaluator(djb2_hash, "DJB2 Hash", rand_bytes),
    #     HashEvaluator(endolith_hash, "Endolith Hash", rand_bytes),
    #     HashEvaluator(first_4bytes_hash, "4RandomBytes", rand_bytes),
    # ]
    # # jr_evaluator = 
    # # djb2_evaluator = HashEvaluator(djb2_hash, "DJB2", rand_bytes)

    for config in evaluator_configs:
        evaluator = HashEvaluator(config[0], config[1], rand_bytes, bytes_per_hash=bytes_per_hash)
        for num_bits in range(1, 6):
            print()
            print('#' * 80)
            print('Counting %d bit occurrences for %s' % (num_bits, evaluator.name))
            occurrences = evaluator.occurrences_for_num_bits(num_bits)
            occurrences.print_summary()
            occurrences.print_chisquare()
            print()
            occurrences.print_count_table()

    return






    jr_values = [0] * num_hashes
    djb2_values = [0] * num_hashes

    print('Computing %d hash values for each hash function' % len(jr_values))
    for i in range(num_hashes):
        data = rand_bytes[i:i+32]
        jr_values[i] = jitter_random_hash(data)
        djb2_values[i] = djb2_hash(data)

    print('Counting occurrences of each value, for various subsets of the hashes')
    for bits in range(1, 6):
        mask = (1 << bits) - 1
        jr_means = []
        djb2_means = []

        for bit in range(33 - bits):
            print('Counting %d bit occurrences at bit %d for JitterRandom (mask=0x%x)' % (
                bits, bit, mask))
            jr_buckets = count_and_print_occurrences(
                map(lambda v: ((v >> bit) & mask), jr_values))
            jr_means.append(statistics.mean(jr_buckets))

            print('Counting %d bit occurrences at bit %d for DJB2 (mask=0x%x)' % (
                bits, bit, mask))
            djb2_buckets = count_and_print_occurrences(
                map(lambda v: ((v >> bit) & mask), djb2_values))
            djb2_means.append(statistics.mean(djb2_buckets))

        print()
        print('JitterRandom means:', jr_means)
        print('        DJB2 means:', djb2_means)
        print()






if __name__ == "__main__":
    main()