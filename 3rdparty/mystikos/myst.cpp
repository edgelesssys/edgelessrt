#include <cstdlib>

extern "C" void myst_eraise()
{
}

extern "C" void __myst_panic()
{
    abort();
}
