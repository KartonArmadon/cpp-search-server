#pragma once
#include "search_server.h"

#include <vector>
#include <deque>


class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(
		const std::string& raw_query, 
		DocumentPredicate document_predicate);
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	void CollectRequest(bool isEmpty);

	void RequestPopFront();

	void RequestPushBack(bool isEmpty);

	int GetNoResultRequests() const;
private:
	struct QueryResult {
		bool isEmpty;
	};
	std::deque<QueryResult> requests_ = {};
	int empty_count = 0;

	const static int min_in_day_ = 1440;
	const SearchServer& server_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	std::vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
	CollectRequest(result.empty());
	return result;
}
