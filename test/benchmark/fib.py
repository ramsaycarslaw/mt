import time

def f(n):
    a, b = 0, 1
    for i in range(0,n):
        a, b = b, a + b
    return a

start = time.time()
answer = f(50);
elapsed = time.time() - start
print("Found answer {}".format(answer))
print("Elapsed {}".format(elapsed))
