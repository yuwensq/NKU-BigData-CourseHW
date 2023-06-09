# coding=utf-8
import matplotlib.pyplot as plt 
import io


items = []
x = []
y = []

with io.open('./longTailData.txt', encoding='utf-8') as f:
    lineNo = 0
    totalTimes = 0
    for line in f:
        x.append(lineNo)
        items.append(int(line.split()[0]))
        y.append(float(line.split()[1]))
        totalTimes = totalTimes + y[lineNo]
        lineNo = lineNo + 1
    # totalTimes = 19835
    # y = map(lambda x:x / totalTimes, y)

data = dict()
y2 = []
with io.open('./rmsePerItem.txt', encoding='utf-8') as f:
    for line in f:
        itemId, rmse = line.split()
        itemId = int(itemId)
        rmse = float(rmse)
        data[itemId] = rmse

for itemId in items:
    y2.append(data[itemId])

new_x = []
new_y = []
for i in range(0, len(x), 10):
    if y2[i] != 0:
        new_x.append(x[i])
        new_y.append(y2[i])
    
        

# plt.plot(x, y, 'ro')
# plt.plot(x, y2, 'b-')
plt.plot(new_x, new_y, 'bo')
plt.show()
