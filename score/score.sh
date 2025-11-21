#!/bin/bash

# 定义输出结果存放的文件夹名称
OUTPUT_DIR="my_output"

echo "This is a debug shell script. Results will be saved to '$OUTPUT_DIR/'"
echo "--------------------------------------------------------------------------------"

# 确保我们在脚本所在的目录下
cd "$(dirname "$0")"

# 创建存放输出结果的文件夹
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir "$OUTPUT_DIR"
else
    # echo "Directory $OUTPUT_DIR already exists."
    : # Do nothing
fi

L=1
R=20

for ((i = $L; i <= $R; i = i + 1))
do
    # 检查输入输出文件是否存在
    if [ ! -f "data/$i.in" ]; then
        echo "Input file data/$i.in not found, skipping TEST $i"
        continue
    fi
    if [ ! -f "data/$i.out" ]; then
        echo "Output file data/$i.out not found, skipping TEST $i"
        continue
    fi

    # 运行程序
    ../build/code << EOF > scm.out
$(cat data/$i.in)
(exit)
EOF

    # === 数据清洗 ===
    
    # 1. 删除最后一行 (exit 提示)
    sed '$d' scm.out > scm_cleaned.out
    mv scm_cleaned.out scm.out
    
    # 2. 删除 'scm> ' 提示符
    sed 's/scm> //' scm.out > scm_cleaned.out
    mv scm_cleaned.out scm.out

    # 3. 【新增】删除空行 (包括只含空格的行)
    # /^[[:space:]]*$/d 表示：匹配 从行首(^)到行尾($)中间全是空白字符([[:space:]]*) 的行，并删除(d)
    sed '/^[[:space:]]*$/d' scm.out > scm_cleaned.out
    mv scm_cleaned.out scm.out

    # === 保存你的输出 ===
    cp scm.out "$OUTPUT_DIR/$i.out"

    # === 对比结果 ===
    # diff -b 会忽略行尾空格和空白数量的差异，但不会忽略空行，所以上面的 sed 很有必要
    # 如果标准答案 data/$i.out 里也有不规范的空行，建议把下面这行改成：
    # diff -b -B "$OUTPUT_DIR/$i.out" "data/$i.out" > "$OUTPUT_DIR/$i.diff"
    # (-B 选项告诉 diff 忽略空行的增加或减少，双重保险)
    
    diff -bB "$OUTPUT_DIR/$i.out" "data/$i.out" > "$OUTPUT_DIR/$i.diff"
    
    if [ $? -ne 0 ]; then
        # 失败 - 红色文字
        echo -e "\033[31m[FAIL] TEST $i\033[0m"
        echo "    -> Your output saved to: $OUTPUT_DIR/$i.out"
        echo "    -> Diff details saved to: $OUTPUT_DIR/$i.diff"
    else
        # 成功 - 绿色文字
        echo -e "\033[32m[PASS] TEST $i\033[0m"
        rm "$OUTPUT_DIR/$i.diff"
    fi
done

echo "--------------------------------------------------------------------------------"
echo "Done."