#include<iostream>
#include<map>
#include<memory>
#include<cstdio>
class test
{
public:
    test()
    {
        value = 0;
    }
private:
    int value;
};

int main(int argc,char **argv)
{
    std::map<test,int> myMap;
    std::shared_ptr<int> data = std::make_shared<int>(10);
    std::shared_ptr<test> t = std::make_shared<test>();
    myMap[t] = data;
    for(auto i : myMap)
        printf("%d %d\n",i.first,myMap[i.first]);

    return 0;
}
