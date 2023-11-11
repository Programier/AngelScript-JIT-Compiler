

class User
{
private string name;

    User(const string& in username = "Test")
    {
        name = username;
    }

    void set_username(const string& in new_name)
    {
        name = new_name;
    }

    string get_username() const
    {
        return name;
    }
};


int main()
{
    User@ user;

    filesystem fs;
    print(fs.getCurrentPath());
    return 0;
}
