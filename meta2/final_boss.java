class final_boss {
    public static void main(String[] args) {
        // 1. Unary chaining
        a = + - ! b;
        c = - - 3;
        
        // 2. Complex method calls
        f(a = 3, b = 4);
        
        // 3. Array syntax
        Integer.parseInt(args[1 + 2]);
        
        // 4. Heavy precedence
        a = b = c || d && e == f < g + h * i;
    }
}
