# frozen_string_literal: true
# typed: false

module T::Private::CallerUtils
  def self.find_caller(&block)
    Kernel.caller_locations.drop(1).find do |loc|
      !loc.path&.start_with?("<internal:") && block.call(loc)
    end
  end
end
