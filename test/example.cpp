
class Test
{
    string get_name()
    {
        return "Test";
    }
};

void example()
{
    Test@ test = Test();
    print(test.get_name());
}

int main()
{

    //print(test.get_name());
    example();
    return 0;
}
