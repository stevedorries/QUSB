#include <iostream>
#include "../KaerHid/kaerhid.h"

using namespace std;

int main()
{
    cout <<"aaaaa !"<<endl;
    char buffer[1024];
    int r = getSamvId(buffer);
    cout << "SAMV " << r;
    return 0;
}

