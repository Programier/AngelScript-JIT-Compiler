

void main()
{
    array<string> buffer;
    for(int i = 0; i < 100; i++)
    {
        buffer.push_back("Hello World");
        print(formatInt(buffer.size()));
    }

    for(uint64 i = 0; i < buffer.size(); i++)
    {
        print(buffer[i]);
    }
}
