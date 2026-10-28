# Financial rules

The modular `Money` type stores paise in `int64_t`, rejects malformed/overflowing parsed strings, and detects arithmetic overflow. It is the intended monetary foundation.

The currently linked legacy financial engine still has historical `double` fields and simplified loan/FD/RD/card calculations. Its existing behavior was preserved to avoid silently changing simulation results. It is not financial or legal advice.

