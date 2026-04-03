class comma_test {
    public static void main(String[] args) {
        // 1. The "Ghost Comma" (Missing first argument)
        f( , 3);

        // 2. The "Trailing Comma" (Missing last argument)
        f(3, );

        // 3. The "Double Comma" (Missing middle argument)
        f(1, , 2);
    }
}
