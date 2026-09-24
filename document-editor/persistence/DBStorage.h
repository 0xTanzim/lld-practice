#ifndef DB_STORAGE_H
#define DB_STORAGE_H

#include <iostream>
#include "Persistence.h"
using namespace std;

class DBStorage : public Persistence {
public:
    void save(const string &data) override {
        (void)data;
        cerr << "DBStorage: persistence not implemented yet." << endl;
    }
};

#endif // DB_STORAGE_H
