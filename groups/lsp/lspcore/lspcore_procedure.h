#ifndef INCLUDED_LSPCORE_PROCEDURE
#define INCLUDED_LSPCORE_PROCEDURE

namespace lspcore {

class Procedure {
    // TODO
};

/*
invoke procedure positionals rest environment:
top: for (;;) {
    env = new env (bind arguments to parameters)
    form = body;
    while form.second != null {
        evaluate(form, env)
        form = form.second
    }

    // last one
    for (;;) { // for nested conditionals
        switch (classify(form, env)) {
        case invocation:
            procedure = ...
            positionals = ...
            goto top
        case conditional:
            form = evaluate(predicate) ? ... : ...
            break
        default:
            return evaluate*(form, env)
        }
    }
}
*/

}  // namespace lspcore

#endif
