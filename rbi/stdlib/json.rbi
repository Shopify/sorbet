# typed: __STDLIB_INTERNAL

# # JavaScript Object Notation (JSON)
#
# JSON is a lightweight data-interchange format. It is easy for us humans to
# read and write. Plus, equally simple for machines to generate or parse.
# JSON is completely language agnostic, making it the ideal interchange format.
#
# Built on two universally available structures:
#
# ```
# 1. A collection of name/value pairs. Often referred to as an _object_, hash table, record, struct, keyed list, or associative array.
# 2. An ordered list of values. More commonly called an _array_, vector, sequence or list.
# ```
#
# To read more about JSON visit: http://json.org
#
# ## Parsing JSON
#
# To parse a JSON string received by another application or generated within
# your existing application:
#
# ```ruby
# require 'json'
#
# my_hash = JSON.parse('{"hello": "goodbye"}')
# puts my_hash["hello"] => "goodbye"
# ```
#
# Notice the extra quotes `''` around the hash notation. Ruby expects the
# argument to be a string and can't convert objects like a hash or array.
#
# Ruby converts your string into a hash
#
# ## Generating JSON
#
# Creating a JSON string for communication or serialization is just as simple.
#
# ```ruby
# require 'json'
#
# my_hash = {:hello => "goodbye"}
# puts JSON.generate(my_hash) => "{\"hello\":\"goodbye\"}"
# ```
#
# Or an alternative way:
#
# ```
# require 'json'
# puts {:hello => "goodbye"}.to_json => "{\"hello\":\"goodbye\"}"
# ```
#
# `JSON.generate` only allows objects or arrays to be converted to JSON syntax.
# `to_json`, however, accepts many Ruby classes even though it acts only as a
# method for serialization:
#
# ```
# require 'json'
#
# 1.to_json => "1"
# ```
module JSON
  Infinity = ::T.let(nil, ::T.untyped)
  JSON_LOADED = ::T.let(nil, ::T.untyped)
  MinusInfinity = ::T.let(nil, ::T.untyped)
  NaN = ::T.let(nil, ::T.untyped)

  # JSON version
  VERSION = ::T.let(nil, ::T.untyped)

  # If *object* is string-like, parse the string and return the parsed result as
  # a Ruby data structure. Otherwise generate a JSON text from the Ruby data
  # structure object and return it.
  #
  # The *opts* argument is passed through to generate/parse respectively. See
  # generate and parse for their documentation.
  sig do
    params(
      object: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.[](object, opts=T.unsafe(nil)); end

  # This is create identifier, which is used to decide if the *json\_create*
  # hook of a class should be called. It defaults to 'json\_class'.
  sig {returns(::String)}
  def self.create_id(); end

  # This is create identifier, which is used to decide if the *json\_create*
  # hook of a class should be called. It defaults to 'json\_class'.
  sig do
    params(
      create_id: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.create_id=(create_id); end

  # Dumps +obj+ as a JSON string, i.e. calls generate on the object and returns
  # the result.
  #
  # - Argument +io+, if given, should respond to method +write+;
  #   the JSON String is written to +io+, and +io+ is returned.
  #   If +io+ is not given, the JSON String is returned.
  # - Argument +limit+, if given, is passed to JSON.generate as option +max_nesting+.
  sig do
    params(
      obj: ::T.untyped,
      anIO: ::T.untyped,
      limit: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.dump(obj, anIO=T.unsafe(nil), limit=T.unsafe(nil)); end

  # The global default options for the JSON.dump method.
  # Deprecated: will be removed in json 3.0.
  sig {returns(::T.untyped)}
  def self.dump_default_options(); end

  # The global default options for the JSON.dump method.
  # Deprecated: will be removed in json 3.0.
  sig do
    params(
      dump_default_options: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.dump_default_options=(dump_default_options); end

  # Deprecated: just use JSON.generate.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(String)
  end
  def self.fast_generate(obj, opts=T.unsafe(nil)); end

  # Generate a JSON document from the Ruby data structure +obj+ and return it.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(String)
  end
  def self.generate(obj, opts=T.unsafe(nil)); end

  # Returns the JSON generator module that is used by JSON. This is either
  # JSON::Ext::Generator or JSON::Pure::Generator.
  sig {returns(::T.untyped)}
  def self.generator(); end

  # Set the module generator to be used by JSON.
  sig do
    params(
      generator: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.generator=(generator); end

  # Returns the Ruby objects created by parsing the given +source+.
  #
  # - Argument +source+ must be, or be convertible to, a String.
  # - Argument +proc+, if given, must be a Proc that accepts one argument.
  # - Argument +options+, if given, contains a Hash of options for the parsing.
  sig do
    params(
      source: ::T.untyped,
      proc: ::T.untyped,
      options: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load(source, proc=T.unsafe(nil), options=T.unsafe(nil)); end

  # Calls parse(File.read(path), opts).
  sig do
    params(
      path: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load_file(path, opts=T.unsafe(nil)); end

  # Calls JSON.parse!(File.read(path, opts)).
  sig do
    params(
      path: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load_file!(path, opts=T.unsafe(nil)); end

  # The global default options for the JSON.load method.
  # Deprecated: will be removed in json 3.0.
  sig {returns(::T.untyped)}
  def self.load_default_options(); end

  # The global default options for the JSON.load method.
  # Deprecated: will be removed in json 3.0.
  sig do
    params(
      load_default_options: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load_default_options=(load_default_options); end

  # Parse the JSON document +source+ into a Ruby data structure and return it.
  sig do
    params(
      json: String,
      args: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.parse(json, args=T.unsafe(nil)); end

  # Parse the JSON document +source+ into a Ruby data structure and return it.
  # The bang version of parse is the same as parse with defaults of
  # +max_nesting: false+ and +allow_nan: true+.
  sig do
    params(
      source: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.parse!(source, opts=T.unsafe(nil)); end

  # Returns the JSON parser class that is used by JSON. This is either
  # JSON::Ext::Parser or JSON::Pure::Parser.
  sig {returns(::T.untyped)}
  def self.parser(); end

  # Set the JSON parser class parser to be used by JSON.
  sig do
    params(
      parser: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.parser=(parser); end

  # Generate a pretty printed JSON document from the Ruby data structure +obj+
  # and return it.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::String)
  end
  def self.pretty_generate(obj, opts=T.unsafe(nil)); end

  sig do
    params(
      result: ::T.untyped,
      proc: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.recurse_proc(result, &proc); end

  # Deprecated: just use JSON.load.
  sig do
    params(
      source: ::T.untyped,
      proc: ::T.untyped,
      options: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.restore(source, proc=T.unsafe(nil), options=T.unsafe(nil)); end

  # Returns the JSON generator state class that is used by JSON. This is either
  # JSON::Ext::Generator::State or JSON::Pure::Generator::State.
  sig {returns(::T.untyped)}
  def self.state(); end

  # Sets the JSON generator state class that is used by JSON.
  sig do
    params(
      state: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.state=(state); end

  # Deprecated: just use JSON.generate.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unparse(obj, opts=T.unsafe(nil)); end

  # Deprecated: just use JSON.generate.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.fast_unparse(obj, opts=T.unsafe(nil)); end

  # Deprecated: just use JSON.pretty_generate.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.pretty_unparse(obj, opts=T.unsafe(nil)); end

  # Like JSON.dump but with +create_additions: true+ as default.
  sig do
    params(
      obj: ::T.untyped,
      anIO: ::T.untyped,
      limit: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_dump(obj, anIO=T.unsafe(nil), limit=T.unsafe(nil)); end

  # Like JSON.generate but with +allow_nan: true+, +max_nesting: false+.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_generate(obj, opts=T.unsafe(nil)); end

  # Like JSON.load but with +create_additions: true+ as default.
  sig do
    params(
      source: ::T.untyped,
      proc: ::T.untyped,
      options: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_load(source, proc=T.unsafe(nil), options=T.unsafe(nil)); end

  # The global default options for the JSON.unsafe_load method.
  # Deprecated: will be removed in json 3.0.
  sig {returns(::T.untyped)}
  def self.unsafe_load_default_options(); end

  # The global default options for the JSON.unsafe_load method.
  # Deprecated: will be removed in json 3.0.
  sig do
    params(
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_load_default_options=(opts); end

  # Like JSON.parse but with +max_nesting: false+, +allow_nan: true+.
  sig do
    params(
      source: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_parse(source, opts=T.unsafe(nil)); end

  # Like JSON.pretty_generate but with +allow_nan: true+, +max_nesting: false+.
  sig do
    params(
      obj: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unsafe_pretty_generate(obj, opts=T.unsafe(nil)); end
end

class JSON::CircularDatastructure < JSON::NestingError
end

# JSON::Coder holds a parser and generator configuration.
#
#   module MyApp
#     JSONC_CODER = JSON::Coder.new(
#       allow_trailing_comma: true
#     )
#   end
#
#   MyApp::JSONC_CODER.load(document)
#
class JSON::Coder
  # Instantiates a new Coder with options and an optional block for custom
  # serialization of non-native types.
  #
  #   coder = JSON::Coder.new do |object|
  #     case object
  #     when Time
  #       object.iso8601(3)
  #     else
  #       object
  #     end
  #   end
  sig do
    params(
      options: T.nilable(T::Hash[Symbol, T.untyped]),
      as_json: T.nilable(T.proc.params(arg0: T.untyped).returns(T.untyped)),
    )
    .void
  end
  def initialize(options=T.unsafe(nil), &as_json); end

  # Serialize the given object into a JSON document.
  # If +io+ is given, writes to it and returns +io+.
  sig do
    params(
      object: ::T.untyped,
      io: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def dump(object, io=T.unsafe(nil)); end

  # Alias for dump.
  sig do
    params(
      object: ::T.untyped,
      io: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def generate(object, io=T.unsafe(nil)); end

  # Parse the given JSON document and return an equivalent Ruby object.
  sig do
    params(
      source: String,
    )
    .returns(::T.untyped)
  end
  def load(source); end

  # Alias for load.
  sig do
    params(
      source: String,
    )
    .returns(::T.untyped)
  end
  def parse(source); end

  # Parse a JSON file at the given path and return an equivalent Ruby object.
  sig do
    params(
      path: String,
    )
    .returns(::T.untyped)
  end
  def load_file(path); end
end

# Fragment of JSON document that is to be included as is:
#
#   fragment = JSON::Fragment.new("[1, 2, 3]")
#   JSON.generate({ count: 3, items: fragment })
#
# This allows to easily assemble multiple JSON fragments that have
# been persisted somewhere without having to parse them nor resorting
# to string interpolation.
#
# Note: no validation is performed on the provided string. It is the
# responsibility of the caller to ensure the string contains valid JSON.
class JSON::Fragment < Struct
  sig do
    params(
      json: String,
    )
    .void
  end
  def initialize(json); end

  # Returns the raw JSON string.
  sig {returns(String)}
  def json(); end

  # Returns the raw JSON string (for use during generation).
  sig do
    params(
      state: ::T.untyped,
      args: ::T.untyped,
    )
    .returns(String)
  end
  def to_json(state=T.unsafe(nil), *args); end
end

# This exception is raised if a generator or unparser error occurs.
class JSON::GeneratorError < JSON::JSONError
  sig do
    params(
      message: ::T.untyped,
      invalid_object: ::T.untyped,
    )
    .void
  end
  def initialize(message=T.unsafe(nil), invalid_object=T.unsafe(nil)); end

  # Returns the object that caused the error.
  sig {returns(::T.untyped)}
  def invalid_object(); end
end

class JSON::GenericObject < OpenStruct
  sig do
    params(
      name: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def [](name); end

  sig do
    params(
      name: ::T.untyped,
      value: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def []=(name, value); end

  sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def as_json(*_); end

  sig {returns(::T.untyped)}
  def to_hash(); end

  sig do
    params(
      a: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def to_json(*a); end

  sig do
    params(
      other: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def |(other); end

  sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.[](*_); end

  sig do
    params(
      obj: ::T.untyped,
      args: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.dump(obj, *args); end

  sig do
    params(
      object: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.from_hash(object); end

  sig do
    params(
      json_creatable: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.json_creatable=(json_creatable); end

  sig {returns(::T.untyped)}
  def self.json_creatable?(); end

  sig do
    params(
      data: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.json_create(data); end

  sig do
    params(
      source: ::T.untyped,
      proc: ::T.untyped,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load(source, proc=T.unsafe(nil), opts=T.unsafe(nil)); end
end

# This module holds all the modules/classes that implement JSON's functionality
# as C extensions.
module JSON::Ext
end

# This is the JSON generator implemented as a C extension. It can be configured
# to be used by setting
#
# ```ruby
# JSON.generator = JSON::Ext::Generator
# ```
#
# with the method generator= in JSON.
module JSON::Ext::Generator
end

module JSON::Ext::Generator::GeneratorMethods
end

module JSON::Ext::Generator::GeneratorMethods::Array
  # Returns a JSON string containing a JSON array, that is generated from
  # this Array instance.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::FalseClass
  # Returns a JSON string for false: 'false'.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::Float
  # Returns a JSON string representation for this Float number.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::Hash
  # Returns a JSON string containing a JSON object, that is generated from
  # this Hash instance.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::Integer
  # Returns a JSON string representation for this Integer number.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::NilClass
  # Returns a JSON string for nil: 'null'.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::Object
  # Converts this object to a string (calling to_s), converts it to a JSON
  # string, and returns the result. This is a fallback, if no special method
  # to_json was defined for some object.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

module JSON::Ext::Generator::GeneratorMethods::String
  # This string should be encoded with UTF-8 A call to this method returns a
  # JSON string encoded with UTF16 big endian characters as u????.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end

  # This method creates a JSON text from the result of a call to
  # to_json_raw_object of this String.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json_raw(*_); end

  # This method creates a raw object hash, that can be nested into other data
  # structures and will be generated as a raw string. This method should be
  # used, if you want to convert raw strings to JSON instead of UTF-8 strings,
  # e. g. binary data.
  sig do
    returns(::T::Hash[T.untyped, T.untyped])
  end
  def to_json_raw_object; end

  # Extends *modul* with the String::Extend module.
  sig do
    params(
      _: ::T.untyped
    )
    .void
  end
  def self.included(_); end
end

module JSON::Ext::Generator::GeneratorMethods::String::Extend
  # Raw Strings are JSON Objects (the raw bytes are stored in an array for the
  # key "raw"). The Ruby String can be created by this module method.
  sig do
    params(
      _: ::T::Hash[T.untyped, T.untyped]
    )
    .returns(::String)
  end
  def json_create(_); end
end

module JSON::Ext::Generator::GeneratorMethods::TrueClass
  # Returns a JSON string for true: 'true'.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::String)
  end
  def to_json(*_); end
end

class JSON::Ext::Generator::State
  sig do
    params(
      _: ::T.untyped
    )
    .void
  end
  def initialize(*_); end

  # Returns the value returned by method `name`.
  # Deprecated: will be removed in json 3.0.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def [](_); end

  # Sets the attribute name to value.
  # Deprecated: will be removed in json 3.0.
  sig do
    params(
      name: ::T.untyped,
      value: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def []=(name, value); end

  # Returns true, if NaN, Infinity, and -Infinity should be generated, otherwise
  # returns false.
  sig do
    returns(::T.untyped)
  end
  def allow_nan?; end

  # Sets allow_nan.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def allow_nan=(_); end

  # This string is put at the end of a line that holds a JSON array.
  sig do
    returns(::T.untyped)
  end
  def array_nl; end

  # This string is put at the end of a line that holds a JSON array.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def array_nl=(_); end

  # Returns the as_json proc, if set.
  sig do
    returns(::T.untyped)
  end
  def as_json; end

  # Sets the as_json proc.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def as_json=(_); end

  # Returns true, if only ASCII characters should be generated. Otherwise
  # returns false.
  sig do
    returns(::T.untyped)
  end
  def ascii_only?; end

  # Sets ascii_only mode.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def ascii_only=(_); end

  # This integer returns the current initial length of the buffer.
  sig do
    returns(::T.untyped)
  end
  def buffer_initial_length; end

  # This sets the initial length of the buffer to `length`, if `length` > 0,
  # otherwise its value isn't changed.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def buffer_initial_length=(_); end

  # Returns true, if circular data structures should be checked, otherwise
  # returns false.
  sig do
    returns(::T.untyped)
  end
  def check_circular?; end

  # Configure this State instance with the Hash *opts*, and return itself.
  #
  # Also aliased as: merge
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def configure(_); end

  # This integer returns the current depth of data structure nesting.
  sig do
    returns(::T.untyped)
  end
  def depth; end

  # This sets the current depth of data structure nesting.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def depth=(_); end

  # Generates a valid JSON document from object `obj` and returns the result.
  # If no valid JSON document can be created this method raises a
  # GeneratorError exception.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def generate(_); end

  # Returns the string that is used to indent levels in the JSON text.
  sig do
    returns(::T.untyped)
  end
  def indent; end

  # Sets the string that is used to indent levels in the JSON text.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def indent=(_); end

  # This integer returns the maximum level of data structure nesting in the
  # generated JSON, max_nesting = 0 if no maximum is checked.
  sig do
    returns(::T.untyped)
  end
  def max_nesting; end

  # This sets the maximum level of data structure nesting in the generated
  # JSON to the integer depth, max_nesting = 0 if no maximum should be checked.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def max_nesting=(_); end

  # Alias for: configure
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def merge(_); end

  # This string is put at the end of a line that holds a JSON object
  # (or Hash).
  sig do
    returns(::T.untyped)
  end
  def object_nl; end

  # This string is put at the end of a line that holds a JSON object
  # (or Hash).
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def object_nl=(_); end

  # Returns true if script-safe mode is enabled, false otherwise.
  sig do
    returns(::T.untyped)
  end
  def script_safe; end

  # Returns true if script-safe mode is enabled, false otherwise.
  sig do
    returns(::T.untyped)
  end
  def script_safe?; end

  # Sets script_safe mode.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def script_safe=(_); end

  # Returns the string that is used to insert a space between the tokens in a
  # JSON string.
  sig do
    returns(::T.untyped)
  end
  def space; end

  # Sets *space* to the string that is used to insert a space between the tokens
  # in a JSON string.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def space=(_); end

  # Returns the string that is used to insert a space before the ':' in
  # JSON objects.
  sig do
    returns(::T.untyped)
  end
  def space_before; end

  # Sets the string that is used to insert a space before the ':' in
  # JSON objects.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def space_before=(_); end

  # Returns true if strict mode is enabled, false otherwise.
  sig do
    returns(::T.untyped)
  end
  def strict; end

  # Returns true if strict mode is enabled, false otherwise.
  sig do
    returns(::T.untyped)
  end
  def strict?; end

  # Sets strict mode.
  sig do
    params(
      _: ::T.untyped
    )
    .returns(::T.untyped)
  end
  def strict=(_); end

  # Returns the configuration instance variables as a hash, that can be passed
  # to the configure method.
  #
  # Also aliased as: to_hash
  sig do
    returns(::T.untyped)
  end
  def to_h; end

  # Alias for: to_h
  sig do
    returns(::T.untyped)
  end
  def to_hash; end

  # Creates a State object from +opts+, which ought to be Hash to create a
  # new State instance configured by +opts+, something else to create an
  # unconfigured instance. If +opts+ is a State object, it is just returned.
  sig do
    params(
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.from_state(opts); end
end

# This is the JSON parser implemented as a C extension. It can be configured
# to be used by setting
#
# ```ruby
# JSON.parser = JSON::Ext::Parser
# ```
#
# with the method parser= in JSON.
class JSON::Ext::Parser
  sig do
    params(
      source: String,
      opts: ::T.untyped,
    )
    .void
  end
  def initialize(source, opts=T.unsafe(nil)); end

  # Parses the current JSON text *source* and returns the complete data
  # structure as a result.
  sig do
    returns(::T.untyped)
  end
  def parse; end

  # Returns a copy of the current *source* string, that was used to construct
  # this Parser.
  sig do
    returns(::String)
  end
  def source; end

  # Parse the source directly.
  sig do
    params(
      source: String,
      opts: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.parse(source, opts=T.unsafe(nil)); end
end

class JSON::Ext::ParserConfig
  sig do
    params(
      opts: ::T.untyped,
    )
    .void
  end
  def initialize(opts); end

  sig do
    params(
      source: String,
    )
    .returns(::T.untyped)
  end
  def parse(source); end
end

# The base exception for JSON errors.
class JSON::JSONError < StandardError
  sig do
    params(
      exception: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.wrap(exception); end
end

# This exception is raised if the nesting of parsed data structures is too deep.
class JSON::NestingError < JSON::ParserError
end

# This exception is raised if a parser error occurs.
class JSON::ParserError < JSON::JSONError
  # Returns the line number where the error occurred, if available.
  sig {returns(T.nilable(Integer))}
  def line; end

  # Returns the column number where the error occurred, if available.
  sig {returns(T.nilable(Integer))}
  def column; end
end

# JSON::GeneratorMethods is included into Object to provide the fallback
# to_json method.
module JSON::GeneratorMethods
  sig do
    params(
      state: ::T.untyped,
      args: ::T.untyped,
    )
    .returns(::String)
  end
  def to_json(state=T.unsafe(nil), *args); end
end

class Array
  include JSON::Ext::Generator::GeneratorMethods::Array
end

class FalseClass
  include JSON::Ext::Generator::GeneratorMethods::FalseClass
end

class Float
  include JSON::Ext::Generator::GeneratorMethods::Float
end

class Hash
  include JSON::Ext::Generator::GeneratorMethods::Hash
end

class Integer
  include JSON::Ext::Generator::GeneratorMethods::Integer
end

class NilClass
  include JSON::Ext::Generator::GeneratorMethods::NilClass
end

class Object
  include JSON::GeneratorMethods
end

class String
  include JSON::Ext::Generator::GeneratorMethods::String
end

class TrueClass
  include JSON::Ext::Generator::GeneratorMethods::TrueClass
end

# source://json//lib/json/add/exception.rb#6
class Exception
  # Methods <tt>Exception#as_json</tt> and +Exception.json_create+ may be used
  # to serialize and deserialize a \Exception object;
  # see Marshal[https://docs.ruby-lang.org/en/master/Marshal.html].
  #
  # \Method <tt>Exception#as_json</tt> serializes +self+,
  # returning a 2-element hash representing +self+:
  #
  #   require 'json/add/exception'
  #   x = Exception.new('Foo').as_json # => {"json_class"=>"Exception", "m"=>"Foo", "b"=>nil}
  #
  # \Method +JSON.create+ deserializes such a hash, returning a \Exception object:
  #
  #   Exception.json_create(x) # => #<Exception: Foo>
  #
  # source://json//lib/json/add/exception.rb#29
  sig do
    params(
      _arg0: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def as_json(*_arg0); end

  # Returns a JSON string representing +self+:
  #
  #   require 'json/add/exception'
  #   puts Exception.new('Foo').to_json
  #
  # Output:
  #
  #   {"json_class":"Exception","m":"Foo","b":null}
  #
  # source://json//lib/json/add/exception.rb#46
  sig do
    params(
      args: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def to_json(*args); end

  class << self
    # See #as_json.
    #
    # source://json//lib/json/add/exception.rb#9
    sig do
      params(
        object: ::T.untyped,
      )
      .returns(::T.untyped)
    end
    def json_create(object); end
  end
end
