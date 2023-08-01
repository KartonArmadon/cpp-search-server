#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
	std::vector<Document> result = server_.FindTopDocuments(raw_query, status);
	CollectRequest(result.empty());
	return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	std::vector<Document> result = server_.FindTopDocuments(raw_query);
	CollectRequest(result.empty());
	return result;
}
void RequestQueue::CollectRequest(bool isEmpty) {
	RequestPushBack(isEmpty);
	if (requests_.size() > min_in_day_) RequestPopFront();
}
void RequestQueue::RequestPopFront() {
	if (requests_.front().isEmpty == true) --empty_count;
	requests_.pop_front();
}
void RequestQueue::RequestPushBack(bool isEmpty) {
	QueryResult q{ isEmpty };
	requests_.push_back(q);
	if (isEmpty) ++empty_count;
}
int RequestQueue::GetNoResultRequests() const {
	return empty_count;
}