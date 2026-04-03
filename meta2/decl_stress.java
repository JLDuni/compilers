class decl_stress {
    // 1. Bad Field Commas
    public static int a, , b;
    public static int c, ;

    // 2. Bad Parameter Commas
    public static int f(int a, , int b) { }
    public static int g( , int a) { }
    public static int h(int a, ) { }

    // 3. Incomplete Parameters
    public static int i(int) { }

    // 4. Missing types/names in fields
    public static int ;
}
