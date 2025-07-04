#ifndef DEF_OP_TRIPLE
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3)
#endif //DEF_OP_TRIPLE
#ifndef DEF_OP_DOUBLE
#define DEF_OP_DOUBLE(enumval, ch1, ch2)
#endif //DEF_OP_DOUBLE
#ifndef DEF_OP_SINGLE
#define DEF_OP_SINGLE(enumval, ch1)
#endif //DEF_OP_SINGLE

DEF_OP_TRIPLE(ass_shl, '<', '<', '=')
DEF_OP_TRIPLE(ass_shr, '>', '>', '=')
DEF_OP_TRIPLE(varargs, '.', '.', '.')

DEF_OP_DOUBLE(op_inc, '+', '+')
DEF_OP_DOUBLE(op_dec, '-', '-')
DEF_OP_DOUBLE(op_shl, '<', '<')
DEF_OP_DOUBLE(op_shr, '>', '>')

DEF_OP_DOUBLE(lop_le_eq, '<', '=')
DEF_OP_DOUBLE(lop_gr_eq, '>', '=')
DEF_OP_DOUBLE(lop_eq, '=', '=')
DEF_OP_DOUBLE(lop_not_eq, '!', '=')
DEF_OP_DOUBLE(lop_and, '&', '&')
DEF_OP_DOUBLE(lop_or, '|', '|')

DEF_OP_DOUBLE(ass_plus, '+', '=')
DEF_OP_DOUBLE(ass_min, '-', '=')
DEF_OP_DOUBLE(ass_mul, '*', '=')
DEF_OP_DOUBLE(ass_div, '/', '=')
DEF_OP_DOUBLE(ass_mod, '%', '=')
DEF_OP_DOUBLE(ass_and, '&', '=')
DEF_OP_DOUBLE(ass_xor, '^', '=')
DEF_OP_DOUBLE(ass_or, '|', '=')

DEF_OP_SINGLE(paropen, '(')
DEF_OP_SINGLE(parclose, ')')
DEF_OP_SINGLE(bropen, '{')
DEF_OP_SINGLE(brclose, '}')
DEF_OP_SINGLE(subopen, '[')
DEF_OP_SINGLE(subclose, ']')
DEF_OP_SINGLE(member, '.')
DEF_OP_SINGLE(comma, ',')
DEF_OP_SINGLE(tenary, '?')
DEF_OP_SINGLE(colon, ':')
DEF_OP_SINGLE(assign, '=')
DEF_OP_SINGLE(semic, ';')
DEF_OP_SINGLE(dot, '.')

DEF_OP_SINGLE(op_mul, '*')
DEF_OP_SINGLE(op_div, '/')
DEF_OP_SINGLE(op_mod, '%')
DEF_OP_SINGLE(op_plus, '+')
DEF_OP_SINGLE(op_min, '-')
DEF_OP_SINGLE(op_neg, '~')
DEF_OP_SINGLE(op_xor, '^')
DEF_OP_SINGLE(op_and, '&')
DEF_OP_SINGLE(op_or, '|')

DEF_OP_SINGLE(lop_le, '<')
DEF_OP_SINGLE(lop_gr, '>')
DEF_OP_SINGLE(lop_not, '!')