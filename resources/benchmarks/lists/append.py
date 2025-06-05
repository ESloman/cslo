import time
x = []
t = time.time()
for i in range(1000000):
    x.append(i)
print("append:", time.time() - t)
