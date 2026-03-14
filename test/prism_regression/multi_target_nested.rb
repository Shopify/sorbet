# typed: false
# disable-parser-comparison: true

# Nesting in multi-write
((a, b))     = []
(((a, b)))   = []
((((a, b)))) = []

((a,)), c     = []
(((a,)), c)   = []
(((a, b)), c) = []

a, ((b,))     = []
(a, ((b,)))   = []
(a, ((b, c))) = []

# Nesting in for loops
for ((a, b))     in [] do end
for (((a, b)))   in [] do end
for ((((a, b)))) in [] do end

for ((a,)), c     in [] do end
for (((a,)), c)   in [] do end
for (((a, b)), c) in [] do end

for a, ((b,))     in [] do end
for (a, ((b,)))   in [] do end
for (a, ((b, c))) in [] do end

# Nesting in rescue clauses
def f1((a, b));     end
def f2(((a, b)));   end
def f3((((a, b)))); end

def f4(((a, _)), c);   end
def f5((((a, _)), c)); end
def f6((((a, b)), c)); end

def f7(a, ((b, _)));   end
def f8((a, ((b, _)))); end
def f9((a, ((b, c)))); end
