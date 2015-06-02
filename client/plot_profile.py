import json
import sys
from math import ceil
import numpy as np

def get_bigger_index(data, V):
    return min(xrange(len(data)),
               key=lambda i: (data[i] < V, i))

def get_delta(data, timeseries):
    delta = [0]
    for i in xrange(len(data) - 1):
        dx = data[i + 1] - data[i]
        dt = timeseries[i + 1] - timeseries[i]
        delta.append(dx / dt)

    return delta

def replicate(data, timeseries, interval):
    buckets = [0] * int(ceil(max(timeseries) / interval))
    ntimeseries = [i * interval for i in xrange(len(buckets))]

    iter = 0

    timeseries = timeseries + [float('inf')]

    for i in xrange(len(buckets)):
        t0 = ntimeseries[i]
        while t0 > timeseries[iter + 1]:
            iter += 1
        buckets[i] = data[iter]

    return buckets, ntimeseries

def add_array(a, b):
    if len(a) < len(b):
        a, b = b, a
    n = list(a)
    for i, v in enumerate(b):
        n[i] += v
    return n

def moving_average(data, n=10):
    a = np.array(data)
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return list(ret[n - 1:] / n)

if __name__ == '__main__':
    all_data = []
    all_timeseries = []
    series_count = 0

    for raw_data in sys.stdin:
        series_count += 1
        zipped_data = [(0, 0)] + json.loads(raw_data)
        timeseries = [entry[0] for entry in zipped_data]
        data = [entry[1] for entry in zipped_data]

        data, timeseries = replicate(data, timeseries, 0.001)

        all_data = add_array(all_data, data)
        all_timeseries = max(timeseries, all_timeseries, key=len)

    all_data = [
        float(v) / series_count / 1024 / 1024
        for v in all_data
    ]
    sec1 = get_bigger_index(all_timeseries, 1)
    limit = len(all_timeseries) - 200
    #limit = sec1

    delta = get_delta(all_data, all_timeseries)
    delta = moving_average(delta, n=100)

    import pylab
    pylab.plot(all_timeseries[:limit],
               delta[:limit])
    pylab.xscale('log')
    pylab.show()
