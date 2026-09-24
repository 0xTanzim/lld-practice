#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <string>
using namespace std;

class Persistence {
public:
    virtual void save(const string &data) = 0;
    virtual ~Persistence() = default;
};

#endif // PERSISTENCE_H
