import os
import statistics
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
# Read stuff generated by run_benchmarks.py

def get_mean(path):
    with open(path, 'r') as file:
        v = [float(line.rstrip()) for line in file]
        mean = statistics.mean(v)
        return mean
    

def create_individual_plot(builds, t, c, k):
    fig, ax = plt.subplots()
    for build in builds:
        num_threads = [1, 2, 4, 8, 16, 20]
        out_dir = f'benchmarks/{t}_{c}_{k}'
        results = [get_mean(f'{out_dir}/{build}_{n}.txt') for n in num_threads]
        ax.plot(num_threads, results, label=build)
        plt.legend(loc="upper left")
        ax.set_xlabel('Threads')
        ax.set_ylabel('1000X op/sec')
        ax.set_title(f'{t}_{c}_{k}')
    fig.savefig(f'graphs/fig_{t}_{c}_{k}.png', dpi=200)


def generate_main_plots():
    builds = ['bench', 'mutex_bench', 'gcc_bench']
    types = ['rb', 'hash']
    configs = ['read', 'mixed']
    key_range = ['small', 'large']
    if not os.path.exists('graphs'):
        os.makedirs('graphs')
    for t in types:
        for c in configs:
            for k in key_range:
                # Each plot will get its own output directory
                create_individual_plot(builds, t, c, k)

if __name__ == '__main__':
    generate_main_plots()