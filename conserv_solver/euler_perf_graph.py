import numpy as np
import matplotlib.pyplot as plt
import json
from pprint import pprint

y1 = []
y2 = []
x = []
s = 100

for n in range(0,30):
    json_data=open('GPU_CLEULER_{sx}x{sx}.json'.format(sx=s))
    data = json.load(json_data)
    y1.append(data['average_timestep']*1000.0)
    json_data.close()
    
    json_data=open('GPU_GLEULER_{sx}x{sx}.json'.format(sx=s))
    data = json.load(json_data)
    y2.append(data['average_timestep']*1000.0)
    json_data.close()
    
    x.append(s)
    s+=100

plt.title('Euler OpenCL vs OpenGL Performance')
plt.ylabel('Time(ms)')
plt.xlabel('Grid size')
plt.plot(x,y1,'r-o', label='OpenCL')
plt.plot(x,y2,'b-o', label='OpenGL')
plt.legend()
plt.savefig('euler_perf_graph.png')