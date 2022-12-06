import os
import time
import statistics
import subprocess
from collections import OrderedDict
import pickle

def individual_test(trials, build, num_threads, t, c, k):
    # Each plot will get its own output directory, determined by 
    # workload, not build type or number of threads
    out_dir = f'benchmarks/{t}_{c}_{k}'

    for n in num_threads:
        build_out = f'{build}_{n}.txt'
        # Clear the file for this build type
        if os.path.exists(f'{out_dir}/{build_out}'):
            os.remove(f'{out_dir}/{build_out}')
        for _ in range(trials):
            cmd = [f'./{build}', '-n', n, '-k', k, '-c', c, '-t', t, '-o', f'../{out_dir}/{build_out}']
            print('running cmd', cmd)
            subprocess.run(cmd, cwd='build')


def test_barrage():
    trials = 5
    # The N x N x ... tests
    builds = ['bench']
    num_threads = ['1', '2', '4', '8', '16', '20', '32']
    types = ['rb', 'hash']
    configs = ['read', 'mixed']
    key_range = ['small', 'large']
    for t in types:
        for c in configs:
            for k in key_range:
                out_dir = f'benchmarks/{t}_{c}_{k}'
                if not os.path.exists(out_dir):
                    os.makedirs(out_dir)
                # Each plot will get its own output directory
                for build in builds:
                    individual_test(trials, build, num_threads, t, c, k)


if __name__ == '__main__':
    test_barrage()