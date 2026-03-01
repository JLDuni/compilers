// 1. Comentário de linha simples
// Este comentário tem /* e */ lá dentro, mas não deve ativar o estado COMMENT

/* 2. Comentário de bloco simples */

/* 3. Comentário de bloco
   que ocupa várias
   linhas e termina aqui -> */

// 4. Teste de operadores colados a comentários
int x = 5; //*****/
int y = 10; /*---*/

/**/ /* 5. Bloco vazio e bloco logo a seguir */

// 6. O caso "traiçoeiro" que vimos antes
//************/ 
/*
  Conteúdo aleatório
**/ 

/* 7. O GRANDE FINAL: Comentário não terminado
   Este comentário começa aqui e nunca fecha.
   O lexer deve reportar o erro na linha onde este /* começou.
