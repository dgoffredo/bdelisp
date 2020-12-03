bdelisp
=======
```racket
(λ (bde) 
 "\( ﾟヮﾟ)/")
```

Why
---
Why not? I see [bdld_datum][1], I see [allocators][2], let's try it.

What
----
`bdelisp` is an implementation of a [scheme][3]-like language in the [lisp][4]
family. It is implemented as a C++ library using (only) the [BDE][5] suite of
libraries.

It is, if nothing else, a complete and concise serialization format for
[bdld::Datum][1] objects.

```racket
(define things {
  "quoted symbol" 'bar
  "array" [1 2 "three" 4L]
  "when" 2020-12-02T23:45:33
  "decimals" [34.5 -2.33 231.4e10]
  "doubles" [34.5B -2.33B 231.4e10B]
  ; this is commented out
  "nest comments" #;(like "this") "the value"
  "int maps" {4 "four" 5 "five"}
  "error literals" #error[42 "something went wrong"]
  "how long" #P1W ; that means one week
})

(print "we've got lists:" '(x y z))
(print "we've got pairs:" (pair [1 2 3] "foo"))
(print "even special syntax:" '([1 2 3] . "foo"))
(print "nil and the empty list are the same:" '())

(assert-equal (datetime-minute (things "when")) 45)

(define THINGS
  (map
    (λ (key value)
      (pair (string-upcase key) value))
    things))

; recur to your heart's content...
(print "Here's a silly way to calculate the constant 9999:"
  (let loop ([countdown 9999] [answer 0])
    (if (zero? countdown)
      answer
      (loop (- countdown 1) (+ answer 1)))))
```

How
---
Nothing yet. Here's a roadmap:

1. AST-walking interpreter for a minimal (unbearable) language without
   macros, but with proper tail calls.
2. [syntax-case][6] style hygienic macros.
3. First-class continuations, e.g. [call-with-composable-continuation][8].
4. Better garbage collection. My initial plan is a "copy-and-sweep" strategy
   using two [managed allocators][7].
5. Bytecode compiler.

We'll see what I get to.

[1]: https://github.com/bloomberg/bde/blob/master/groups/bdl/bdld/bdld_datum.h
[2]: https://github.com/bloomberg/bde/tree/master/groups/bdl/bdlma
[3]: https://en.wikipedia.org/wiki/Scheme_(programming_language)
[4]: https://en.wikipedia.org/wiki/Lisp_(programming_language)
[5]: https://github.com/bloomberg/bde
[6]: https://www.gnu.org/software/guile/manual/html_node/Syntax-Case.html
[7]: https://github.com/bloomberg/bde/blob/master/groups/bdl/bdlma/bdlma_managedallocator.h
[8]: https://docs.racket-lang.org/guide/conts.html
