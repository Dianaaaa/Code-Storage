# Lab3实现功能：

## 一、 word ladder：

- 基本功能
- 支持输入不同长度的单词
- 支持输入不在词典中的单词
- 若没有Ladder则输出“There is no word ladder."

## 二、 N-grams

- 基本功能
- 选取第一个字母为大写的单词作为开头
- 若结尾的不是"!"、 "?"、 "."、 ";"，将继续产生单词，直到出现可作为结尾的单词（尽可能短）。
- 支持中文（必须是GBK格式的，否则输出乱码。最好使用windows编译运行）
- 可自动识别文档为中文还是英文，并调整输出的行宽。
- 中文版同样以完整句子作为结束。


附测试文件：平凡的世界.txt