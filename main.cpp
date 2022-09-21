#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
};


struct SearchQuery
{
    set<string> plus_words ;
    set<string> minus_words ;
};

SearchQuery search_query ;


map<string, map<int, double>> word_to_document_freqs_ ;


class SearchServer
{
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const   // получает сплошную строчку слов
    {
        const set<string> local_plus_words = ParseQuery(raw_query);          // пилит их на отдельные слова и делает из них упорядоченный вектор сет

        auto matched_documents = FindAllDocuments(local_plus_words);         // отправляет этот сет в функцию поиска всех совпаюащих доков (релевантность выше 0)

        sort(matched_documents.begin(), matched_documents.end(), [](const auto& lhs, const auto& rhs)    // сортирует полученный ВЕКТОР <ид релевантность> по релевантности
                                                                        {
                                                                            return lhs.relevance > rhs.relevance;
                                                                        }                                                   );


        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT) ;  // отпиливает лишние куски вектора
        }
        return matched_documents;
    }

    void AddDocument(int document_id, const string& document) // получает ид и сплошную строку документа
     {


        ++document_count_ ;

        const vector<string> words = SplitIntoWordsNoStop(document); // формируем вектор из распиленных слов ОДНОГО ДОКУМЕНТА

        for ( const string& word : words)   // идём по этому вектору со словами ОДНОГО ДОКУМЕНТА

        {
            word_to_documents_[word].insert(document_id) ;

                // добавляем в мап доков [это_слово], а в сет ид доков этого слова .insert(ид текущего документа)



              word_to_document_freqs_[word][document_id] += ( 1.0 / words.size() ) ;  // добавляем в мап с релевантностью слов в ключ этого слова \ ключ ид документа \ частоту встречания этого слова
        }
     }

private:

    set<string> stop_words_;

    map<string, set<int>> word_to_documents_ ;                 // мап каталога слов документа

       // мап из ключ:[слова запроса] - значение:( MAP (ид дока , TF этого слова в доке)

    int document_count_ = 0;   // общее кол-во документов

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text) const
    {



        for (const string& current_word : SplitIntoWordsNoStop(text))
        {

            if ( current_word[0] == '-' )
            {
                search_query.minus_words.insert(current_word.substr(1));
            }

            else
            {
                search_query.plus_words.insert(current_word);
            }

        }

        return search_query.plus_words;
    }

    vector<Document> FindAllDocuments(const set<string>& local_plus_words) const   // надо сформировать мап по СЛОВАМ ЗАПРОСА из мапов по ид дока и IDF ЭТОГО СЛОВА в этом доке

    {
        map<int, double> document_to_relevance ; // здесь будут несортированные совпадающие доки - <ид, релевантность>

        vector<Document> matched_docs ;


        for ( auto& search_query_word : local_plus_words)    // идём по ВЕКТОРУ слов поискового запроса (тут только плюс-слова, )

        {



                if ( word_to_documents_.count(search_query_word) != 0 )  // ищем это слово в ключах мапа доков

                {

                    for (  int  id_docs_for_word : word_to_documents_.at(search_query_word) )


                    {
                         // =  log ( document_count_ / word_to_documents_.at(search_query_word).size()  )      ;  // пихаем ид-шники доков в которых оно есть в мап по вычислению релевантности на место ключа для ключа слова

                        document_to_relevance[id_docs_for_word] += ( word_to_document_freqs_[search_query_word][id_docs_for_word] * log ( document_count_ * 1.0 / (word_to_documents_.at(search_query_word).size() ) ) ) ;



                    }





                }
                // получаем в итоге мап из ключ:ид \ значение:релевантность
        }


        for (const auto& search_query_word : search_query.minus_words ) // идём по минус словам

        {
           if ( word_to_documents_.count(search_query_word) != 0 )  // ищем minus слово в ключах мапа доков

                {
                    for ( auto doc_id : word_to_documents_.at(search_query_word) )
                    {
                            document_to_relevance[doc_id]  = 0   ;     // идём по сету ключа и берём каждый ид, суём в map<int, int> - { ид, релевантность +1 }
                    }
                }
        }



          for ( auto [ doc_pos_id , doc_pos_rel ] : document_to_relevance )

                {
                    if ( doc_pos_rel ) matched_docs.push_back({doc_pos_id , doc_pos_rel}) ;


                }


        return matched_docs ;

    }



};

SearchServer CreateSearchServer()
{
    SearchServer search_server;

    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();



    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());

    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();



    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl  ;
    }


}
