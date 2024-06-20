## · Long Term Goals
+ implement optimizer (`nnc_mio.c`).
+ implement `namespace` support.
+ implement `float` type support.
+ implement copy by value for structs and unions.
+ implement `import` statement.

## · To rethink & reimplement
+ reimplement `nnc_resolve.c`.
    - make enum 4 bytes long. (for compatibility when porting C types and libraries).<br>
        or just add the way to specify type for whole enum.
    - rethink the way implicit casts are working, 
        because now it is painfully to cast everything while coding.
    - inject `implicit_cast` into ast while resolving it.
+ reimplement lexer with DFA instead of NFA. (at least try.)
    - maybe use Flex?
+ reimplement `nnc_3a.c`, make it more pretty and consume less memory. (rename to `nnc_ir.c`).
+ reimplement `nnc_parse.c` (parser), with changed syntax.
    - remove parentesis from condition expressions, e.g:<br>
        `if (expr) { ... }`    ===> `if expr { ... }` or `if expr then stmt`     <br>
        `while (expr) { ... }` ===> `while expr { ... }` or `while expr do stmt` <br>
    add new keywords if needed. (or do not, idk for now, think about it)
    - add another simplified form of for-loop, just for iteration purposes.<br>
        `for (decl; cond; iter) { ... }`      (first form) <br>
        `for decl in n [to|downto] q { ... }` (second form)<br>
            this is can be achieved using first form, but this will 
            not recalculate the bound value, like in first form.<br>
            but this will require additional keywords and logic to be implemented.<br>
        `for n to q { ... }`                  (third form) <br>
            this is just the way to repeat some stuff q - n times,<br>
            without additional variable for storing iteration value.
            q is also precalculated once at the start.
    - replace `~`, `||`, `&&` to identifiers: `not`, `or`, `and`, like in python.

## · Misc
+ combine `deps_impl_x86_64.s` and `intrin_impl_x86_64.s` into one sigle library.
+ remove `./tests` folder inside root.
    - maybe leave it and make more functional tests.
+ update `nnc.ebnf` file. I think at the end of all refactorings it will be deprecated.
+ add good readme.