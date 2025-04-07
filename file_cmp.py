#coding:utf-8


def compare_files(file1, file2):
    count = 0
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        lines1 = f1.readlines()
        lines2 = f2.readlines()

    # 使用ndiff函数逐行比较
    for index,line in enumerate(lines2):
        if(line != lines1[index]):
            count = count + 1
            print(line)
            print(lines2[index])
            print("---------------")

    # 输出差异
    print((float)(count)/len(lines2))


file1 = 'test.sam'
file2 = '../minimap2-fpga/test_ref.sam'

compare_files(file1, file2)