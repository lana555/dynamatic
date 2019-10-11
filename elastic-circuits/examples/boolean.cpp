bool boolean(int n, bool b) {
    if (n == 0)
        return true;
    bool even = n % 2;
    if (even)
        return not b;
    return even and b;
}
