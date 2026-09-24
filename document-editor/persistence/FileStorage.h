#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <fstream>
#include <iostream>
#include "Persistence.h"
using namespace std;

class FileStorage : public Persistence {
public:
    static constexpr const char *DEFAULT_PATH = "document.txt";

    explicit FileStorage(const string &path = DEFAULT_PATH) : path(path) {}

    void save(const string &data) override {
        ofstream outFile(path);
        if (!outFile) {
            cerr << "Error: Unable to open '" << path << "' for writing." << endl;
            return;
        }
        outFile << data;
        cout << "Document saved to " << path << endl;
    }

private:
    string path;
};

#endif // FILE_STORAGE_H
