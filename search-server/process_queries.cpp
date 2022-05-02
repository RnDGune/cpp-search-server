
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server,
	const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	
	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), 
		[&](std::string s) {return search_server.FindTopDocuments(s); });
	return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> documents(queries.size());
    std::transform(std::execution::par,
        queries.begin(), queries.end(), documents.begin(),
        [&search_server](std::string const & qry) {
            auto v = search_server.FindTopDocuments(qry);
            return v;
        });
    std::vector<Document> result;
    for (auto tmp : documents) {
        result.insert(result.end(), tmp.begin(),tmp.end());
    }
    return result;
}