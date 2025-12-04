# typed: true

extend T::Sig

sig { params(block: T.proc.void).void }
def demo1(&block)
    T.assert_type!(block, T.proc.void)
    T.assert_type!(!!block, T::Boolean)

    if block
        block.call
    end
end


sig { params(block: T.nilable(T.proc.void)).void }
def demo2(&block)
    T.assert_type!(block, T.nilable(T.proc.void))
    T.assert_type!(!!block, T::Boolean)

    if block
        block.call
    end
end


sig { params(block: T.proc.void).void }
def demo3(&block)
    T.assert_type!(block, T.proc.void)
    T.assert_type!(block_given?, TrueClass) # T::Boolean ?!

    if block_given?
        block.call
    end
end


sig { params(block: T.nilable(T.proc.void)).void }
def demo4(&block)
    T.assert_type!(block, T.nilable(T.proc.void))
    T.assert_type!(block_given?, T::Boolean)

    if block_given?
        block.call
    end
end
