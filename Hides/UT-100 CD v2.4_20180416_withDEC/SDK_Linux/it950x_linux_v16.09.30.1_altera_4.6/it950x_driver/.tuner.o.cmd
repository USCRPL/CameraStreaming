cmd_/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o := /home/user/Desktop/driver/Eagle/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-gcc -Wp,-MD,/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/.tuner.o.d  -nostdinc -isystem /home/user/Desktop/driver/Eagle/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/../lib/gcc/arm-linux-gnueabihf/4.8.3/include -I./arch/arm/include -Iarch/arm/include/generated/uapi -Iarch/arm/include/generated  -Iinclude -I./arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I./include/uapi -Iinclude/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-dwarf2-cfi-asm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -fno-ipa-sra -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfp -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -fno-delete-null-pointer-checks -O2 --param=allow-store-data-races=0 -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -g -pg -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -DCC_HAVE_ASM_GOTO -DEXPORT_SYMTAB -Wno-unused-value -Wno-unused-variable -Wno-unused-parameter -Wno-switch -Wno-enum-compare -Wno-int-to-pointer-cast -Wno-array-bounds  -DMODULE  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(tuner)"  -D"KBUILD_MODNAME=KBUILD_STR(usb_it950x)" -c -o /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.c

source_/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o := /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.c

deps_/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o := \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/modulatorType.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/kasan.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/gcov/kernel.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/uapi/linux/types.h \
  arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
    $(wildcard include/config/64bit.h) \
  include/uapi/asm-generic/bitsperlong.h \
  include/uapi/linux/posix_types.h \
  arch/arm/include/uapi/asm/posix_types.h \
  include/uapi/asm-generic/posix_types.h \
  include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
  include/linux/delay.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/user/Desktop/driver/Eagle/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/lib/gcc/arm-linux-gnueabihf/4.8.3/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  arch/arm/include/asm/linkage.h \
  include/linux/bitops.h \
  arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  arch/arm/include/asm/irqflags.h \
    $(wildcard include/config/cpu/v7m.h) \
  arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  arch/arm/include/asm/hwcap.h \
  arch/arm/include/uapi/asm/hwcap.h \
  include/asm-generic/irqflags.h \
  arch/arm/include/asm/barrier.h \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/arm/heavy/mb.h) \
    $(wildcard include/config/arch/has/barriers.h) \
    $(wildcard include/config/arm/dma/mem/bufferable.h) \
  include/asm-generic/barrier.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/le.h \
  arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  arch/arm/include/asm/swab.h \
  arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/asm-generic/bitops/ext2-atomic-setbit.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/printk.h \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
    $(wildcard include/config/debug/rodata.h) \
    $(wildcard include/config/lto.h) \
  include/linux/kern_levels.h \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/uapi/linux/kernel.h \
  include/uapi/linux/sysinfo.h \
  arch/arm/include/asm/cache.h \
    $(wildcard include/config/arm/l1/cache/shift.h) \
    $(wildcard include/config/aeabi.h) \
  include/linux/dynamic_debug.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/uapi/linux/string.h \
  arch/arm/include/asm/string.h \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  include/uapi/asm-generic/errno.h \
  include/uapi/asm-generic/errno-base.h \
  arch/arm/include/asm/delay.h \
  arch/arm/include/asm/memory.h \
    $(wildcard include/config/need/mach/memory/h.h) \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/have/tcm.h) \
    $(wildcard include/config/arm/patch/phys/virt.h) \
    $(wildcard include/config/phys/offset.h) \
    $(wildcard include/config/xip/kernel.h) \
    $(wildcard include/config/xip/phys/addr.h) \
  include/uapi/linux/const.h \
  include/linux/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  include/linux/pfn.h \
  arch/arm/include/generated/asm/param.h \
  include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  include/uapi/asm-generic/param.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/error.h \
    $(wildcard include/config/header.h) \
    $(wildcard include/config/size.h) \
    $(wildcard include/config/sn.h) \
    $(wildcard include/config/subtype.h) \
    $(wildcard include/config/value.h) \
    $(wildcard include/config/chksum.h) \
    $(wildcard include/config/value.h) \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/user.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/register.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/standard.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/variable.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/version.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Omega.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_Script.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_1_Script.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_2_Script.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_Script_V2.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_1_Script_V2.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_2_Script_V2.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_3_Script_V2.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_4_Script_V2I.h \
  /home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/Firmware_Afa_Omega_LNA_Config_5_Script_V2I.h \

/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o: $(deps_/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o)

$(deps_/home/user/Desktop/driver/Eagle/it950x_linux_v16.09.30.1_altera_4.6/it950x_driver/tuner.o):
