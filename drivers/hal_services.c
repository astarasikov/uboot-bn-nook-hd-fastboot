#include <omap4_hs.h>
#include <common.h>
#include <asm/arch/rom_public_api_func.h>

u32 SEC_ENTRY_Std_Ppa_Call (u32 appl_id, u32 inNbArg, ...)
{
	u32 result = 0;
	u32 Param[4];
	va_list ap;

	va_start(ap, inNbArg);

	switch (inNbArg)
	{
		case 0:
			result = omap_smc_ppa(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg);
			break;

		case 1:
			Param[0] = va_arg(ap, u32);
			result = omap_smc_ppa(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0]);
			break;

		case 2:
			Param[0] = va_arg(ap, u32);
			Param[1] = va_arg(ap, u32);
			result = omap_smc_ppa(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1]);
			break;

		case 3:
			Param[0] = va_arg(ap, u32);
			Param[1] = va_arg(ap, u32);
			Param[2] = va_arg(ap, u32);
			result = omap_smc_ppa(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1], Param[2]);
			break;
		case 4:
			Param[0] = va_arg(ap, u32);
			Param[1] = va_arg(ap, u32);
			Param[2] = va_arg(ap, u32);
			Param[3] = va_arg(ap, u32);
			result = omap_smc_ppa(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1], Param[2], Param[3]);
			break;
		default:
			printf("[ERROR] [SEC_ENTRY] Number of arguments not supported \n");
			return 1;
	}
	va_end(ap);
	if (result != 0)
		printf("[ERROR] : %d\n",result);
	return result;
}

u32 PL310aux(u32 appl_id, u32 value)
{
	__asm__ __volatile__("stmfd   sp!, {r2-r12, lr}");
	__asm__ __volatile__("mov    r12, r0");
	__asm__ __volatile__("ldr    r0, [r1]");
	__asm__ __volatile__("orr    r0, r0, #0x1E000000");
	__asm__ __volatile__("dsb");
	__asm__ __volatile__("smc #0");
	//__asm__ __volatile__(".word  0xE1600071");
	__asm__ __volatile__("ldmfd   sp!, {r2-r12, pc}");
	return 0;
}

