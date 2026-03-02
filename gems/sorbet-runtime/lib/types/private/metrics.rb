# frozen_string_literal: true
# typed: true

module T::Private::Metrics
  @counters = {}
  @handler = nil

  COUNTER_NAMES = [
    'sorbet.runtime.method_wrappers_created',
    'sorbet.runtime.type_checked_calls',
    'sorbet.runtime.abstract_method_calls',
    'sorbet.runtime.abstract_method_raises',
    'sorbet.runtime.abstract_class_instantiations',
    'sorbet.runtime.abstract_class_raises',
  ].freeze

  def self.increment(name)
    @counters[name] = (@counters[name] || 0) + 1
  end

  def self.handler=(handler)
    @handler = handler
  end

  def self.counters
    @counters.dup
  end

  def self.report!
    return unless @handler
    tags = {}
    build_id = ENV['BUILDKITE_BUILD_ID']
    tags['build_id'] = build_id if build_id
    @handler.call(@counters.dup, tags)
  end

  def self.reset!
    @counters.clear
  end
end
