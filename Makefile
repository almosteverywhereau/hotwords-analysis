CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I. -I./cppjieba
TARGET = hotwords
DEMO_TARGET = demo
SOURCE = hotwords.cpp
DEMO_SOURCE = demo.cpp

.PHONY: all clean run demo test

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

demo: $(DEMO_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(DEMO_TARGET) $(DEMO_SOURCE)

run: $(TARGET)
	./$(TARGET) input1.txt hotwords_output.txt 600

run-demo: demo
	./$(DEMO_TARGET)

test: $(TARGET)
	./$(TARGET) input1.txt output_5min.txt 300
	./$(TARGET) input1.txt output_10min.txt 600
	./$(TARGET) input1.txt output_20min.txt 1200

clean:
	rm -f $(TARGET) $(DEMO_TARGET) *.o

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
