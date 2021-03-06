/* Generated by gliss-attr ($(date)) copyright (c) 2009 IRIT - UPS */

#include <$(proc)/api.h>
#include <$(proc)/id.h>
#include <$(proc)/macros.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t otawa_$(proc)_reg_t;

typedef otawa_$(proc)_reg_t *(*reg_fun_t)($(proc)_inst_t *inst);


#define END_REG	0
/* supposed max number of registers used by an instruction */
#define MAX_REG 64

/* change size if any instruction uses a lot of registers */
static otawa_$(proc)_reg_t tmp_var[MAX_REG];

/* put the gliss2 otawa_used_regs in tmp_var for further use
 * return the address of reg infos */
otawa_$(proc)_reg_t *USED_REGS(uint32_t arg1, ...)
{
	va_list param_list;
	uint32_t val;
	int i = 1;

	tmp_var[0] = arg1;

	if (arg1 == END_REG)
		return tmp_var;

	va_start(param_list, arg1);
	val = va_arg(param_list, uint32_t);
	while (val != END_REG)
	{
		tmp_var[i] = val;
		i++;
		val = va_arg(param_list, uint32_t);
	}
	tmp_var[i] = END_REG;
	va_end(param_list);
	return tmp_var;
}

static otawa_$(proc)_reg_t *otawa_used_regs_UNKNOWN($(proc)_inst_t *inst)
{
	/* this code should also be used as default value if
	an instruction has no otawa_used_regs field */
	tmp_var[0] = END_REG;
	return tmp_var;
}

$(foreach instructions)
static otawa_$(proc)_reg_t *otawa_used_regs_$(IDENT)($(proc)_inst_t *inst) {
$(otawa_used_regs)
};

$(end)

/* function table */
static reg_fun_t otawa_used_regs_table[] =
{
	otawa_used_regs_UNKNOWN$(foreach instructions),
	otawa_used_regs_$(IDENT)$(end)
};



/**
 * Get the OTAWA used regs of the instruction.
 * @return the address of an array of otawa_$(proc)_reg_t.
 */
otawa_$(proc)_reg_t * $(proc)_used_regs(ppc_inst_t *inst) {
	return otawa_used_regs_table[inst->ident](inst);
}

#ifdef __cplusplus
}
#endif
