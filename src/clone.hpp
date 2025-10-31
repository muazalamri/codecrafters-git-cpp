#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <curl/curl.h>

using namespace std;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// get_refs: fetch remote refs info
string get_refs(const string& base_url) {
    string url = base_url + "/info/refs?service=git-upload-pack";
    string response;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) throw runtime_error("Failed to fetch refs");
    return response;
}

// parse_refs: extract commit hash of HEAD
string parse_head_hash(const string& data) {
    stringstream ss(data);
    string line;
    while (getline(ss, line)) {
        if (line.find("refs/heads/") != string::npos) {
            size_t space = line.find(' ');
            if (space == string::npos) continue;
            string hash = line.substr(4, space - 4); // skip pkt-len
            return hash;
        }
    }
    throw runtime_error("No head found");
}


// make pkt-line body for POST
string make_pkt(const string& commit_hash) {
    string want = "want " + commit_hash + " side-band-64k\n";
    char len[5];
    sprintf(len, "%04x", (int)(want.size() + 4));
    string pkt = string(len) + want + "0000";
    return pkt;
}

// send POST request for pack
string request_pack(const string& base_url, const string& body) {
    string url = base_url + "/git-upload-pack";
    string response;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-git-upload-pack-request");
    headers = curl_slist_append(headers, "Accept: application/x-git-upload-pack-result");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) throw runtime_error("Failed to request pack");
    return response;
}

// extract pack data (channel 1 only)
vector<unsigned char> extract_pack_data(const string& response) {
    vector<unsigned char> data;
    size_t i = 0;
    while (i + 5 < response.size()) {
        unsigned char chan = response[i + 4];
        if (chan == 1) {
            data.push_back(response[i + 5]);
        }
        i++;
    }
    return data;
}

// main clone logic
void git_clone(const string& remote_url, const string& dir) {
    std::filesystem::create_directories(dir + "/.git");
    string refs = get_refs(remote_url);
    string head_hash = parse_head_hash(refs);
    string pkt = make_pkt(head_hash);
    string response = request_pack(remote_url, pkt);

    // Write raw response to simulate packfile
    ofstream out(dir + "/.git/pack-result.bin", ios::binary);
    out.write(response.c_str(), response.size());
    out.close();

    // Simple HEAD file
    ofstream head(dir + "/.git/HEAD");
    head << "ref: refs/heads/main\n";
    head.close();

    cout << "Cloned " << remote_url << " into " << dir << endl;
}