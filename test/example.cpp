class Example
{
private string msg;

    Example(const string& in new_msg = "Empty")
    {
        msg = new_msg;
    }

    void set_msg(const string& in new_str)
    {
        msg = new_str;
    }

    string get_msg() const
    {
        return msg;
    }
};


void main()
{
    Example@ e1 = Example();
    Example@ e2 = Example();

    print(e1.get_msg());
    e2.set_msg("Hello");
    print(e2.get_msg());
    @e1 = @e2;
    print(e1.get_msg());
}
