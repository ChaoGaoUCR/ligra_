import matplotlib.pyplot as plt
import numpy as np

# 数据
methods = ["PK", "LJ", "SX", "wiki", "ttw"]
intersection_acc = [0.698, 0.481, 0.899, 0.471, 0.809]
union_acc = [0.939, 0.896, 0.982, 0.905, 0.951]
merge_acc = [0.975, 0.956, 0.996, 0.979, 0.984]

intersection_reduce = [0.2624570088, 0.2743484033, 0.3113243861, 0.3914327459, 0.215379072]
union_reduce = [0.2804607263, 0.3056103886, 0.3503111149, 0.4250716674, 0.2592107937]
merge_reduce = [0.3150384604, 0.3520917181, 0.3851398719, 0.4913110791, 0.2825892536]

# 设置柱状图位置和宽度
x = np.arange(len(methods))
width = 0.25

# 创建图形和子图
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10))

# 绘制 Accuracy 柱状图
ax1.bar(x - width, intersection_acc, width, label='Intersection', color='b')
ax1.bar(x, union_acc, width, label='Union', color='g')
ax1.bar(x + width, merge_acc, width, label='Merge', color='r')
ax1.set_ylabel('Accuracy')
ax1.set_title('Accuracy Comparison of Different Methods')
ax1.set_xticks(x)
ax1.set_xticklabels(methods)
ax1.legend()

# 绘制 Reduce Size 柱状图
ax2.bar(x - width, intersection_reduce, width, label='Intersection', color='b')
ax2.bar(x, union_reduce, width, label='Union', color='g')
ax2.bar(x + width, merge_reduce, width, label='Merge', color='r')
ax2.set_ylabel('Reduce Size')
ax2.set_title('Reduce Size Comparison of Different Methods')
ax2.set_xticks(x)
ax2.set_xticklabels(methods)
ax2.legend()

# 调整布局并保存为 PDF
plt.tight_layout()
plt.savefig('comparison_chart.pdf')
plt.show()