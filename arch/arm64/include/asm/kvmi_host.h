/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_ARM64_KVMI_HOST_H
#define _ASM_ARM64_KVMI_HOST_H

// TODO: this still contains x86 stuff, but will do for now.

//#include <asm/kvm_page_track.h>

// SHOULD NEVER BE USED
#define KVMI_MAX_ACCESS_TREES 1

struct msr_data;

struct kvmi_monitor_interception {
	bool kvmi_intercepted;
	bool kvm_intercepted;
	bool (*monitor_fct)(struct kvm_vcpu *vcpu, bool enable);
};

struct kvmi_interception {
	bool restore_interception;
	struct kvmi_monitor_interception breakpoint;
	struct kvmi_monitor_interception cr3w;
	struct kvmi_monitor_interception descriptor;
};

struct kvm_vcpu_arch_introspection {
	struct kvm_regs delayed_regs;
	bool have_delayed_regs;
};

struct kvm_arch_introspection {
};

struct kvmi_arch_mem_access {
};

#ifdef CONFIG_KVM_INTROSPECTION

bool kvmi_monitor_bp_intercept(struct kvm_vcpu *vcpu, u32 dbg);
bool kvmi_cr_event(struct kvm_vcpu *vcpu, unsigned int cr,
		   unsigned long old_value, unsigned long *new_value);
bool kvmi_cr3_intercepted(struct kvm_vcpu *vcpu);
bool kvmi_monitor_cr3w_intercept(struct kvm_vcpu *vcpu, bool enable);
void kvmi_xsetbv_event(struct kvm_vcpu *vcpu);
bool kvmi_monitor_desc_intercept(struct kvm_vcpu *vcpu, bool enable);
bool kvmi_descriptor_event(struct kvm_vcpu *vcpu, u8 descriptor, u8 write);
bool kvmi_msr_event(struct kvm_vcpu *vcpu, struct msr_data *msr);
bool kvmi_monitor_msrw_intercept(struct kvm_vcpu *vcpu, u32 msr, bool enable);
bool kvmi_msrw_intercept_originator(struct kvm_vcpu *vcpu);
bool kvmi_update_ad_flags(struct kvm_vcpu *vcpu);
bool kvmi_cpuid_event(struct kvm_vcpu *vcpu, u8 insn_len,
		      unsigned int function, unsigned int index);

#else /* CONFIG_KVM_INTROSPECTION */

static inline bool kvmi_monitor_bp_intercept(struct kvm_vcpu *vcpu, u32 dbg)
	{ return false; }
static inline bool kvmi_cr_event(struct kvm_vcpu *vcpu, unsigned int cr,
				 unsigned long old_value,
				 unsigned long *new_value) { return true; }
static inline bool kvmi_cr3_intercepted(struct kvm_vcpu *vcpu) { return false; }
static inline bool kvmi_monitor_cr3w_intercept(struct kvm_vcpu *vcpu,
						bool enable) { return false; }
static inline void kvmi_xsetbv_event(struct kvm_vcpu *vcpu) { }
static inline bool kvmi_monitor_desc_intercept(struct kvm_vcpu *vcpu,
					       bool enable) { return false; }
static inline bool kvmi_descriptor_event(struct kvm_vcpu *vcpu, u8 descriptor,
					 u8 write) { return true; }
static inline bool kvmi_msr_event(struct kvm_vcpu *vcpu, struct msr_data *msr)
				{ return true; }
static inline bool kvmi_monitor_msrw_intercept(struct kvm_vcpu *vcpu, u32 msr,
					       bool enable) { return false; }
static inline bool kvmi_msrw_intercept_originator(struct kvm_vcpu *vcpu)
				{ return false; }
static inline bool kvmi_update_ad_flags(struct kvm_vcpu *vcpu) { return false; }
static inline bool kvmi_cpuid_event(struct kvm_vcpu *vcpu, u8 insn_len,
				    unsigned int function, unsigned int index)
				    { return true; }

#endif /* CONFIG_KVM_INTROSPECTION */

#endif /* _ASM_ARM64_KVMI_HOST_H */
