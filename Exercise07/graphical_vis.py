import matplotlib.pyplot as plt
import numpy as np

def graphical_visualization(operations):
    x = np.arange(1, 8)
    plt.grid()
    labels = ['100k', '500k', '1M', '5M', '10M', '25M', '50M']
    plt.plot(x, operations, 'b-')
    plt.xlabel('Number of Operations')
    plt.ylabel('Operations per Second')
    plt.title('Operations per second depending on total operations')
    plt.tick_params('y')
    plt.xticks(x, labels)
    plt.legend()
    plt.savefig('graphical_visualization.png')
    plt.show()



if __name__ == '__main__':
    operations = [1428571, 1428571, 1449275, 1519756, 1612903, 1628664, 1368738]
    graphical_visualization(operations)
