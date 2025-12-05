# Makefile for Hot Words Analysis System
# 热词统计与分析系统编译脚本

CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I. -I./cppjieba
TARGET = hotwords
DEMO_TARGET = demo
SOURCE = hotwords.cpp
DEMO_SOURCE = demo.cpp

.PHONY: all clean run demo test

# 编译主程序
all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo "Compiling Hot Words Analysis System..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Build successful: $(TARGET)"

# 编译演示程序
demo: $(DEMO_SOURCE)
	@echo "Compiling demo program..."
	$(CXX) $(CXXFLAGS) -o $(DEMO_TARGET) $(DEMO_SOURCE)
	@echo "Build successful: $(DEMO_TARGET)"

# 运行主程序（使用input1.txt）
run: $(TARGET)
	@echo "Running Hot Words Analysis System..."
	./$(TARGET) input1.txt hotwords_output.txt 600

# 运行演示程序
run-demo: demo
	@echo "Running demo program..."
	./$(DEMO_TARGET)

# 测试不同窗口大小
test: $(TARGET)
	@echo "Testing with 5-minute window..."
	./$(TARGET) input1.txt output_5min.txt 300
	@echo "Testing with 10-minute window..."
	./$(TARGET) input1.txt output_10min.txt 600
	@echo "Testing with 20-minute window..."
	./$(TARGET) input1.txt output_20min.txt 1200

# 清理编译文件
clean:
	@echo "Cleaning up..."
	rm -f $(TARGET) $(DEMO_TARGET) *.o
	@echo "Clean completed."

# 帮助信息
help:
	@echo "Hot Words Analysis System - Makefile"
	@echo "Available targets:"
	@echo "  make           - 编译主程序"
	@echo "  make demo      - 编译演示程序"
	@echo "  make run       - 运行主程序（默认10分钟窗口）"
	@echo "  make run-demo  - 运行演示程序"
	@echo "  make test      - 测试不同窗口大小"
	@echo "  make clean     - 清理编译文件"
	@echo "  make help      - 显示此帮助信息"
