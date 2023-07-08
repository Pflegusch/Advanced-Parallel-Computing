import matplotlib.pyplot as plt
import numpy as np

def graphical_visualization(pthread_mutex_updates, atomic_inc_updates, lock_rmw_updates):
    x = np.arange(1, 11)
    plt.grid()
    labels = ['1', '2', '4', '8', '12', '16', '24', '32', '40', '48']
    plt.plot(x, pthread_mutex_updates, 'b-', label="Pthread Mutex")
    plt.plot(x, atomic_inc_updates, 'r-', label="Atomic Increment")
    plt.plot(x, lock_rmw_updates, 'c-', label="Lock rmw")
    plt.xlabel('Number of Threads')
    plt.ylabel('Updates per second')
    plt.title('Shared counter performance analysis')
    plt.tick_params('y')
    plt.xticks(x, labels)
    plt.savefig('graphical_visualization.png')
    plt.show()



if __name__ == '__main__':
    pthread_mutex_updates = [25000000,2702703,1250000,1123596,909091,645161,471698,353357,306748,312500]
    atomic_inc_updates = [50000000,11111111,6666667,3571429,2380952,1754386,1204819,900901,684932,990099]
    lock_rmw_updates = [25000000,5882353,769231,314465,188679,112233,63694,42391,33036,17718]
    graphical_visualization(pthread_mutex_updates, atomic_inc_updates, lock_rmw_updates)
