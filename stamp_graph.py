import numpy as np
import os
import statistics
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# Results manually calculated in excel
results = {
    "vacation-low+": [
        0.251217347,
        0.4419044798,
        0.8611607941,
        1.729522737,
        2.517663227,
        1.81245443],
    "vacation-high+": [
        0.2523732295,
        0.4440968312,
        0.8410099013,
        1.685208104,
        2.194992856,
        1.435810908
    ],
    "labyrinth+": [
        0.9574982339,
        1.015620227,
        1.201923328,
        1.389577166,
        1.262516887,
        0.8559177589
    ],
    "ssca2": [
        0.400028285,
        0.7549429531,
        1.382996917,
        2.578383405,
        3.361073882,
        2.658138206
    ]
}


def create_individual_plot():
    fig, axs = plt.subplots(2, 2)
    plot = 0

    for name, result in results.items():
        ax = axs[plot // 2, plot % 2]
        plot += 1
        num_threads = [1, 2, 4, 8, 16, 32]
        ax.plot(num_threads, result, marker='o')
        ax.set_xlabel('Threads')
        ax.set_ylabel('Speedup')
        ax.set_title(f'TL2 on {name}')
        ax.yaxis.set_major_locator(plt.MaxNLocator(5))
        ax.xaxis.set_ticks(num_threads)

    fig.set_size_inches(9.5, 6)
    plt.tight_layout()
    fig.savefig(f'stamp_graphs.png', dpi=300)



if __name__ == '__main__':
    create_individual_plot()
