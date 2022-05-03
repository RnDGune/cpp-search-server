#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) 
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  
                                                        
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();

    for (const std::string_view word : words) {
        auto it = document_words_set_.emplace(std::string(word));
        word_to_document_freqs_[*it.first][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*it.first] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(std::execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
    int document_id) const {

    const auto query = ParseQuery(std::execution::seq, raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    if (document_ids_.count(document_id) == 0) { return { {}, {} }; }
    const auto& words_freq = GetWordFrequencies(document_id);
    auto word_checker = [&words_freq](const auto word) {
        return words_freq.count(word);
    };

    bool is_minus_word = any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), word_checker);
    if (is_minus_word) {
        return
        { std::vector<std::string_view>{}, documents_.at(document_id).status };
    }
    copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_checker);
    sort( matched_words.begin(), matched_words.end());
    auto it_last = remove(matched_words.begin(), matched_words.end(), "");
    matched_words.erase(it_last, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
    const std::string_view raw_query, int document_id) const {
   
    const auto query = ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());
    if (document_ids_.count(document_id) == 0) { return { {}, {} }; }
    const auto& words_freq = GetWordFrequencies(document_id);
    auto word_checker = [&words_freq](const auto word) {
        return words_freq.count(word);
    };

    bool is_minus_word = any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), word_checker);
    if (is_minus_word) {
        return
        { std::vector<std::string_view>{}, documents_.at(document_id).status };
    }
    copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_checker);
    sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto it_last = unique(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(it_last, matched_words.end());
    it_last = remove(std::execution::par, matched_words.begin(), matched_words.end(), "");
    matched_words.erase(it_last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };

  
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
(const std::execution::sequenced_policy& policy,const std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word " + std::string(word) + " is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int rating_sum = 0;
    rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word " + std::string(text) + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    if (word_to_document_freqs_.at(word).size() == 0) {
        throw std::invalid_argument("The word is missing from the document");
    }
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
  
}

 const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> empty_map;
    if (document_to_word_freqs_.count(document_id) != 0) {
        return document_to_word_freqs_.at(document_id);
    }
    return empty_map;
}

 void SearchServer::RemoveDocument(int document_id) {
     std::vector<std::string_view> vec_words;
     if (document_to_word_freqs_.count(document_id) == 0) {
         return;
     }
     for (auto& [key, value] : document_to_word_freqs_.at(document_id)) {
             vec_words.push_back(key);        
     }
     for (auto& word : vec_words) {
         word_to_document_freqs_.erase(word);
     }
     documents_.erase(document_id);
     document_to_word_freqs_.erase(document_id);
     //auto it = find(document_ids_.begin(), document_ids_.end(), document_id);
     //if (it != document_ids_.end()) {
     document_ids_.erase(document_id);
     //}
    
 }

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    const auto& words_freqs = GetWordFrequencies(document_id);
    if (!words_freqs.empty()) {
        std::vector<std::string_view> words;
        words.reserve(words_freqs.size());
        std::for_each(policy, words_freqs.begin(), words_freqs.end(), [&words](auto& wf) {
            words.push_back(std::string_view(wf.first));
            });
        std::for_each(policy, words.begin(), words.end(), [this, document_id](const auto& word) {
            return word_to_document_freqs_[word].erase(document_id);
            });
    }
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
    
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    return SearchServer::RemoveDocument(document_id);
}



