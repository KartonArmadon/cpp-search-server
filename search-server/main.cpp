#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <optional>

using namespace std;

/// <summary>
/// Считывает строку из стандартного потока ввода (std::cin)
/// </summary>
/// <returns>Считанная из потока строка</returns>
string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

/// <summary>
/// <para>Считывает из стандартного потока ввода (std::cin) целочисленное значение</para>
/// <para>Все последующие символы в строке игнорируются</para>
/// </summary>
/// <returns>Считанное из потока число</returns>
int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

/// <summary>
/// <para>Разбивает строку в набор слов</para>
/// <para>В качестве разделителя используется символ пробела</para>
/// </summary>
/// <param name="text"></param>
/// <returns></returns>
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

/// <summary>
/// Преобразует контейнер, содержащий строки, в набор уникальных строк
/// </summary>
/// <typeparam name="StringContainer">Тип-контейнер</typeparam>
/// <param name="strings">Контейнер, содержащий строковые элементы</param>
/// <returns>Множество (не повторяющиеся элементы, отсортированные по возрастанию) строк</returns>
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

/// <summary>
/// <para>Статусы документа:</para>
/// <para>- Актуальный</para>
/// <para>- Неактуальный</para>
/// <para>- Заблокированный</para>
/// <para>- Удалённый</para>
/// </summary>
enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

const int MAX_RESULT_DOCUMENT_COUNT = 5;
class SearchServer {
#pragma region public
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
		for (const auto& word : stop_words_) {
			if (!IsValidWord(word))
				throw invalid_argument("Одно из стоп-слов содержит недопустимые символы");
		}
	}

	explicit SearchServer(const string& stop_words_text)
		: SearchServer(MakeUniqueNonEmptyStrings(SplitIntoWords(stop_words_text)))
	{
	}

	/// <summary>
	/// Добавляет документ в поисковую систему
	/// </summary>
	/// <param name="document_id">Идентификатор документа</param>
	/// <param name="document">Содержимое (текстовое) документа</param>
	/// <param name="status">Статус документа</param>
	/// <param name="ratings">Оценки документа</param>
	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {

		if (document_id < 0) throw invalid_argument("Попытка добавить документ с отрицательным id");

		if (documents_.count(document_id) > 0) throw invalid_argument("Попытка добавить документ с уже существующим id");

		const vector<string> words = SplitIntoWordsNoStop(document);

		for (const string& word : words) {
			if (!IsValidWord(word)) throw invalid_argument("Попытка добавить слово со спец-символами в документ");
		}

		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
		documents_ids_.push_back(document_id);
	}

	/// <summary>
	/// Находит MAX_RESULT_DOCUMENT_COUNT документов, удовлетворяющих запросу по указанной функции-поиска.
	/// </summary>
	/// <typeparam name="DocumentPredicate"></typeparam>
	/// <param name="raw_query">Фраза-запрос для поиска</param>
	/// <param name="document_predicate">Функция-поиска по которой будет производиться отбор документов</param>
	/// <returns>MAX_RESULT_DOCUMENT_COUNT документов, удовлетворяющих условиям поиска</returns>
	template <typename DocumentPredicate>
	vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
		Query query;
		ParseQuery(raw_query, query);
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

	/// <summary>
	/// Находит MAX_RESULT_DOCUMENT_COUNT документов, удовлетворяющих запросу
	/// </summary>
	/// <param name="raw_query">Фраза-запрос для поиска</param>
	/// <param name="status">Среди документов с каким статусом производить поиск</param>
	/// <returns>MAX_RESULT_DOCUMENT_COUNT документов, удовлетворяющих условиям поиска</returns>
	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
		return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
			return document_status == status;
			});
	}

	/// <summary>
	/// Поиск слов из запроса в документе
	/// </summary>
	/// <param name="raw_query">Фраза-запрос</param>
	/// <param name="document_id">Номер документа для поиска</param>
	/// <returns>Пару значений: список слов из запроса, содержащихся в документе; статус документа</returns>
	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
		Query query;
		ParseQuery(raw_query, query);
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
		tuple<vector<string>, DocumentStatus> result = { matched_words, documents_.at(document_id).status };
		return result;
	}

	int GetDocumentCount() const {
		return static_cast<int>(documents_.size());
	}

	int GetDocumentId(int number) {
		if (number >= documents_ids_.size())
			throw out_of_range("В системе меньше документов чем вы думаете!");
		else
			return documents_ids_.at(number);
	}
#pragma endregion
#pragma region private
private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;
	vector<int> documents_ids_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	/// <summary>
	/// Разбивает строку на слова игнорируя стоп-слова
	/// </summary>
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

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	void ParseQuery(const string& raw_query, Query& query) const {
		const vector<string> query_words = SplitIntoWords(raw_query);
		if (!IsValidQuery(query_words)) throw invalid_argument("Фраза-запрос составлена некорректно");

		for (const string& word : query_words) {
			if (!IsValidWord(word)) throw invalid_argument("Слово в запросе содержит спец-символы");
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
	}

	QueryWord ParseQueryWord(string word) const {
		bool is_minus = false;
		if (word[0] == '-') {
			is_minus = true;
			word = word.substr(1);
		}
		return { word, is_minus, IsStopWord(word) };
	}

	/// <summary>
	/// Проверяет запрос на корректность
	/// </summary>
	/// <param name="raw_query">запрос</param>
	/// <returns>false - запрос содержит ошибку; true - запрос корректный</returns>
	bool IsValidQuery(const vector<string>& query_words) const {
		for (const auto& word : query_words) {
			if (word.length() == 1 && word == "-")
				return false;
			if (word.length() > 1)
				if (word[0] == '-' && word[1] == '-')
					return false;
		}
		return true;
	}

	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template <typename DocumentPredicate>
	vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
		map<int, double> document_to_relevance;
		//перебираем все "плюс-слова" из запроса (query)
		for (const string& word : query.plus_words) {
			//если текущего слова (word) нет в "базе" (word_to_document_freqs_)
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			//перебираем все пары [id документа, частота слова в документе] для текущего слова (word)
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

	/// <summary>
	/// Проверяет слово на допустимость. Слово не должно содержать специальных символов
	/// </summary>
	/// <param name="word">Слово для проверки</param>
	/// <returns>false - слово содержит спец. символы; true - слово корректное</returns>
	bool IsValidWord(const string& word) const {
		return none_of(word.begin(), word.end(), [](char ch) {
			return int(ch) >= 0 && int(ch) <= 31;
			});
	}
#pragma endregion
};