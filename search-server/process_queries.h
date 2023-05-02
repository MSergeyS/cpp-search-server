#pragma once

#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer &search_server,
        const vector<string> &queries);

vector<Document> ProcessQueriesJoined(const SearchServer &search_server,
        const vector<string> &queries);
