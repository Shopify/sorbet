# typed: true
# disable-parser-comparison: true

LOG_HEADER = 'Hello!'
str = <<~DESC
        This\n\is a regular line
        So\nis this
        \n#{LOG_HEADER}.
        the previous line however contains string interpolation
      DESC