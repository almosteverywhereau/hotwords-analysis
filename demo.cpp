#include "cppjieba/Jieba.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

// 读文件
bool ReadUtf8Lines(const std::string& filename, std::vector<std::string>& lines) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        // 换行符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return true;
}

// 拼字符串
std::string Join(const std::vector<std::string>& items, const std::string& delim) {
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) oss << delim;
        oss << items[i];
    }
    return oss.str();
}

int main(int argc, char* argv[]) {
    // 1. 参数
    std::string inputFile = "test_sentences.txt";   
    std::string outputFile = "output.txt";          
    if (argc >= 2) inputFile = argv[1];
    if (argc >= 3) outputFile = argv[2];

    // 2. 初始化
    cppjieba::Jieba jieba(
        "dict/jieba.dict.utf8",      
        "dict/hmm_model.utf8",       
        "dict/user.dict.utf8",       
        "dict/idf.utf8",             
        "dict/stop_words.utf8"       
    );

    // 3. 读
    std::vector<std::string> lines;
    if (!ReadUtf8Lines(inputFile, lines)) {
        std::cerr << "[ERROR] cannot open input file: " << inputFile << std::endl;
        std::cerr << "[HINT ] create a UTF-8 file named '" << inputFile << "' with Chinese sentences." << std::endl;
        return EXIT_FAILURE;
    }
    if (lines.empty()) {
        std::cerr << "[WARN ] input file is empty." << std::endl;
    }

    // 4. 写
    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[ERROR] cannot open output file: " << outputFile << std::endl;
        return EXIT_FAILURE;
    }

    // 头信息
    out << "===== cppjieba segmentation demo (Sun Yat-sen University CS School) =====\n";
    out << "InputFile: " << inputFile << "\n";
    out << "LineCount: " << lines.size() << "\n";
    out << "Note: Chinese sentences are read from input and processed; console prints remain English to avoid encoding issues.\n\n";

    // 5. 用户词
    jieba.InsertUserWord("中山大学计算机学院");

    // 6. 处理
    const size_t topk = 5; 
    for (size_t idx = 0; idx < lines.size(); ++idx) {
        const std::string& sentence = lines[idx];
        out << "[LINE " << (idx + 1) << "] " << sentence << "\n";

        // HMM
        std::vector<std::string> cutWords;
        jieba.Cut(sentence, cutWords, true);
        out << "  Cut(HMM): " << Join(cutWords, "/") << "\n";

        // NoHMM
        std::vector<std::string> cutWordsNoHMM;
        jieba.Cut(sentence, cutWordsNoHMM, false);
        out << "  Cut(NoHMM): " << Join(cutWordsNoHMM, "/") << "\n";

        // 搜索模式
        std::vector<std::string> searchWords;
        jieba.CutForSearch(sentence, searchWords);
        out << "  CutForSearch: " << Join(searchWords, "/") << "\n";

        // 标注
        std::vector<std::pair<std::string, std::string>> tagres;
        jieba.Tag(sentence, tagres);
        out << "  Tag: ";
        for (size_t i = 0; i < tagres.size(); ++i) {
            if (i) out << "/";
            out << tagres[i].first << "(" << tagres[i].second << ")";
        }
        out << "\n";

        // 关键词
        std::vector<cppjieba::KeywordExtractor::Word> keywordres;
        jieba.extractor.Extract(sentence, keywordres, topk);
        out << "  Keywords(TF-IDF): ";
        for (size_t i = 0; i < keywordres.size(); ++i) {
            if (i) out << ", ";
            out << keywordres[i].word << ":" << keywordres[i].weight;
        }
        out << "\n";

        // 关键词2
        std::vector<std::pair<std::string, double>> keywordres2;
        jieba.extractor.Extract(sentence, keywordres2, topk);
        out << "  Keywords2: ";
        for (size_t i = 0; i < keywordres2.size(); ++i) {
            if (i) out << ", ";
            out << keywordres2[i].first << ":" << keywordres2[i].second;
        }
        out << "\n\n"; 
    }

    out << "===== END =====\n";
    out.close();

    std::cout << "Processing finished. Output file generated: " << outputFile << std::endl;
    std::cout << "Open the file with a UTF-8 capable editor to view results." << std::endl;
    return EXIT_SUCCESS;
}

