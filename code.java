class StringTest {
    // 1. Invalid Escapes
    String a = "\a";       // Invalid: Bell
    String b = "\?";       // Invalid: Question mark
    String c = "\v";       // Invalid: Vertical Tab
    String d = "\0";       // Invalid: Null character (if represented as char)

    // 2. The "Lonely Backslash" (At end of line)
    // This triggers TWO errors: "invalid escape (\)" AND "unterminated string"
    String e = "\;

    // 3. Unterminated Strings (Newline inside)
    String f = "This string
    breaks here";

    // 4. Multiple invalid escapes in one string
    String g = "\a \b \?";

    // 5. Valid escapes mixed with invalid ones
    String h = "Valid:\n Invalid:\a";
}

