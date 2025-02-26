#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>

std::mutex mtx;
std::unordered_map<std::string, int> global_sentiment;
std::unordered_map<std::string, int> word_count;

const std::vector<std::string> positive_words = {"good", "excellent", "great", "fantastic", "love"};
const std::vector<std::string> negative_words = {"bad", "terrible", "horrible", "disastrous", "awful"};

// Lower Function
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// Analyze text return local sentiment and word count
void analyze_text(const std::string& review, std::unordered_map<std::string, int>& local_sentiment, std::unordered_map<std::string, int>& local_word_count) {
    int positive_count = 0, negative_count = 0;
    std::istringstream textStream(review);
    std::string word;

    while (textStream >> word) {
        word = to_lower(word);
        if (std::find(positive_words.begin(), positive_words.end(), word) != positive_words.end() ||
            std::find(negative_words.begin(), negative_words.end(), word) != negative_words.end()) {
            local_word_count[word]++;
        }

        if (std::find(positive_words.begin(), positive_words.end(), word) != positive_words.end()) {
            positive_count++;
        }
        if (std::find(negative_words.begin(), negative_words.end(), word) != negative_words.end()) {
            negative_count++;
        }
    }

    std::string sentiment;
    if (positive_count > negative_count) sentiment = "Positive";
    else if (negative_count > positive_count) sentiment = "Negative";
    else sentiment = "Neutral";

    local_sentiment[sentiment]++;
}

// Map Function - Processes a chunk of reviews
void mapper(const std::vector<std::string>& reviews, size_t start, size_t end, std::unordered_map<std::string, int>& local_sentiment, std::unordered_map<std::string, int>& local_word_count) {
    for (size_t i = start; i < end; i++) {
        analyze_text(reviews[i], local_sentiment, local_word_count);
    }
}

// Reduce Function - Partial results
void reducer(const std::vector<std::unordered_map<std::string, int>>& sentiment_results, const std::vector<std::unordered_map<std::string, int>>& word_results) {
    for (const auto& local_sentiment : sentiment_results) {
        for (const auto& entry : local_sentiment) {
            global_sentiment[entry.first] += entry.second;
        }
    }

    for (const auto& local_word_count : word_results) {
        for (const auto& entry : local_word_count) {
            word_count[entry.first] += entry.second;
        }
    }
}

// Read File Function
std::vector<std::string> read_reviews_from_file(const std::string& filename) {
    std::vector<std::string> reviews;
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Error: Cannot open the file " << filename << std::endl;
        return reviews;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            reviews.push_back(line);
        }
    }
    return reviews;
}

int main() {
    std::string filename = "folder;
    std::vector<std::string> reviews = read_reviews_from_file(filename);

    if (reviews.empty()) {
        std::cerr << "There isn't a review file." << std::endl;
        return 1;
    }

    // Parallel execution
    size_t num_workers = std::thread::hardware_concurrency();
    size_t chunk_size = reviews.size() / num_workers;
    std::vector<std::thread> mappers;
    std::vector<std::unordered_map<std::string, int>> sentiment_results(num_workers);
    std::vector<std::unordered_map<std::string, int>> word_results(num_workers);

    for (size_t i = 0; i < num_workers; i++) {
        size_t start = i * chunk_size;
        size_t end = (i == num_workers - 1) ? reviews.size() : start + chunk_size;
        mappers.emplace_back(mapper, std::ref(reviews), start, end, std::ref(sentiment_results[i]), std::ref(word_results[i]));
    }

    for (auto& t : mappers) {
        t.join();
    }

    // Reduce phase
    reducer(sentiment_results, word_results);

    // Print results
    std::cout << "=== Analysis Result ===\n";
    for (const auto& entry : global_sentiment) {
        std::cout << entry.first << ": " << entry.second << " reviews\n";
    }

    std::cout << "\n=== Word Count ===\n";
    for (const auto& entry : word_count) {
        std::cout << entry.first << ": " << entry.second << "\n";
    }

}
