from sklearn import hmm
import numpy as np
import matplotlib.pyplot as plt

def plot_fhmm(model):
    X, Z = model.sample(100)
    plt.plot(X[:,0], X[:,1], "-o", label = "observations", ms = 6, alpha = 0.7)
    for i, m in enumerate(means):
        plt.text(m[0], m[1], 'Component %i' % (i+1),
        size=17, horizontalalignment = 'center',
        bbox=dict(alpha=.7, facecolor='w'))
    plt.legend(loc='best')
    plt.show

