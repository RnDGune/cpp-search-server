//#include "search_server.h" разкоментить при сдаче

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iostream>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) { //  1e-6
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        /*for (const int rating : ratings) {
            rating_sum += rating;
        }*/
        return rating_sum / static_cast<int>(ratings.size());
    }



    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }


    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }


    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};


/*
   Подставьте сюда вашу реализацию макросов

   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func) RunTestImpl(func,#func) ;

template <typename FuncType>
void RunTestImpl(const FuncType& test_func, const string& func_name) {
    test_func();
    cerr << func_name << " OK" << endl;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}




// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
void TestAddDocument() {
    const int doc_id_1 = 1;
    const int doc_id_2 = 2;
    const int doc_id_3 = 3;
    const string content_1 = "cat in the city"s;
    const string content_2 = "dog in the city"s;
    const string content_3 = "cat in the countryside"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        const auto add_documents = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(add_documents.size(), 0, "Found non-existent documents");
    }
    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);
        const auto add_documents_cat = server.FindTopDocuments("cat"s);
        const auto add_documents_dog = server.FindTopDocuments("dog"s);
        ASSERT_EQUAL(add_documents_cat.size(), 2);
        ASSERT_EQUAL_HINT(add_documents_cat[0].id, 1, "Mismatched ID");
        ASSERT_EQUAL_HINT(add_documents_cat[1].id, 3, "Mismatched ID");
        ASSERT_EQUAL_HINT(add_documents_cat[0].rating, 2, "Wrong rating");
        ASSERT_EQUAL_HINT(add_documents_cat[1].rating, 2, "Wrong rating");
        ASSERT_EQUAL(add_documents_dog.size(), 1);
        ASSERT_EQUAL_HINT(add_documents_dog[0].id, 2, "Mismatched ID");
        ASSERT_EQUAL_HINT(add_documents_dog[0].rating, 2, "Wrong rating");
        
    }
}

void TestExcludeDocumentWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto not_find_documents = server.FindTopDocuments("cat -city"s);
        const auto find_documents = server.FindTopDocuments("cat"s);
        ASSERT_HINT(not_find_documents.size() == 0, "Not excluded minus word");
        ASSERT(find_documents.size() == 1);

    }

}

void TestMatchingDocuments() {

        SearchServer test_server;
        test_server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        test_server.AddDocument(1, "cat in the city"s, DocumentStatus::IRRELEVANT, { 2, 2, 2 });
        test_server.AddDocument(2, "cat in the city"s, DocumentStatus::BANNED, { 2, 2, 2 });
        test_server.AddDocument(3, "cat in the city"s, DocumentStatus::REMOVED, { 2, 2, 2 });
        const auto find_documents_actual = test_server.FindTopDocuments("cat city");
        ASSERT_HINT( find_documents_actual[0].id == 0, "Incorrect search by status"s);
        const auto find_documents_irrleveant = test_server.FindTopDocuments("cat city", DocumentStatus::IRRELEVANT);
        ASSERT_HINT(find_documents_irrleveant[0].id == 1, "Incorrect search by status"s);
        const auto find_documents_banned = test_server.FindTopDocuments("cat city", DocumentStatus::BANNED);
        ASSERT_HINT(find_documents_banned[0].id == 2, "Incorrect search by status"s);
        const auto find_documents_removed = test_server.FindTopDocuments("cat city", DocumentStatus::REMOVED);
        ASSERT_HINT(find_documents_removed[0].id == 3, "Incorrect search by status"s);
       
}

void TestRelevance() {

    SearchServer test_server;
    test_server.AddDocument(0, "white cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    test_server.AddDocument(1, "brown dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    test_server.AddDocument(2, "black cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto find_documents = test_server.FindTopDocuments("white cat city"s);// log(3) log(3/2) log(1)

    ASSERT_HINT(find_documents[0].id == 0, "Wrong relevance sort"s);
    ASSERT_HINT(find_documents[1].id == 2, "Wrong relevance sort"s);
    ASSERT_HINT(find_documents[2].id == 1, "Wrong relevance sort"s);
    
    const double relevance_1 = ((log(3.0) * 1.0 / 5.0) + (log(3.0 / 2.0) * 1.0 / 5.0) + (log(1.0) * 1.0 / 5.0));
    const double relevance_2 = (log(1.0) * 1.0 / 5.0);
    const double relevance_3 = ((log(3.0 / 2.0) * 1.0 / 5.0) + (log(1.0) * 1.0 / 5.0));
    ASSERT_HINT((find_documents[0].relevance-relevance_1)<EPSILON,"Wrong relevane calculate"s);
    ASSERT_HINT((find_documents[1].relevance - relevance_3) < EPSILON, "Wrong relevane calculate"s);
    ASSERT_HINT((find_documents[2].relevance - relevance_2) < EPSILON, "Wrong relevane calculate"s);   
}

void TestRating() {
    SearchServer test_server;
    test_server.AddDocument(0, "white cat in the city"s, DocumentStatus::ACTUAL, { -2, 14, 0 }); //4
    test_server.AddDocument(1, "white cat in the city"s, DocumentStatus::ACTUAL, { 2, 2}); //2
    test_server.AddDocument(2, "white cat in the city"s, DocumentStatus::ACTUAL, { 0});        //0

    const auto find_documents = test_server.FindTopDocuments("cat"s);

    ASSERT_HINT(find_documents[0].rating == 4, "Wrong rating calculation");
    ASSERT_HINT(find_documents[1].rating == 2, "Wrong rating calculation");
    ASSERT_HINT(find_documents[2].rating == 0, "Wrong rating calculation");

}

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);// тест работы стоп слов
    RUN_TEST(TestAddDocument); // тест добавления документов 
    RUN_TEST(TestExcludeDocumentWithMinusWords); // тест поддержки минус слов
    RUN_TEST(TestMatchingDocuments); // тест  поиска с заданым статусом
    RUN_TEST(TestRelevance); // тест подсчета релевантноси и сортировки по релевантности
    RUN_TEST(TestRating); // тест подсчета рейтинга

    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}