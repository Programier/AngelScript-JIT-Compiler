
double factorial(double value)
{
    return value < 1 ? 1 : value * factorial(value - 1);
}

int main()
{
    print(formatFloat(factorial(5)));
    return 0;
}
