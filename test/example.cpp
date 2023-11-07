
int main()
{
    int j = 0;
    for(int i = 0; i < 1000000000; i++)
    {
        ++j;
    }

    print(formatInt(j));
    return 0;
}
