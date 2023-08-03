#include "document.h"

Document::Document(int id, double relevance, int rating)
	: id(id)
	, relevance(relevance)
	, rating(rating) {
}

std::ostream& operator<<(std::ostream& output, Document item) {
	using namespace std::string_literals;
	output << "{ document_id = "s << item.id << ", relevance = "s << item.relevance << ", rating = "s << item.rating << " }"s;

	return output;
}