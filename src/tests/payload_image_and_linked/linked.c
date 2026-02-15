static int x = 1;

__attribute__((constructor)) void init_enclave()
{
    ++x;
}

int linked_test()
{
    return x;
}
