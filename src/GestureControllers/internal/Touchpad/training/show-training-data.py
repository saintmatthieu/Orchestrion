import pianissimo as pp
import pianissimo_labels as ppl
import matplotlib.pyplot as plt
import numpy as np

t = np.array(pp.t) - pp.t[0]
dt = np.average(np.diff(t))
y = np.array(pp.y)
dy = np.abs(np.diff(y)) / dt

spans = ppl.on_spans
i_on = []
for i, v in enumerate(t):
    for span in spans:
        if v >= span[0] and v <= span[1]:
            i_on.append(i)
            break

# Find the threshold that minimizes the number of false positives and false negatives, using a binary search.
prev_lower = 0
prev_upper = np.max(dy)
threshold = prev_upper / 2
prev_error_count = len(dy)
while True:
    fpc = 0
    fnc = 0
    for i, v in enumerate(dy):
        if v > threshold:
            if i not in i_on:
                fpc += 1
        else:
            if i in i_on:
                fnc += 1
    error_count = fpc + fnc
    if error_count >= prev_error_count:
        break
    prev_error_count = error_count
    if fpc > fnc:
        prev_lower = threshold
        threshold = (threshold + prev_upper) / 2
    else:
        prev_upper = threshold
        threshold = (threshold + prev_lower) / 2

print(f'Optimal threshold: {threshold}, dt: {dt}')

j = [i for i, v in enumerate(dy) if v > threshold]

# Algorithm: at least 5 consecutive samples above the threshold means entering in swipe state.
# Then, the next sample below the threshold terminates the swipe gesture.
# We then look back for the first sample above the threshold, an take the difference between last and first
# as swipe amplitude.

swipe = False
swipe_start = 0
swipe_ends = []
swipe_amplitudes = []
for i, v in enumerate(dy):
    if v > threshold:
        if not swipe:
            swipe = True
            swipe_start = i
    else:
        if swipe and i - swipe_start >= 5:
            swipe_ends.append(i)
            swipe_amplitudes.append(np.abs(y[i] - y[swipe_start]))
        swipe = False

plt.subplot(2, 1, 1)
plt.plot(t, y)
plt.plot(t[i_on], y[i_on], 'o')
plt.plot(t[j], y[j], 'x')
# Add text labels for the swipe amplitudes
for i, v in enumerate(swipe_amplitudes):
    # Formatted string with 2 decimal places
    plt.text(t[swipe_ends[i]], y[swipe_ends[i]], f'{v:.2f}', fontsize=8, ha='center', va='bottom')
    # Add a big circle around the swipe amplitude
    plt.plot(t[swipe_ends[i]], y[swipe_ends[i]], 'o', markersize=10, color='orange')
plt.grid()

plt.subplot(2, 1, 2)
plt.plot(t[:-1], dy)
plt.plot(t[i_on], dy[i_on], 'o')
plt.plot(t[j], dy[j], 'x')
plt.axhline(threshold, color='red', linestyle='--')
plt.grid()

plt.show()
