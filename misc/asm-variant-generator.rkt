#lang racket

;; generates the argument type variants for assembly instructions (up to three arguments)

(define OP "move")
(define ARG0 '(gpreg fpreg gptemp fptemp memtemp))
(define ARG1 '(gpreg fpreg gptemp fptemp memtemp const))
(define ARG2 '())

(define (type->predicate type index)
  (case type
    [(gpreg) (format "isGpReg(ir->args[~a])" index)]
    [(fpreg) (format "isFpReg(ir->args[~a])" index)]
    [(gptemp) (format "isGpTemp(ir->args[~a])" index)]
    [(fptemp) (format "isFpTemp(ir->args[~a])" index)]
    [(memtemp) (format "isMemTemp(ir->args[~a])" index)]
    [(const) (format "isConst(ir->args[~a])" index)]))

(let* ([variants (apply cartesian-product
                        (filter (compose not empty?)
                                (list ARG0 ARG1 ARG2)))]
       [predicates (map (λ (variant)
                          (map type->predicate
                               variant
                               (build-list (length variant) identity)))
                        variants)]
       [merged (map (curryr string-join " && ")
                    predicates)]
       [wrapped (string-join merged
                             ") {\n  //TODO\n} else if ("
                             #:before-first "if ("
                             #:after-last (format ") {\n  //TODO\n} else {\n  error(__FILE__, __LINE__, \"unhandled arguments to ~a\");\n}" OP))])
  (displayln wrapped))