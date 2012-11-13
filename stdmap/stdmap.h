#ifndef __STDMAP

#define __STDMAP

#include <map>
#include <iostream>

template <class KeyType, class ValueType> class STDMap
{
    std::map<KeyType, ValueType>* m;
public:
    STDMap()
    {
        m = new std::map<KeyType,ValueType>;
    }
    void insert(const KeyType& key, const ValueType& value)
    {
        (*m)[key] = value;
        return;
    }
    ValueType* locate(const KeyType& key)
    {
        typename std::map<KeyType,ValueType>::iterator it = m->find(key);
        if((*it).first != key)
        {
            if(it == m->begin())
            {
                return 0;
            }
            --it;
        }        
        return &((*it).second);
    }
    void remove(const KeyType& key)
    {
        m->erase(key);
        return;
    }
    ~STDMap()
    {
        delete m;
    }
};

#endif
