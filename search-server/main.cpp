#include "document.h"
#include "search_server.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "paginator.h"
#include "request_queue.h"

using namespace std;

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
	cout << "{ "s
		<< "document_id = "s << document_id << ", "s
		<< "status = "s << static_cast<int>(status) << ", "s
		<< "words ="s;
	for (const string& word : words) {
		cout << ' ' << word;
	}
	cout << "}"s << endl;
}

int main() {
	SearchServer search_server("and in on"s);
	search_server.AddDocument(0, "white cat and modern ring"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "puffy cat puffy tail cat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "nice dog cool eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "nice bird jenny"s, DocumentStatus::BANNED, { 9 });
	const int document_count = search_server.GetDocumentCount();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		const auto [words, status] = search_server.MatchDocument("puffy cat"s, document_id);
		PrintMatchDocumentResult(document_id, words, status);
	}
	cout << endl << endl;
	auto result = search_server.FindTopDocuments("cat -white");
	cout << result.size() << endl;
	for (const auto& item : result) {
		cout << "{ "s
			<< "document_id = "s << item.id << ", "s
			<< "rating = "s << item.rating << ", "s
			<< "relevance = "s << item.relevance;
		cout << "}"s << endl;
	}
    return 0;
}