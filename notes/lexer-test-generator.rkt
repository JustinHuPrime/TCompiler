#lang racket

(define TEMPLATE "\n\n  tokenType = lex(report, info, &tokenInfo);\n  test(status, \"[lexer] [lex] comprehensive file token ~a is ~a\",\n       tokenType == ~a);\n  test(status, \"[lexer] [lex] comprehensive file token ~a is at line ~a\",\n       tokenInfo.line == ~a);\n  test(status, \"[lexer] [lex] comprehensive file token ~a is at char ~a\",\n       tokenInfo.character == ~a);")

(define (gen-tests token-number token-name token-line token-char)
  (displayln (string-join (build-list 40 (const "-")) ""))
  (displayln "")
  (displayln "")
  (displayln "  tokenType = lex(report, info, &tokenInfo);")
  (displayln (format "  test(status, \"[lexer] [lex] comprehensive file token ~a is ~a\","
                     token-number token-name))
  (displayln (format "       tokenType == TT_~a);"
                     (list->string (map char-upcase (string->list token-name)))))
  (displayln (format "  test(status, \"[lexer] [lex] comprehensive file token ~a is at line ~a\","
                     token-number token-line))
  (displayln (format "       tokenInfo.line == ~a);"
                     token-line))
  (displayln (format "  test(status, \"[lexer] [lex] comprehensive file token ~a is at char ~a\","
                     token-number token-char))
  (displayln (format "       tokenInfo.character == ~a);"
                     token-char))
  (displayln (string-join (build-list 40 (const "-")) "")))

(define g gen-tests)
