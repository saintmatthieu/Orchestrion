import pianissimo as pp

# pianissimo contains vectors `t` and `y`. `t` has a resolution of 1kHz. Not every ms has a value, though, e.g. it could be [0.000, 0.001, 0.005, ...]
# To each `t` value there is a corresponding `y` value.
# We want to create a 1kHz wav file from this data.

import numpy as np
import wave

t = (np.array(pp.t)*1000).astype(int)
t = t - t[0]
# allocate T, which contains all float samples
T = np.zeros((t[-1]+1,), dtype=np.float32)
# fill T with the values from y, interpolating the missing values
T[t] = pp.y
for i in range(1, len(T)):
    if T[i] == 0:
        T[i] = T[i-1]

# create a wave file
with wave.open('output.wav', 'wb') as w:
    w.setnchannels(1)  # mono
    w.setsampwidth(2)  # 16 bit
    w.setframerate(1000)  # 1kHz
    # convert to int16 and write to file
    w.writeframes((T * 32767).astype(np.int16).tobytes())