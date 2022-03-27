#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    int flag = 0;
    std::vector<int> delete_id_list;
    std::map <int, std::set<std::string>> uniq_documents;
    for (const auto document_id : search_server) {
        flag = 0;
        std::set<std::string> uniq_words;
        auto words_in_document = search_server.GetWordFrequencies(document_id);
        for (auto words : words_in_document) { 
            uniq_words.emplace(words.first);
        }
        if (uniq_documents.empty()) {
            uniq_documents.emplace(document_id, uniq_words);
            continue;
        }
        else {
            for (auto docs : uniq_documents) {
                if (uniq_words == docs.second) {
                    delete_id_list.push_back(document_id);
                    std::cout << "Found duplicate document id " << document_id << std::endl;
                    flag = 1;
                    break;
                }
            }
        }
        if (flag != 1) {
            uniq_documents.emplace(document_id, uniq_words);
        }
    }
    for (int delete_id : delete_id_list) {
        search_server.RemoveDocument(delete_id);
    }
}

