class expr_stress {
    public static void main(String[] args) {
        // 1. Invalid Left-Hand Side Assignments
        1 = a;
        f() = 3;
        (a) = 4;

        // 2. Bad Method Calls
        f(1, , 2);
        f(int a);

        // 3. Bad Dot Lengths
        (a+b).length;
        1.length;
        f().length;

        // 4. Bad Integer.parseInt
        Integer.parseInt();
        Integer.parseInt(args[]);
        Integer.parseInt(1);

        // 5. Empty/Missing Expressions
        c = ();
        a = * 3;
        b = 4 + ;
        
        // 6. Misplaced operators
        a = a ! b;
    }
}
