
class Example
{
private string msg;

    Example()
    {
        msg = "Help me, PLEEEEASE!!!";
    }

    string get_msg()
    {
        return msg;
    }
};

int main()
{
    Example@ e = Example();
    print(e.get_msg());
    return 0;
}
