#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> uniq_documents;// временное хранилище уникальных документов
    bool flag = 0;// флаг для выхода из цикла
    std::vector<int> delete_list;//список на удаление
    for (auto document_id : search_server) { // перебераем все документы из search_servera
        flag = 0;
        if (uniq_documents.empty()) {
            uniq_documents.push_back(document_id);
            continue;
        }
        for (int uniq_document_id : uniq_documents) {
            if (search_server.CompareDocumentsWords(uniq_document_id, document_id)) { // если документа нет, добовляем в uniq_documents если есть удаляем 
                delete_list.push_back(document_id);
                std::cout << "Found duplicate document id " << document_id << std::endl;
                flag = 1;
                continue;
            }
        }
        if (flag) {
            continue;
        }
        uniq_documents.push_back(document_id);
    }
    for (int delete_id : delete_list) {
        search_server.RemoveDocument(delete_id);
    }
}