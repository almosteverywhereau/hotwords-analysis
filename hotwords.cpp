
#include "cppjieba/Jieba.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <cstdlib>


// 时间戳
struct Timestamp {
    int hours;
    int minutes;
    int seconds;
    
    Timestamp() : hours(0), minutes(0), seconds(0) {}
    Timestamp(int h, int m, int s) : hours(h), minutes(m), seconds(s) {}
    
   
    int toSeconds() const {
        return hours * 3600 + minutes * 60 + seconds;
    }
    

    static Timestamp fromSeconds(int totalSeconds) {
        int h = totalSeconds / 3600;
        int m = (totalSeconds % 3600) / 60;
        int s = totalSeconds % 60;
        return Timestamp(h, m, s);
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(1) << hours << ":"
            << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << seconds;
        return oss.str();
    }
    
    bool operator<(const Timestamp& other) const {
        return toSeconds() < other.toSeconds();
    }
    
    bool operator<=(const Timestamp& other) const {
        return toSeconds() <= other.toSeconds();
    }
};


struct Message {
    Timestamp timestamp;
    std::string content;
    bool isQuery;
    int queryK;
    
    Message() : isQuery(false), queryK(0) {}
    Message(const Timestamp& ts, const std::string& text) 
        : timestamp(ts), content(text), isQuery(false), queryK(0) {}
};

// 词频
struct WordFreq {
    std::string word;
    int count;
    
    WordFreq() : word(""), count(0) {}
    WordFreq(const std::string& w, int c) : word(w), count(c) {}
    
   
    bool operator<(const WordFreq& other) const {
        if (count != other.count) return count > other.count;
        return word < other.word; 
    }
};

// 滑动窗口

class SlidingWindow {
private:
    int windowSize;  
    std::unordered_map<std::string, int> wordCount;  
    std::queue<std::pair<Timestamp, std::vector<std::string>>> messageQueue;  
    std::set<std::string> stopWords;  
    std::set<std::string> sensitiveWords;  
    int totalWords;  
    Timestamp latestTime;  
    int outOfOrderCount;  
    int totalMessageCount;  
    
    // 快照
    struct Snapshot {
        Timestamp timestamp;
        std::unordered_map<std::string, int> wordCount;
        int totalWords;
    };
    std::vector<Snapshot> history;
    
public:
    SlidingWindow(int winSize = 600) : windowSize(winSize), totalWords(0), 
                                        latestTime(0, 0, 0), outOfOrderCount(0), totalMessageCount(0) {}
    

    void loadStopWords(const std::string& filename) {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            std::cerr << "[WARN] Cannot load stop words from: " << filename << std::endl;
            return;
        }
        std::string word;
        while (std::getline(ifs, word)) {
            if (!word.empty() && word.back() == '\r') {
                word.pop_back();
            }
            if (!word.empty()) {
                stopWords.insert(word);
            }
        }
        std::cout << "[INFO] Loaded " << stopWords.size() << " stop words." << std::endl;
    }
    
    
    void loadSensitiveWords(const std::string& filename) {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            std::cerr << "[WARN] Cannot load sensitive words from: " << filename << std::endl;
            return;
        }
        std::string word;
        while (std::getline(ifs, word)) {
            if (!word.empty() && word.back() == '\r') {
                word.pop_back();
            }
            if (!word.empty()) {
                sensitiveWords.insert(word);
            }
        }
        std::cout << "[INFO] Loaded " << sensitiveWords.size() << " sensitive words." << std::endl;
    }
    
   
    void addMessage(const Timestamp& ts, const std::vector<std::string>& words) {
        totalMessageCount++;
        
      
        if (ts < latestTime) {
            outOfOrderCount++;
        } else {
            latestTime = ts;
        }
        
    
        std::vector<std::string> filteredWords;
        for (const auto& word : words) {
            if (stopWords.find(word) == stopWords.end() && 
                sensitiveWords.find(word) == sensitiveWords.end() &&
                word.length() > 0) {
                filteredWords.push_back(word);
                wordCount[word]++;
                totalWords++;
            }
        }
        
      
        messageQueue.push({ts, filteredWords});
        
      
        removeExpiredMessages(ts);
    }
    
    // 删掉过期的
    void removeExpiredMessages(const Timestamp& currentTime) {
        int currentSeconds = currentTime.toSeconds();
        int windowStart = currentSeconds - windowSize;
        
        while (!messageQueue.empty()) {
            const auto& front = messageQueue.front();
            if (front.first.toSeconds() < windowStart) {
               
                for (const auto& word : front.second) {
                    auto it = wordCount.find(word);
                    if (it != wordCount.end()) {
                        it->second--;
                        totalWords--;
                        if (it->second <= 0) {
                            wordCount.erase(it);
                        }
                    }
                }
                messageQueue.pop();
            } else {
                break;
            }
        }
    }
    
    // Top-K
    std::vector<WordFreq> getTopK(int k) const {
        std::vector<WordFreq> result;
        for (const auto& pair : wordCount) {
            result.push_back(WordFreq(pair.first, pair.second));
        }
        
      
        std::sort(result.begin(), result.end());
        
        
        if (result.size() > (size_t)k) {
            result.resize(k);
        }
        
        return result;
    }
    
    // 存快照
    void saveSnapshot(const Timestamp& ts) {
        Snapshot snap;
        snap.timestamp = ts;
        snap.wordCount = wordCount;
        snap.totalWords = totalWords;
        history.push_back(snap);
    }
    
    // 趋势
    double getTrend(const std::string& word) const {
        if (history.size() < 2) return 0.0;
        
      
        const auto& current = wordCount;
        const auto& previous = history.back().wordCount;
        
        int currentCount = 0;
        int previousCount = 0;
        
        auto it1 = current.find(word);
        if (it1 != current.end()) currentCount = it1->second;
        
        auto it2 = previous.find(word);
        if (it2 != previous.end()) previousCount = it2->second;
        
        if (previousCount == 0) {
            return currentCount > 0 ? 100.0 : 0.0;
        }
        
        return ((double)(currentCount - previousCount) / previousCount) * 100.0;
    }
    
    // 统计
    void printStatistics() const {
        std::cout << "[STAT] Total unique words: " << wordCount.size() 
                  << ", Total words: " << totalWords 
                  << ", Messages in window: " << messageQueue.size() << std::endl;
    }
    
    // 新兴词
    std::vector<std::pair<std::string, double>> getEmergingWords(double threshold = 50.0) const {
        std::vector<std::pair<std::string, double>> emerging;
        
        if (history.size() < 2) return emerging;
        
        const auto& current = wordCount;
        const auto& previous = history.back().wordCount;
        
        for (const auto& pair : current) {
            const std::string& word = pair.first;
            int currentCount = pair.second;
            
            auto it = previous.find(word);
            int previousCount = (it != previous.end()) ? it->second : 0;
            
            if (previousCount == 0 && currentCount > 0) {
              
                if (currentCount >= 3) {  
                    emerging.push_back({word, 100.0});
                }
            } else if (previousCount > 0) {
                double growth = ((double)(currentCount - previousCount) / previousCount) * 100.0;
                if (growth >= threshold) {
                    emerging.push_back({word, growth});
                }
            }
        }
        
        // 排序
        std::sort(emerging.begin(), emerging.end(), 
                  [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
                      return a.second > b.second;
                  });
        
        return emerging;
    }
    
    // 降温词
    std::vector<std::pair<std::string, double>> getCoolingWords(double threshold = 30.0) const {
        std::vector<std::pair<std::string, double>> cooling;
        
        if (history.size() < 2) return cooling;
        
        const auto& current = wordCount;
        const auto& previous = history.back().wordCount;
        
        for (const auto& pair : previous) {
            const std::string& word = pair.first;
            int previousCount = pair.second;
            
            auto it = current.find(word);
            int currentCount = (it != current.end()) ? it->second : 0;
            
            if (previousCount > 0) {
                double decline = ((double)(previousCount - currentCount) / previousCount) * 100.0;
                if (decline >= threshold) {
                    cooling.push_back({word, decline});
                }
            }
        }
        
        // 排序
        std::sort(cooling.begin(), cooling.end(), 
                  [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
                      return a.second > b.second;
                  });
        
        return cooling;
    }
    
    int getTotalWords() const { return totalWords; }
    int getUniqueWords() const { return wordCount.size(); }
    int getOutOfOrderCount() const { return outOfOrderCount; }
    int getTotalMessageCount() const { return totalMessageCount; }
    double getOutOfOrderRate() const { 
        return totalMessageCount > 0 ? (double)outOfOrderCount / totalMessageCount * 100.0 : 0.0; 
    }
    
    // 调窗口
    void setWindowSize(int newSize) {
        windowSize = newSize;
        std::cout << "[INFO] Window size changed to " << newSize << " seconds (" 
                  << (newSize/60) << " minutes)" << std::endl;
    }
    
    int getWindowSize() const { return windowSize; }
};




// 解析时间
bool parseTimestamp(const std::string& line, Timestamp& ts, std::string& content) {
    if (line.empty() || line[0] != '[') return false;
    
    size_t endBracket = line.find(']');
    if (endBracket == std::string::npos) return false;
    
    std::string timeStr = line.substr(1, endBracket - 1);
    
  
    int h = 0, m = 0, s = 0;
    char colon1, colon2;
    std::istringstream iss(timeStr);
    
    if (!(iss >> h >> colon1 >> m >> colon2 >> s)) {
        return false;
    }
    
    ts = Timestamp(h, m, s);
    
    // 拿内容
    if (endBracket + 2 < line.length()) {
        content = line.substr(endBracket + 2);
    } else {
        content = "";
    }
    
    return true;
}

// 解析QUERY
bool parseQuery(const std::string& content, int& k) {
    if (content.find("[ACTION]") != std::string::npos && 
        content.find("QUERY") != std::string::npos) {
        size_t kPos = content.find("K=");
        if (kPos != std::string::npos) {
            std::string kStr = content.substr(kPos + 2);
            k = std::atoi(kStr.c_str());
            return true;
        }
    }
    return false;
}


// 主程序


int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  热词统计与分析系统 v1.0" << std::endl;
    std::cout << "  Hot Words Analysis System" << std::endl;
    std::cout << "========================================" << std::endl;
    
   
    std::string inputFile = "input1.txt";
    std::string outputFile = "hotwords_output.txt";
    int windowSize = 600; 
    
    if (argc >= 2) inputFile = argv[1];
    if (argc >= 3) outputFile = argv[2];
    if (argc >= 4) windowSize = std::atoi(argv[3]);
    
    std::cout << "[CONFIG] Input file: " << inputFile << std::endl;
    std::cout << "[CONFIG] Output file: " << outputFile << std::endl;
    std::cout << "[CONFIG] Window size: " << windowSize << " seconds" << std::endl;
   
    std::cout << "[INIT] Initializing Jieba segmenter..." << std::endl;
    cppjieba::Jieba jieba(
        "dict/jieba.dict.utf8",
        "dict/hmm_model.utf8",
        "dict/user.dict.utf8",
        "dict/idf.utf8",
        "dict/stop_words.utf8"
    );
    std::cout << "[INFO] Jieba initialized successfully." << std::endl;
    
    // 初始化
    SlidingWindow window(windowSize);
    window.loadStopWords("dict/stop_words.utf8");
    
    // 敏感词
    std::ifstream testSensitive("dict/sensitive_words.utf8");
    if (!testSensitive.is_open()) {
        std::ofstream createSensitive("dict/sensitive_words.utf8");
        createSensitive << "敏感词1\n敏感词2\n"; // 可以添加实际敏感词
        createSensitive.close();
    } else {
        testSensitive.close();
    }
    window.loadSensitiveWords("dict/sensitive_words.utf8");
    
    // 读文件
    std::cout << "[PROCESS] Reading input file..." << std::endl;
    std::ifstream ifs(inputFile);
    if (!ifs.is_open()) {
        std::cerr << "[ERROR] Cannot open input file: " << inputFile << std::endl;
        return EXIT_FAILURE;
    }
    
    // 输出文件
    std::ofstream ofs(outputFile);
    if (!ofs.is_open()) {
        std::cerr << "[ERROR] Cannot open output file: " << outputFile << std::endl;
        return EXIT_FAILURE;
    }
    
    ofs << "===== 热词统计与分析系统输出 =====" << std::endl;
    ofs << "输入文件: " << inputFile << std::endl;
    ofs << "窗口大小: " << windowSize << " 秒 (" << (windowSize/60) << " 分钟)" << std::endl;
    ofs << "======================================" << std::endl << std::endl;
    
    // 处理
    std::string line;
    int lineCount = 0;
    int queryCount = 0;
    
    while (std::getline(ifs, line)) {
        lineCount++;
        
        // 换行符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        Timestamp ts;
        std::string content;
        
        if (!parseTimestamp(line, ts, content)) {
           
            int k;
            if (parseQuery(line, k)) {
                queryCount++;
                std::cout << "[QUERY " << queryCount << "] Top-" << k << " at line " << lineCount << std::endl;
                
                auto topK = window.getTopK(k);
                
                ofs << "[时间: 当前] Query #" << queryCount << " - Top-" << k << " 热词:" << std::endl;
                for (size_t i = 0; i < topK.size(); ++i) {
                    ofs << "  " << (i+1) << ". " << topK[i].word 
                        << " (出现 " << topK[i].count << " 次)";
                    
                   
                    double trend = window.getTrend(topK[i].word);
                    if (trend > 0) {
                        ofs << " ↑" << std::fixed << std::setprecision(1) << trend << "%";
                    } else if (trend < 0) {
                        ofs << " ↓" << std::fixed << std::setprecision(1) << (-trend) << "%";
                    }
                    ofs << std::endl;
                }
                
                // 新兴词
                auto emerging = window.getEmergingWords(50.0);
                if (!emerging.empty() && queryCount > 1) {
                    ofs << "\n  📈 新兴热词 (增长率>50%):" << std::endl;
                    for (size_t i = 0; i < std::min(emerging.size(), (size_t)3); ++i) {
                        ofs << "    • " << emerging[i].first << " (+" 
                            << std::fixed << std::setprecision(1) << emerging[i].second << "%)" << std::endl;
                    }
                }
                
                // 降温词
                auto cooling = window.getCoolingWords(30.0);
                if (!cooling.empty() && queryCount > 1) {
                    ofs << "  📉 降温热词 (下降率>30%):" << std::endl;
                    for (size_t i = 0; i < std::min(cooling.size(), (size_t)3); ++i) {
                        ofs << "    • " << cooling[i].first << " (-" 
                            << std::fixed << std::setprecision(1) << cooling[i].second << "%)" << std::endl;
                    }
                }
                
                ofs << std::endl;
                window.saveSnapshot(Timestamp(0, 0, 0));
                window.printStatistics();
            }
            continue;
        }
        
        // 检查QUERY
        int k;
        if (parseQuery(content, k)) {
            queryCount++;
            std::cout << "[QUERY " << queryCount << "] Top-" << k << " at " << ts.toString() << std::endl;
            
            auto topK = window.getTopK(k);
            
            ofs << "[时间: " << ts.toString() << "] Query #" << queryCount << " - Top-" << k << " 热词:" << std::endl;
            for (size_t i = 0; i < topK.size(); ++i) {
                ofs << "  " << (i+1) << ". " << topK[i].word 
                    << " (出现 " << topK[i].count << " 次)";
                
                // 趋势
                double trend = window.getTrend(topK[i].word);
                if (trend > 0) {
                    ofs << " ↑" << std::fixed << std::setprecision(1) << trend << "%";
                } else if (trend < 0) {
                    ofs << " ↓" << std::fixed << std::setprecision(1) << (-trend) << "%";
                }
                ofs << std::endl;
            }
            
            // 新兴词
            auto emerging = window.getEmergingWords(50.0);
            if (!emerging.empty() && queryCount > 1) {
                ofs << "\n  📈 新兴热词 (增长率>50%):" << std::endl;
                for (size_t i = 0; i < std::min(emerging.size(), (size_t)3); ++i) {
                    ofs << "    • " << emerging[i].first << " (+" 
                        << std::fixed << std::setprecision(1) << emerging[i].second << "%)" << std::endl;
                }
            }
            
            // 降温词
            auto cooling = window.getCoolingWords(30.0);
            if (!cooling.empty() && queryCount > 1) {
                ofs << "  📉 降温热词 (下降率>30%):" << std::endl;
                for (size_t i = 0; i < std::min(cooling.size(), (size_t)3); ++i) {
                    ofs << "    • " << cooling[i].first << " (-" 
                        << std::fixed << std::setprecision(1) << cooling[i].second << "%)" << std::endl;
                }
            }
            
            ofs << std::endl;
            
            window.saveSnapshot(ts);
            window.printStatistics();
            continue;
        }
        
        // 分词
        std::vector<std::string> words;
        jieba.Cut(content, words, true);
        
        // 加到窗口
        window.addMessage(ts, words);
        
        // 进度
        if (lineCount % 1000 == 0) {
            std::cout << "[PROGRESS] Processed " << lineCount << " lines..." << std::endl;
        }
    }
    
    std::cout << "[INFO] Total lines processed: " << lineCount << std::endl;
    std::cout << "[INFO] Total queries: " << queryCount << std::endl;
    std::cout << "[INFO] Out-of-order messages: " << window.getOutOfOrderCount() 
              << " (" << std::fixed << std::setprecision(2) << window.getOutOfOrderRate() << "%)" << std::endl;
    
    // 自动查询
    if (queryCount < 2 && lineCount > 0) {
        std::cout << "[AUTO] Executing automatic final query for trend analysis..." << std::endl;
        queryCount++;
        
        auto topK = window.getTopK(10);
        
        ofs << "\n[时间: 最终] Query #" << queryCount << " - Top-10 热词（自动查询）:" << std::endl;
        for (size_t i = 0; i < topK.size(); ++i) {
            ofs << "  " << (i+1) << ". " << topK[i].word 
                << " (出现 " << topK[i].count << " 次)";
            
            // 趋势
            double trend = window.getTrend(topK[i].word);
            if (trend > 0) {
                ofs << " ↑" << std::fixed << std::setprecision(1) << trend << "%";
            } else if (trend < 0) {
                ofs << " ↓" << std::fixed << std::setprecision(1) << (-trend) << "%";
            }
            ofs << std::endl;
        }
        
        // 新兴词
        auto emerging = window.getEmergingWords(50.0);
        if (!emerging.empty() && queryCount > 1) {
            ofs << "\n  📈 新兴热词 (增长率>50%):" << std::endl;
            for (size_t i = 0; i < std::min(emerging.size(), (size_t)3); ++i) {
                ofs << "    • " << emerging[i].first << " (+" 
                    << std::fixed << std::setprecision(1) << emerging[i].second << "%)" << std::endl;
            }
        }
        
        // 降温词
        auto cooling = window.getCoolingWords(30.0);
        if (!cooling.empty() && queryCount > 1) {
            ofs << "  📉 降温热词 (下降率>30%):" << std::endl;
            for (size_t i = 0; i < std::min(cooling.size(), (size_t)3); ++i) {
                ofs << "    • " << cooling[i].first << " (-" 
                    << std::fixed << std::setprecision(1) << cooling[i].second << "%)" << std::endl;
            }
        }
        
        ofs << std::endl;
        window.saveSnapshot(Timestamp(99, 99, 99));
    }
    
    // 最终统计
    ofs << "\n===== 最终统计 =====" << std::endl;
    ofs << "处理的总行数: " << lineCount << std::endl;
    ofs << "处理的消息数: " << window.getTotalMessageCount() << std::endl;
    ofs << "查询次数: " << queryCount << std::endl;
    ofs << "窗口大小: " << window.getWindowSize() << " 秒 (" << (window.getWindowSize()/60) << " 分钟)" << std::endl;
    ofs << "窗口内唯一词数: " << window.getUniqueWords() << std::endl;
    ofs << "窗口内总词数: " << window.getTotalWords() << std::endl;
    ofs << "乱序消息数: " << window.getOutOfOrderCount() 
        << " (" << std::fixed << std::setprecision(2) << window.getOutOfOrderRate() << "%)" << std::endl;
    
    // 最终Top-20
    ofs << "\n===== 最终 Top-20 热词 =====" << std::endl;
    auto finalTop = window.getTopK(20);
    for (size_t i = 0; i < finalTop.size(); ++i) {
        ofs << "  " << (i+1) << ". " << finalTop[i].word 
            << " (出现 " << finalTop[i].count << " 次)" << std::endl;
    }
    
    ofs << "\n===== 分析完成 =====" << std::endl;
    
    ifs.close();
    ofs.close();
    
    std::cout << "[SUCCESS] Analysis completed. Results saved to: " << outputFile << std::endl;
    std::cout << "========================================" << std::endl;
    
    return EXIT_SUCCESS;
}
