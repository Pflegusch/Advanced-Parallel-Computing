import matplotlib.pyplot as plt
import numpy as np

def graphical_visualization(no_numa, membind, interleave):
    x = np.arange(1, 11)
    plt.grid()
    labels = ['1', '2', '4', '8', '12', '16', '24', '32', '40', '48']
    plt.plot(x, no_numa, 'b-', label="No Numa")
    plt.plot(x, membind, 'r-', label="Numa membind")
    plt.plot(x, interleave, 'c-', label="Numa interleave")
    plt.xlabel('Number of Threads')
    plt.ylabel('Time in ms')
    plt.title('Different configurations of NUMA with different threads')
    plt.tick_params('y')
    plt.xticks(x, labels)
    plt.legend()
    plt.savefig('graphical_visualization.png')
    plt.show()



if __name__ == '__main__':
    no_numa = [0.000042, 0.000101, 0.000196, 0.000463, 0.000812, 0.000860, 0.001172, 0.001713, 0.002027, 0.002224]
    membind = [0.000033, 0.000088, 0.000131, 0.000230, 0.000632, 0.001203, 0.002172, 0.004723, 0.006034, 0.006224]
    interleave = [0.000040, 0.000123, 0.000230, 0.000434, 0.000845, 0.000923, 0.001349, 0.001693, 0.002453, 0.002563]
    graphical_visualization(no_numa, membind, interleave)
