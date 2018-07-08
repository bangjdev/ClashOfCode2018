#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>

using namespace std;

string working_path, filepath;
vector <string> patterns;
int counter = 0, counter_scanned = 0, counter_skipped = 0;
clock_t start = clock();
static const int BUFF_SIZE = 16777216;
char buffer[BUFF_SIZE + 5];


// A search engine abstract class, can be used to add many search algorithm to improve speed
class SearchEngine {
public:
    virtual bool do_search(ifstream &ifs) = 0;
};

// SearchEngine using KMP algorithm
class KMPEngine: public SearchEngine {
private:
    int *ptable, n, m;
    string pattern;
public:
    KMPEngine(string &pattern) {
        this->pattern = pattern;
        n = pattern.length();
        pre_compute();
    }
    void pre_compute() {
        int i = 0, j = 1;
        ptable = new int[n + 1];
        ptable[0] = 0;
        while (j < n) {
            if (pattern[j] == pattern[i]) {
                ptable[j] = i + 1;
                i ++;
                j ++;
            } else {
                if (i > 0)
                    i = ptable[i - 1];
                else {
                    ptable[j] = 0;
                    j ++;
                }
            }
        }
    }
    bool is_character(char c) {
        return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_');
    }
    bool do_search(ifstream &working_file) {
        if (!working_file.is_open()) {
            counter_skipped ++;
            return false;
        }
        bool flag_eof = false;
        working_file.read(buffer, BUFF_SIZE);
        m = working_file.gcount();
        if (m < BUFF_SIZE) {
            flag_eof = true;
            buffer[m] = ' ';
            m ++;
        }
        int i = 0, j = 0;
        if (buffer[0] == pattern[1])
            j = 1;
        while ((i < m) && (j < n)) {
            if (!is_character(buffer[i]))
                buffer[i] = ' ';
//	    cout << "Compare |" << (int) buffer[i] << "|" << pattern[j] << "\n";
            if (buffer[i] == pattern[j]) {
                i ++;
                j ++;
            } else {
                if (j > 0)
                    j = ptable[j - 1];
                else
                    i ++;
            }
            if (j == n)
                return true;
            if (i >= m) {
                if (!flag_eof) {
                    working_file.read(buffer, BUFF_SIZE);
                    m = working_file.gcount();
                    if (m < BUFF_SIZE) {
                        flag_eof = true;
                        buffer[m ++] = ' ';
                    }
                    i = 0;
                }
            }
        }
        return false;
    }
};


/*
    DirectoryHandler helper class
    Initialize with file path, and use do_grep() to start matching using list of engines
*/

class DirectoryHandler {
    private:
        string filepath;
        vector <SearchEngine*> search_engine;
        ifstream ifs;
    public:
        DirectoryHandler(string filepath) {
            this->filepath = filepath;
            search_engine.clear();
        }
        void add_search_engine(SearchEngine *se) {
            this->search_engine.push_back(se);
        }
        void do_grep() {
            recursive_traversal(filepath);
        }
        void recursive_traversal(string current_path) {
            current_path += "/";
            DIR *dir;
            if ((dir = opendir(current_path.c_str())) == NULL)
                return;
            struct dirent *current_point;
            while ((current_point = readdir(dir)) != NULL) {
                if (current_point->d_type == DT_DIR) {
                    if ((strcmp(current_point->d_name, ".") == 0) || (strcmp(current_point->d_name, "..") == 0))
                        continue;
                    recursive_traversal(current_path + current_point->d_name);
                } else {
//                    cout << "Checking " << current_path + current_point->d_name << "\n";
                    ifs.open((current_path + current_point->d_name).c_str(), std::ios::binary);
                    counter_scanned ++;
                    bool found = true;
                    for (int i = 0; i < search_engine.size(); i ++) {
                        ifs.clear();
                        ifs.seekg(0);
                        if (!search_engine[i]->do_search(ifs)) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        cout << current_path + current_point->d_name << "\n";
                        counter ++;
                    }

                    ifs.close();
                }
            }
            closedir(dir);
        }

};

// This method split given string into words

void extract(char s[], vector<string> &result) {
    result.clear();
    string temp;
    int len = strlen(s);
    for (int i = 0; i < len; i ++) {
        if (s[i] != ' ') {
            temp = " ";
            while ((i < len) && (s[i] != ' ')) {
                temp += s[i];
                i ++;
            }
            temp += " ";
            result.push_back(temp);
        }
    }
}

int main(int argc, char *argv[]) {
//    freopen("log.txt", "w", stdout);
    switch (argc) {
        case 1: {
            cout << "Error! 0 given word found";
            return 0;
        }
            break;
        case 2:
            working_path = ".";
            break;
        default:
            working_path = argv[2];
            break;
    }
    extract(argv[1], patterns);

    DirectoryHandler dhand = DirectoryHandler(working_path);
    for (string s : patterns)
        dhand.add_search_engine(new KMPEngine(s));

    dhand.do_grep();

    cout << "Searching completed after " << (float) (clock() - start) / CLOCKS_PER_SEC << "s [";
    cout << "scanned " << counter_scanned << " file" << (counter_scanned>1?"s, ":", ");
    cout << "skipped " << counter_skipped << " file" << (counter_skipped>1?"s, ":", ");
    cout << "found " << counter << " file" << (counter>1?"s]\n":"]\n");

    return 0;
}

