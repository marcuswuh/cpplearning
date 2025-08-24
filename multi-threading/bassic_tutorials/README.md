# Overview
## 参考资料
//https://en.cppreference.com/w/cpp/thread/thread.html
## 目录说明
- bassis_


# clang-format
```bash
# 生成google风格的clang-format配置文件.
clang-format -style=google -dump-config > .clang-format
# 个人习惯：
## BreakBeforeBraces: Allman # 大括号风格选择Allman时，函数定义时/if/while/for花括号单独一行。
## IndentWidth: 4 # 设置缩进4个空格


# 检查并替换风格(指定单个文件)
clang-format -i ./src/main.cpp -style=file
# 结合其他工具实现批量检查
find . -name "*.cpp" -exec clang-format -i {} \;
```

# cppcheck
```
cppcheck --enable=all src/
```

# LICENSE





