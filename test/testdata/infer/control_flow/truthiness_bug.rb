# typed: true

class Test
  def set_approval_status(_, from_self)
    # approved = _.foo?
    #

    # t1 = if true then true else true end

    if true then end

    # This used to report a dead-code error on the 'pending' below due
    # to a bug in CFG simplification
    # puts(
    #   approved ? 'success' : 'pending'
    # )
  end
end
