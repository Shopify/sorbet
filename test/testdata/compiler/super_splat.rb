# compiled: true
# typed: true
# frozen_string_literal: true


super(*T.unsafe(ARGS))

ARGS = [1,2,3]
Child.new.foo(*[1,2,3])
