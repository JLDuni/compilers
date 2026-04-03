class recovery_test {
    public static void main(String[] args) {
        int x = (1 + ); // Erro aqui
        f( , 2);        // Erro aqui
        int y = Integer.parseInt( ); // Erro aqui
        System.out.println(x + y); // Se isto aparecer na AST, a recuperação local funcionou!
    }
}
