#include <string>
#include <vector>

#include "search_server.h"

void AddDocument(SearchServer &search_server, const int document_id,
		const string &document, DocumentStatus status,
		const vector<int> &ratings) {
	search_server.AddDocument(document_id, document, status, ratings);
}
