#include <iostream>
#include <stdexcept>

#include <algorithm>
#include <cmath>
#include <utility>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
	Document() = default;

	Document(int id, double relevance, int rating)
		: id(id)
		, relevance(relevance)
		, rating(rating) {
	}

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

ostream& operator<<(ostream& output, Document item) {
	output << "{ document_id = "s << item.id << ", relevance = "s << item.relevance << ", rating = "s << item.rating << " }"s;
	return output;
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
	set<string> non_empty_strings;
	for (const string& str : strings) {
		if (!str.empty()) {
			non_empty_strings.insert(str);
		}
	}
	return non_empty_strings;
}

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

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

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
	{
		if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw invalid_argument("Some of stop words are invalid"s);
		}
	}

	explicit SearchServer(const string& stop_words_text)
		: SearchServer(
			SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
	{
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status,
		const vector<int>& ratings) {
		if ((document_id < 0) || (documents_.count(document_id) > 0)) {
			throw invalid_argument("Invalid document_id"s);
		}
		const auto words = SplitIntoWordsNoStop(document);

		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
		document_ids_.push_back(document_id);
	}

	template <typename DocumentPredicate>
	vector<Document> FindTopDocuments(const string& raw_query,
		DocumentPredicate document_predicate) const {
		const auto query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, document_predicate);
		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
		return FindTopDocuments(
			raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
				return document_status == status;
			});
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	size_t GetDocumentCount() const {
		return documents_.size();
	}

	int GetDocumentId(int index) const {
		return document_ids_.at(index);
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
		int document_id) const {
		const auto query = ParseQuery(raw_query);

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
	const set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;
	vector<int> document_ids_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	static bool IsValidWord(const string& word) {
		// A valid word must not contain special characters
		return none_of(word.begin(), word.end(), [](char c) {
			return c >= '\0' && c < ' ';
			});
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!IsValidWord(word)) {
				throw invalid_argument("Word "s + word + " is invalid"s);
			}
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
		for (const int rating : ratings) {
			rating_sum += rating;
		}
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(const string& text) const {
		if (text.empty()) {
			throw invalid_argument("Query word is empty"s);
		}
		string word = text;
		bool is_minus = false;
		if (word[0] == '-') {
			is_minus = true;
			word = word.substr(1);
		}
		if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
			throw invalid_argument("Query word "s + text + " is invalid");
		}

		return { word, is_minus, IsStopWord(word) };
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query result;
		for (const string& word : SplitIntoWords(text)) {
			const auto query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					result.minus_words.insert(query_word.data);
				}
				else {
					result.plus_words.insert(query_word.data);
				}
			}
		}
		return result;
	}

	// Existence required
	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template <typename DocumentPredicate>
	vector<Document> FindAllDocuments(const Query& query,
		DocumentPredicate document_predicate) const {
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
			matched_documents.push_back(
				{ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};

template<typename PageIterator>
class IteratorRange {
public:
	IteratorRange(PageIterator begin, PageIterator end) :_begin(begin), _end(end) {}
	auto begin() { return _begin; }
	auto end() { return _end; }
	auto size() { return distance(_begin, _end); }

private:
	PageIterator _begin;
	PageIterator _end;
};

template<typename PageIterator>
ostream& operator<<(ostream& output, IteratorRange<PageIterator> item) {
	for (auto it = item.begin(); it != item.end(); ++it) {
		cout << *it;
	}
	return output;
}

template<typename PageIterator>
class Paginator {
public:
	Paginator(PageIterator it_begin, PageIterator it_end, size_t page_size = 3) {
		if (page_size < 1)
			page_size = 1;
		auto size = distance(it_begin, it_end);
		if (size == 0)
			return;
		size_t page_count = 1 + (size / 3);
		_pages.reserve(page_count);
		PageIterator it_current = it_begin;
		size_t current_page_size = {};
		size_t distance_to_end = {};
		do
		{
			distance_to_end = distance(it_current, it_end);
			current_page_size = min(distance_to_end, page_size);
			_pages.push_back(IteratorRange(it_current, it_current + current_page_size));
			it_current = next(it_current, current_page_size);
		} while (distance(it_current, it_end) > 0);
	}

	auto begin() const { return _pages.begin(); }

	auto end() const { return _pages.end(); }

	size_t size() const { return _pages.size(); }

private:
	vector<IteratorRange<PageIterator>> _pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size);
}

class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server) :server_(search_server) {}
	// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
	template <typename DocumentPredicate>
	vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
		vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
		CollectRequest(result.empty());
		return result;
	}
	vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
		vector<Document> result = server_.FindTopDocuments(raw_query, status);
		CollectRequest(result.empty());
		return result;
	}
	vector<Document> AddFindRequest(const string& raw_query) {
		vector<Document> result = server_.FindTopDocuments(raw_query);
		CollectRequest(result.empty());
		return result;
	}
	int GetNoResultRequests() const {
		return no_result_requests_;
	}
private:
	struct QueryResult {
		bool isEmpty;
	};
	deque<QueryResult> requests_ = {};
	int no_result_requests_ = 0;

	const static int min_in_day_ = 1440;
	const SearchServer& server_;

	void CollectRequest(bool isEmpty) {
		RequestPushBack(isEmpty);
		if (requests_.size() > min_in_day_) RequestPopFront();
	}
	void RequestPopFront() {
		if (requests_.front().isEmpty == true) --no_result_requests_;
		requests_.pop_front();
	}
	void RequestPushBack(bool isEmpty) {
		QueryResult q{ isEmpty };
		requests_.push_back(q);
		if (isEmpty) ++no_result_requests_;
	}
};

int main() {
	SearchServer search_server("and in at"s);
	RequestQueue request_queue(search_server);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
	search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
	search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
	// 1439 запросов с нулевым результатом
	for (int i = 0; i < 1439; ++i) {
		request_queue.AddFindRequest("empty request"s);
	}
	// все еще 1439 запросов с нулевым результатом
	request_queue.AddFindRequest("curly dog"s);
	// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
	request_queue.AddFindRequest("big collar"s);
	// первый запрос удален, 1437 запросов с нулевым результатом
	request_queue.AddFindRequest("sparrow"s);
	cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
	return 0;
}