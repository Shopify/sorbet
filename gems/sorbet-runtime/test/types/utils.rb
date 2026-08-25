# frozen_string_literal: true
# typed: ignore
require_relative '../test_helper'

module Opus::Types::Test
  class UtilsTest < Critic::Unit::UnitTest
    describe 'T::Utils.unwrap_nilable' do
      it 'unwraps when multiple elements' do
        type = T.any(String, NilClass, Float)
        unwrapped = T.must(T::Utils.unwrap_nilable(type))

        assert(T.any(String, Float).subtype_of?(unwrapped))
        assert(unwrapped.subtype_of?(T.any(String, Float)))
      end

      it 'unwraps with a simple pair' do
        type = T.any(String, Float)
        assert_nil(T::Utils.unwrap_nilable(type))
      end
    end

    describe 'T::Utils.signature_for_method' do
      it 'returns nil on methods without sigs' do
        c = Class.new do
          def no_sig; end
        end
        assert_nil(T::Utils.signature_for_method(c.instance_method(:no_sig)))
      end

      it 'returns things on methods with sigs' do
        c = Class.new do
          extend T::Sig
          sig { returns(Integer) }
          def sigfun
            85
          end
        end
        sfm = T::Utils.signature_for_method(c.instance_method(:sigfun))
        refute_nil(sfm)
        assert_equal(:sigfun, sfm.method.name)
        assert_equal(:sigfun, sfm.method_name)
        assert_equal('Integer', sfm.return_type.name)
      end
    end

    describe 'T::Utils.signature_for_instance_method_defined_on' do
      it 'returns the signature hidden by an unsigned prepended method' do
        klass = Class.new do
          extend T::Sig

          sig { returns(String) }
          def foo
            "original"
          end
        end
        prepend_mod = Module.new do
          def foo
            "prepended"
          end
        end
        klass.prepend(prepend_mod)

        assert_nil(T::Utils.signature_for_instance_method(klass, :foo))
        signature = T::Utils.signature_for_instance_method_defined_on(klass, :foo)
        refute_nil(signature)
        assert_equal(klass, signature.method.owner)
        assert_equal('String', signature.return_type.name)
      end

      it 'returns nil when the method is only inherited' do
        parent = Class.new do
          extend T::Sig

          sig { returns(String) }
          def foo
            "parent"
          end
        end
        child = Class.new(parent)

        assert_nil(T::Utils.signature_for_instance_method_defined_on(child, :foo))
      end

      it 'returns nil when the method does not exist' do
        assert_nil(T::Utils.signature_for_instance_method_defined_on(Class.new, :foo))
      end
    end

    describe 'force_type_init' do
      it 'works' do
        Class.new do
          extend T::Sig

          sig { params(x: Integer).returns(String) }
          def foo(x); x.to_s; end

          T::Private::Methods.send(
            :run_sig_block_for_key,
            T::Private::Methods.send(:method_to_key, instance_method(:foo)),
            force_type_init: true
          )
        end
      end
    end

    describe 'run_all_sig_blocks' do
      it 'raises if pending sigs' do
        Module.new do
          extend T::Sig
          sig { void }
        end
        exn = assert_raises(RuntimeError) do
          T::Utils.run_all_sig_blocks
        end
        assert_match(/pending `sig` block in/, exn.message)
      end
    end
  end
end
