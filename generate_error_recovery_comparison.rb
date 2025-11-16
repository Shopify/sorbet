#!/usr/bin/env ruby
# frozen_string_literal: true

=begin
Generate error recovery comparison document for Prism vs Original parsers.

This script generates a JSON data file first, then renders markdown from it.
This allows for partial re-runs of specific phases or files.

Usage:
  ruby generate_error_recovery_comparison.rb --all                    # Generate everything
  ruby generate_error_recovery_comparison.rb --file assign.rb         # Process single file
  ruby generate_error_recovery_comparison.rb --phase parsers          # Re-run parser phase only
  ruby generate_error_recovery_comparison.rb --phase autocorrects     # Re-run autocorrects only
  ruby generate_error_recovery_comparison.rb --phase assessments      # Re-run Claude assessments only
  ruby generate_error_recovery_comparison.rb --render                 # Regenerate markdown from JSON

Environment variables:
  OPENAI_API_BASE - Base URL for OpenAI-compatible API (should point to Claude proxy)
  OPENAI_API_KEY - API key for authentication
=end

require 'net/http'
require 'uri'
require 'json'
require 'fileutils'
require 'tempfile'

# Configuration
SORBET_BINARY = "./bazel-bin/main/sorbet"
TEST_DIR = "test/testdata/parser/error_recovery"
JSON_FILE = "error_recovery_data.json"
OUTPUT_FILE = "error_recovery_comparison.md"
CLAUDE_MODEL = "claude-sonnet-4-5"

def run_sorbet(file_path, parser_mode, autocorrect: false)
  cmd = [SORBET_BINARY, "--silence-dev-message"]
  cmd << "-a" if autocorrect
  cmd << "--print=desugar-tree" unless autocorrect
  cmd << "--parser=#{parser_mode}"
  cmd << file_path

  output = `#{cmd.join(' ')} 2>&1`
  success = $?.exitstatus == 0
  [output, success]
rescue => e
  ["Error: #{e.message}", false]
end

def split_output(output)
  lines = output.split("\n")

  # Find where the tree starts (first line with "class <emptyTree>")
  tree_start = lines.index { |line| line.start_with?('class <emptyTree>') }

  if tree_start.nil?
    # No tree found, everything is errors
    return [output.strip, ""]
  end

  errors = lines[0...tree_start].join("\n").strip
  tree = lines[tree_start..-1].join("\n").strip

  [errors, tree]
end

def extract_error_count(errors)
  return 0 if errors.include?("No errors! Great job.")

  match = errors.match(/Errors?: (\d+)/)
  match ? match[1].to_i : 0
end

def count_autocorrects(errors)
  errors.scan(/Autocorrect: Use `-a`/).length
end

def calculate_tree_similarity(tree1, tree2)
  return 1.0 if tree1 == tree2
  return 0.0 if tree1.empty? || tree2.empty?

  # Simple character-based similarity
  max_len = [tree1.length, tree2.length].max
  return 0.0 if max_len == 0

  matches = 0
  tree1.chars.zip(tree2.chars).each do |c1, c2|
    matches += 1 if c1 == c2
  end

  matches.to_f / max_len
end

def get_tree_similarity_emoji(similarity)
  if similarity >= 1.0
    '🟢'
  elsif similarity >= 0.70
    '🟡'
  else
    '🔴'
  end
end

def generate_tree_diff(tree1, tree2)
  return "One or both trees are empty" if tree1.empty? || tree2.empty?
  return "Trees are identical" if tree1 == tree2

  # Write to temp files and use diff
  Tempfile.create(['original', '.txt']) do |f1|
    Tempfile.create(['prism', '.txt']) do |f2|
      f1.write(tree1)
      f1.flush
      f2.write(tree2)
      f2.flush

      diff = `diff -u #{f1.path} #{f2.path} 2>/dev/null`

      if diff.empty?
        "Trees are identical"
      else
        # Clean up the diff to use Original/Prism labels
        lines = diff.split("\n")
        if lines.length >= 2
          lines[0] = "--- Original"
          lines[1] = "+++ Prism"
        end
        # Filter out "No newline at end of file" markers
        lines = lines.reject { |line| line.start_with?('\\') }
        lines.join("\n")
      end
    end
  end
end

def call_claude_api(prompt)
  api_base = ENV['OPENAI_API_BASE']
  api_key = ENV['OPENAI_API_KEY']

  raise "OPENAI_API_BASE not set" unless api_base
  raise "OPENAI_API_KEY not set" unless api_key

  uri = URI.parse("#{api_base}/chat/completions")

  request_body = {
    model: CLAUDE_MODEL,
    messages: [
      { role: "user", content: prompt }
    ],
    temperature: 0.2,
    max_tokens: 200
  }

  http = Net::HTTP.new(uri.host, uri.port)
  http.use_ssl = (uri.scheme == 'https')
  http.verify_mode = OpenSSL::SSL::VERIFY_NONE if http.use_ssl?
  http.read_timeout = 60

  request = Net::HTTP::Post.new(uri.path)
  request['Content-Type'] = 'application/json'
  request['Authorization'] = "Bearer #{api_key}"
  request.body = request_body.to_json

  response = http.request(request)

  unless response.is_a?(Net::HTTPSuccess)
    raise "API request failed: #{response.code} #{response.message}\n#{response.body}"
  end

  result = JSON.parse(response.body)
  result['choices'][0]['message']['content']
end

def assess_error_quality_with_claude(filename, input_code, original_errors, prism_errors)
  prompt = <<~PROMPT
    You are analyzing parser error quality for the Sorbet Ruby type-checker. We're comparing error recovery between the Original parser (Whitequark fork) and the new Prism parser.

    Test file: #{filename}

    Input Ruby code:
    ```ruby
    #{input_code}
    ```

    Original errors:
    ```
    #{original_errors}
    ```

    Prism errors:
    ```
    #{prism_errors}
    ```

    Please assess the QUALITY and ACCURACY of the Prism errors compared to the Original errors. Focus on:
    1. Are the error locations accurate and point to the actual problem?
    2. Are the error messages clear and helpful?
    3. Does Prism catch the same issues as Original (or better)?
    4. Overall, are Prism's errors as good or better than Original's?

    Respond with ONLY a JSON object in this exact format:
    {
      "emoji": "🟢" or "🟡" or "🔴",
      "assessment": "Brief one-line description of the quality assessment"
    }

    Guidelines:
    - 🟢 (Green): Prism errors are good/excellent - accurate locations, clear messages, catches issues well
    - 🟡 (Yellow): Prism errors are acceptable but have minor issues - maybe points to EOF instead of problem line, or messages differ but still useful
    - 🔴 (Red): Prism errors have problems - misses errors that Original catches, or very inaccurate

    Be concise in the assessment (one sentence, no markdown formatting).
  PROMPT

  begin
    result_text = call_claude_api(prompt)

    # Extract JSON from response
    json_match = result_text.match(/\{.*\}/m)
    if json_match
      result = JSON.parse(json_match[0])
      return [result['emoji'], result['assessment']]
    else
      puts "Warning: Could not parse Claude response for #{filename}: #{result_text}"
      return ['🟡', 'LLM quality assessment unavailable']
    end
  rescue => e
    puts "Error calling Claude API for #{filename}: #{e.message}"
    return ['🟡', "Error during assessment: #{e.message}"]
  end
end

def load_data
  if File.exist?(JSON_FILE)
    JSON.parse(File.read(JSON_FILE))
  else
    {}
  end
end

def save_data(data)
  File.write(JSON_FILE, JSON.pretty_generate(data))
end

def run_parsers_for_file(test_file, filename, data)
  puts "  - Running parsers..."

  input_code = File.read(test_file)

  original_output, _ = run_sorbet(test_file, "original")
  prism_output, _ = run_sorbet(test_file, "prism")

  original_errors, original_tree = split_output(original_output)
  prism_errors, prism_tree = split_output(prism_output)

  original_error_count = extract_error_count(original_errors)
  prism_error_count = extract_error_count(prism_errors)
  original_autocorrects = count_autocorrects(original_errors)
  prism_autocorrects = count_autocorrects(prism_errors)

  tree_similarity = calculate_tree_similarity(original_tree, prism_tree)
  tree_emoji = get_tree_similarity_emoji(tree_similarity)
  tree_diff = generate_tree_diff(original_tree, prism_tree)

  data[filename] ||= {}
  data[filename].merge!({
    'filename' => filename,
    'input_code' => input_code,
    'original_errors' => original_errors,
    'original_error_count' => original_error_count,
    'original_autocorrects' => original_autocorrects,
    'prism_errors' => prism_errors,
    'prism_error_count' => prism_error_count,
    'prism_autocorrects' => prism_autocorrects,
    'original_tree' => original_tree,
    'prism_tree' => prism_tree,
    'tree_similarity' => tree_similarity,
    'tree_emoji' => tree_emoji,
    'tree_diff' => tree_diff
  })

  puts "  ✓ Parsers complete"
end

def run_autocorrects_for_file(test_file, filename, data)
  return unless data[filename]
  return unless data[filename]['prism_autocorrects'] && data[filename]['prism_autocorrects'] > 0

  puts "  - Running autocorrect..."

  input_code = data[filename]['input_code']

  Tempfile.create([filename, '.rb']) do |temp_file|
    FileUtils.cp(test_file, temp_file.path)

    autocorrect_output, autocorrect_success = run_sorbet(temp_file.path, "prism", autocorrect: true)

    original_code = input_code
    autocorrected_code = File.read(temp_file.path)

    if original_code != autocorrected_code
      Tempfile.create(['orig', '.rb']) do |orig|
        Tempfile.create(['auto', '.rb']) do |auto|
          orig.write(original_code)
          orig.flush
          auto.write(autocorrected_code)
          auto.flush

          diff = `diff -u #{orig.path} #{auto.path} 2>/dev/null`
          unless diff.empty?
            lines = diff.split("\n")
            if lines.length >= 2
              lines[0] = "--- Original"
              lines[1] = "+++ Autocorrected"
            end
            # Filter out "No newline at end of file" markers
            lines = lines.reject { |line| line.start_with?('\\') }
            data[filename]['autocorrect_code_diff'] = lines.join("\n")
          end
        end
      end
    else
      data[filename]['autocorrect_code_diff'] = "No changes made to code"
    end
  end

  puts "  ✓ Autocorrect complete"
end

def run_assessment_for_file(filename, data)
  return unless data[filename]

  puts "  - Calling Claude API..."

  error_emoji, error_assessment = assess_error_quality_with_claude(
    filename,
    data[filename]['input_code'],
    data[filename]['original_errors'],
    data[filename]['prism_errors']
  )

  data[filename]['error_emoji'] = error_emoji
  data[filename]['error_assessment'] = error_assessment

  puts "  ✓ Assessment complete"
end

def process_file_all_phases(test_file)
  filename = File.basename(test_file)
  puts "Processing #{filename}..."

  data = load_data

  run_parsers_for_file(test_file, filename, data)
  run_autocorrects_for_file(test_file, filename, data)
  run_assessment_for_file(filename, data)

  save_data(data)
  render_markdown_from_json
  puts "✓ Completed #{filename}\n"
end

def run_phase_for_all_files(phase)
  data = load_data
  test_files = Dir.glob("#{TEST_DIR}/*.rb").sort

  test_files.each_with_index do |test_file, index|
    filename = File.basename(test_file)
    puts "[#{index + 1}/#{test_files.length}] Processing #{filename}..."

    case phase
    when 'parsers'
      run_parsers_for_file(test_file, filename, data)
    when 'autocorrects'
      run_autocorrects_for_file(test_file, filename, data)
    when 'assessments'
      run_assessment_for_file(filename, data)
    end

    save_data(data)
    render_markdown_from_json
  end

  puts "\n✓ Phase '#{phase}' complete"
end

def format_error_header(error_count, autocorrects)
  if autocorrects > 0
    "(#{error_count}) | Autocorrects: #{autocorrects}"
  else
    "(#{error_count})"
  end
end

def generate_markdown_section(file_data)
  original_header = format_error_header(file_data['original_error_count'], file_data['original_autocorrects'])
  prism_header = format_error_header(file_data['prism_error_count'], file_data['prism_autocorrects'])

  # Build autocorrect section if present
  autocorrect_section = ""
  if file_data['autocorrect_code_diff']
    autocorrect_section = <<~AUTOCORRECT

      <details>
      <summary>Prism autocorrect diff</summary>

      ```diff
      #{file_data['autocorrect_code_diff']}
      ```

      </details>
    AUTOCORRECT
  end

  <<~MARKDOWN
    ## #{file_data['filename']}

    <details>
    <summary>Input</summary>

    ```ruby
    #{file_data['input_code']}
    ```

    </details>

    <details>
    <summary>Original errors #{original_header}</summary>

    ```
    #{file_data['original_errors']}
    ```

    </details>

    <details>
    <summary>Prism errors #{prism_header}</summary>

    ```
    #{file_data['prism_errors']}
    ```

    </details>

    <details>
    <summary>LLM quality assessment #{file_data['error_emoji']}</summary>

    #{file_data['error_assessment']}

    </details>
    #{autocorrect_section}
    <details>
    <summary>Original desugar tree</summary>

    ```
    #{file_data['original_tree']}
    ```

    </details>

    <details>
    <summary>Prism desugar tree</summary>

    ```
    #{file_data['prism_tree']}
    ```

    </details>

    <details>
    <summary>Desugar tree diff #{file_data['tree_emoji']}</summary>

    ```diff
    #{file_data['tree_diff']}
    ```

    </details>

  MARKDOWN
end

def render_markdown_from_json
  data = load_data

  if data.empty?
    puts "Error: No data in JSON file"
    exit 1
  end

  File.open(OUTPUT_FILE, 'w') do |f|
    f.puts "# Error recovery comparison: Prism vs original\n\n"

    # Sort by filename
    data.keys.sort.each do |filename|
      f.puts generate_markdown_section(data[filename])
    end
  end
end

def main
  # Check for required environment variables (only for phases that need them)
  needs_api = !ARGV.include?('--render') && !ARGV.include?('--phase') ||
              (ARGV.include?('--phase') && ARGV[ARGV.index('--phase') + 1] == 'assessments')

  if needs_api
    unless ENV['OPENAI_API_BASE']
      puts "Error: OPENAI_API_BASE environment variable not set"
      puts "Set it to your Claude proxy URL"
      exit 1
    end

    unless ENV['OPENAI_API_KEY']
      puts "Error: OPENAI_API_KEY environment variable not set"
      exit 1
    end
  end

  # Check sorbet binary exists
  unless File.exist?(SORBET_BINARY)
    puts "Error: Sorbet binary not found at #{SORBET_BINARY}"
    puts "Please build sorbet first"
    exit 1
  end

  if ARGV.empty?
    puts "Usage:"
    puts "  ruby generate_error_recovery_comparison.rb --all"
    puts "  ruby generate_error_recovery_comparison.rb --file FILENAME"
    puts "  ruby generate_error_recovery_comparison.rb --phase PHASE"
    puts "    Phases: parsers, autocorrects, assessments"
    puts "  ruby generate_error_recovery_comparison.rb --render"
    exit 1
  end

  case ARGV[0]
  when '--all'
    test_files = Dir.glob("#{TEST_DIR}/*.rb").sort
    puts "Found #{test_files.length} test files"
    puts "Using Claude model: #{CLAUDE_MODEL}"
    puts "Output: JSON -> #{JSON_FILE}, Markdown -> #{OUTPUT_FILE}\n\n"

    test_files.each_with_index do |test_file, index|
      filename = File.basename(test_file)
      puts "[#{index + 1}/#{test_files.length}] #{filename}"
      process_file_all_phases(test_file)
    end

    puts "\n✓ Complete"

  when '--file'
    filename = ARGV[1]
    unless filename
      puts "Error: --file requires a filename argument"
      exit 1
    end

    test_file = "#{TEST_DIR}/#{filename}"
    unless File.exist?(test_file)
      puts "Error: File not found: #{test_file}"
      exit 1
    end

    process_file_all_phases(test_file)
    puts "\n✓ Complete"

  when '--phase'
    phase = ARGV[1]
    unless ['parsers', 'autocorrects', 'assessments'].include?(phase)
      puts "Error: Invalid phase '#{phase}'"
      puts "Valid phases: parsers, autocorrects, assessments"
      exit 1
    end

    run_phase_for_all_files(phase)
    puts "\n✓ Complete"

  when '--render'
    render_markdown_from_json

  else
    puts "Unknown option: #{ARGV[0]}"
    exit 1
  end
end

if __FILE__ == $0
  main
end
