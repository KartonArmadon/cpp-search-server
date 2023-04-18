#include <iostream>
#include <math.h>
#include <algorithm>
#include <utility>

#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result = 0;
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
};

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document) {
		const vector<string> words = SplitIntoWordsNoStop(document);
		double inverse_size = 1.0 / static_cast<double>(words.size());
		for (const auto& item : words) {
			//word_to_documents_[item].insert(document_id);
			word_to_document_freqs_[item][document_id] += inverse_size;
		}
		++document_count_;
	}

	vector<Document> FindTopDocuments(const string& raw_query) const {
		const Query query_words = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query_words);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return lhs.relevance > rhs.relevance;
			});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

private:
	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	map<string, map<int, double>> word_to_document_freqs_; //слово, (id-документа, TF)

	set<string> stop_words_;

	int document_count_ = 0;

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

	Query ParseQuery(const string& text) const {
		Query query_words = {};
		for (const string& word : SplitIntoWordsNoStop(text)) {
			if (word[0] == '-')
				query_words.minus_words.insert(word.substr(1));
			else
				query_words.plus_words.insert(word);
		}
		return query_words;
	}

	double CalculateIDF(const string& word) const {
		double count_docs_with_word = static_cast<double>(word_to_document_freqs_.at(word).size()); //количество документов с заданным словом
		double idf = log(static_cast<double>(document_count_) / count_docs_with_word); //idf слова в запросе
		return idf;
	}

	vector<Document> FindAllDocuments(const Query& query_words) const {
		vector<Document> matched_documents = {};
		if (query_words.plus_words.size() == 0) {
			return matched_documents;
		}

		//id, tf-idf;
		map<int, double> id_relevance = {};
		for (const auto& item : query_words.plus_words) {
			if (word_to_document_freqs_.count(item) == 0)
				continue;

			double idf = CalculateIDF(item);
			for (const auto& [id, tf] : word_to_document_freqs_.at(item)) {
				id_relevance[id] += static_cast<double>(tf) * idf;
			}

		}
		for (const auto& item : query_words.minus_words) {
			if (word_to_document_freqs_.count(item) > 0)
				for (const auto& [id, tf] : word_to_document_freqs_.at(item)) {
					id_relevance.erase(id);
				}
		}
		for (const auto& [id, relevance] : id_relevance) {
			matched_documents.push_back({ id, relevance });
		}
		return matched_documents;
	}
};

SearchServer CreateSearchServer() {
	SearchServer search_server;
	search_server.SetStopWords(ReadLine());

	const int document_count = ReadLineWithNumber();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		search_server.AddDocument(document_id, ReadLine());
	}
	return search_server;
}

int main() {
	const SearchServer search_server = CreateSearchServer();

	const string query = ReadLine();
	for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
		cout << "{ document_id = "s << document_id << ", "
			<< "relevance = "s << relevance << " }"s << endl;
	}
}