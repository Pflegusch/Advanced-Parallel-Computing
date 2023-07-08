import matplotlib.pyplot as plt
import numpy as np

def graphical_visualization(pthread_mutex_updates, atomic_inc_updates, lock_rmw_updates, mcs_lock):
    x = np.arange(1, 11)
    plt.grid()
    labels = ['1', '2', '4', '8', '12', '16', '24', '32', '40', '48']
    plt.plot(x, pthread_mutex_updates, 'b-', label="Pthread Mutex")
    plt.plot(x, atomic_inc_updates, 'r-', label="Atomic Increment")
    plt.plot(x, lock_rmw_updates, 'c-', label="Lock rmw")
    plt.plot(x, mcs_lock, 'y-', label="Queue Lock")
    plt.xlabel('Number of Threads')
    plt.ylabel('Updates per second')
    plt.title('Shared counter performance analysis')
    plt.tick_params('y')
    plt.xticks(x, labels)
    plt.legend()
    plt.savefig('graphical_visualization.png')
    plt.show()



if __name__ == '__main__':
    pthread_mutex_updates = [25000000,2941176,1315789,1162791,1020408,649351,492611,377358,346021,490196]
    atomic_inc_updates = [50000000,12500000,6666667,3571429,2380952,1754386,1204819,900901,684932,591716]
    lock_rmw_updates = [25000000,4545455,1639344,420168,181159,95329,42337,24102,16364,14190]
    mcs_lock = [20000000, 793651, 350877, 183150, 122549, 92507, 62267, 47304, 38670, 27647]
    graphical_visualization(pthread_mutex_updates, atomic_inc_updates, lock_rmw_updates, mcs_lock)
