// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <iostream>
#include <stdexcept>

using namespace std;

int main()
{
    cout << "hello stdc++\n";

    try
    {
        throw logic_error("exception test");
    }
    catch (const exception& e)
    {
        cout << e.what() << '\n';
    }

    return 0;
}
