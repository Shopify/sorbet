#!/usr/bin/env ruby
# frozen_string_literal: true

# Usage: ruby tools/scripts/diff_prism_exp.rb test/testdata/desugar/file.rb
#
# For each .prism.exp file associated with the given .rb test file,
# opens a diff against the corresponding .exp file.

require "optparse"

color = true
OptionParser.new do |opts|
  opts.banner = "Usage: #{$0} [options] <test_file.rb>"
  opts.on("--no-color", "Disable color output") { color = false }
end.parse!

abort "Usage: #{$0} [options] <test_file.rb>" if ARGV.empty?

rb = ARGV[0]
abort "File not found: #{rb}" unless File.exist?(rb)

diff_tool = `git config diff.tool`.strip
if diff_tool.empty?
  diff_tool = ENV["DIFFTOOL"] || "diff"
end

base_command = [diff_tool]
if color
  case File.basename(diff_tool)
  when "difft", "delta"
    base_command << "--color=always"
  when "diff", "colordiff"
    base_command << "--color"
  when "icdiff"
    # color is on by default, no flag needed
  when "vimdiff", "opendiff", "kdiff3", "meld"
    # interactive tools handle color themselves
  else
    base_command << "--color=always"
  end
end

dir = File.dirname(rb)
base = File.basename(rb)

prism_exps = Dir.glob("#{dir}/#{base}.*.prism.exp")
abort "No .prism.exp files found for #{rb}" if prism_exps.empty?

prism_exps.sort.each do |prism_exp|
  regular_exp = prism_exp.sub(".prism.exp", ".exp")
  unless File.exist?(regular_exp)
    puts "Skipping #{prism_exp} (no regular .exp)"
    next
  end

  puts "#{File.basename(regular_exp)} vs #{File.basename(prism_exp)}"

  system(*base_command, regular_exp, prism_exp)
end
