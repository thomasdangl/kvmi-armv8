// SPDX-License-Identifier: GPL-2.0
/*
 * KVM introspection - ARM64
 *
 * Copyright (C) 2019-2021 Bitdefender S.R.L.
 */

#include "linux/kvm_host.h"
#include "../../../virt/kvm/introspection/kvmi_int.h"
#include "kvmi.h"

void kvmi_arch_init_vcpu_events_mask(unsigned long *supported)
{
	//BUILD_BUG_ON(KVM_MEM_SLOTS_NUM != KVMI_MEM_SLOTS_NUM);

	set_bit(KVMI_VCPU_EVENT_BREAKPOINT, supported);
	set_bit(KVMI_VCPU_EVENT_CR, supported);
	set_bit(KVMI_VCPU_EVENT_HYPERCALL, supported);
	set_bit(KVMI_VCPU_EVENT_DESCRIPTOR, supported);
	set_bit(KVMI_VCPU_EVENT_MSR, supported);
	set_bit(KVMI_VCPU_EVENT_PF, supported);
	set_bit(KVMI_VCPU_EVENT_SINGLESTEP, supported);
	set_bit(KVMI_VCPU_EVENT_TRAP, supported);
	set_bit(KVMI_VCPU_EVENT_XSETBV, supported);
}

void kvmi_arch_setup_vcpu_event(struct kvm_vcpu *vcpu,
				struct kvmi_vcpu_event *ev)
{
	struct kvmi_vcpu_event_arch *event = &ev->arch;

	kvm_arch_vcpu_get_regs(vcpu, &event->regs);
	kvm_arch_vcpu_get_sregs(vcpu, &event->sregs);
}

int kvmi_arch_cmd_vcpu_get_registers(struct kvm_vcpu *vcpu,
				struct kvmi_vcpu_get_registers_reply *rpl)
{
	kvm_arch_vcpu_get_regs(vcpu, &rpl->regs);
	kvm_arch_vcpu_get_sregs(vcpu, &rpl->sregs);

	return 0;
}

void kvmi_arch_cmd_vcpu_set_registers(struct kvm_vcpu *vcpu,
				      const struct kvm_regs *regs)
{
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);
	struct kvm_regs *dest = &vcpui->arch.delayed_regs;

	memcpy(dest, regs, sizeof(*dest));

	vcpui->arch.have_delayed_regs = true;
}

void kvmi_arch_post_reply(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);

	if (!vcpui->arch.have_delayed_regs)
		return;

	kvm_arch_vcpu_set_regs(vcpu, &vcpui->arch.delayed_regs);
	vcpui->arch.have_delayed_regs = false;
}

bool kvmi_arch_is_agent_hypercall(struct kvm_vcpu *vcpu)
{
#if 0
	unsigned long subfunc1, subfunc2;
	bool longmode = is_64_bit_mode(vcpu);

	if (longmode) {
		subfunc1 = kvm_rdi_read(vcpu);
		subfunc2 = kvm_rsi_read(vcpu);
	} else {
		subfunc1 = kvm_rbx_read(vcpu);
		subfunc1 &= 0xFFFFFFFF;
		subfunc2 = kvm_rcx_read(vcpu);
		subfunc2 &= 0xFFFFFFFF;
	}

	return (subfunc1 == KVM_HC_XEN_HVM_OP_GUEST_REQUEST_VM_EVENT
		&& subfunc2 == 0);
#endif
	return false;
}

/*
 * Returns true if one side (kvm or kvmi) tries to enable/disable the breakpoint
 * interception while the other side is still tracking it.
 */
bool kvmi_monitor_bp_intercept(struct kvm_vcpu *vcpu, u32 dbg)
{
	struct kvmi_interception *arch_vcpui = READ_ONCE(vcpu->arch.kvmi);
	u32 bp_mask = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_USE_SW_BP;
	bool enable = false;

	if ((dbg & bp_mask) == bp_mask)
		enable = true;

	return (arch_vcpui && arch_vcpui->breakpoint.monitor_fct(vcpu, enable));
}
EXPORT_SYMBOL(kvmi_monitor_bp_intercept);

#if 0
static bool monitor_bp_fct_kvmi(struct kvm_vcpu *vcpu, bool enable)
{
	if (enable) {
		if (static_call(kvm_x86_bp_intercepted)(vcpu))
			return true;
	} else if (!vcpu->arch.kvmi->breakpoint.kvmi_intercepted)
		return true;

	vcpu->arch.kvmi->breakpoint.kvmi_intercepted = enable;

	return false;
}

static bool monitor_bp_fct_kvm(struct kvm_vcpu *vcpu, bool enable)
{
	if (enable) {
		if (static_call(kvm_x86_bp_intercepted)(vcpu))
			return true;
	} else if (!vcpu->arch.kvmi->breakpoint.kvm_intercepted)
		return true;

	vcpu->arch.kvmi->breakpoint.kvm_intercepted = enable;

	return false;
}

static int kvmi_control_bp_intercept(struct kvm_vcpu *vcpu, bool enable)
{
	struct kvm_guest_debug dbg = {};
	int err = 0;

	vcpu->arch.kvmi->breakpoint.monitor_fct = monitor_bp_fct_kvmi;
	if (enable)
		dbg.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_USE_SW_BP;

	err = kvm_arch_vcpu_set_guest_debug(vcpu, &dbg);
	vcpu->arch.kvmi->breakpoint.monitor_fct = monitor_bp_fct_kvm;

	return err;
}

static void kvmi_arch_disable_bp_intercept(struct kvm_vcpu *vcpu)
{
	kvmi_control_bp_intercept(vcpu, false);

	vcpu->arch.kvmi->breakpoint.kvmi_intercepted = false;
	vcpu->arch.kvmi->breakpoint.kvm_intercepted = false;
}

static bool monitor_cr3w_fct_kvmi(struct kvm_vcpu *vcpu, bool enable)
{
	vcpu->arch.kvmi->cr3w.kvmi_intercepted = enable;

	if (enable)
		vcpu->arch.kvmi->cr3w.kvm_intercepted =
			static_call(kvm_x86_cr3_write_intercepted)(vcpu);
	else if (vcpu->arch.kvmi->cr3w.kvm_intercepted)
		return true;

	return false;
}

static bool monitor_cr3w_fct_kvm(struct kvm_vcpu *vcpu, bool enable)
{
	if (!vcpu->arch.kvmi->cr3w.kvmi_intercepted)
		return false;

	vcpu->arch.kvmi->cr3w.kvm_intercepted = enable;

	if (!enable)
		return true;

	return false;
}

/*
 * Returns true if one side (kvm or kvmi) tries to disable the CR3 write
 * interception while the other side is still tracking it.
 */
bool kvmi_monitor_cr3w_intercept(struct kvm_vcpu *vcpu, bool enable)
{
	struct kvmi_interception *arch_vcpui = READ_ONCE(vcpu->arch.kvmi);

	return (arch_vcpui && arch_vcpui->cr3w.monitor_fct(vcpu, enable));
}
EXPORT_SYMBOL(kvmi_monitor_cr3w_intercept);

static void kvmi_control_cr3w_intercept(struct kvm_vcpu *vcpu, bool enable)
{
	vcpu->arch.kvmi->cr3w.monitor_fct = monitor_cr3w_fct_kvmi;
	static_call(kvm_x86_control_cr3_intercept)(vcpu, CR_TYPE_W, enable);
	vcpu->arch.kvmi->cr3w.monitor_fct = monitor_cr3w_fct_kvm;
}

static void kvmi_arch_disable_cr3w_intercept(struct kvm_vcpu *vcpu)
{
	kvmi_control_cr3w_intercept(vcpu, false);

	vcpu->arch.kvmi->cr3w.kvmi_intercepted = false;
	vcpu->arch.kvmi->cr3w.kvm_intercepted = false;
}

/*
 * Returns true if one side (kvm or kvmi) tries to disable the descriptor
 * interception while the other side is still tracking it.
 */
bool kvmi_monitor_desc_intercept(struct kvm_vcpu *vcpu, bool enable)
{
	struct kvmi_interception *arch_vcpui = READ_ONCE(vcpu->arch.kvmi);

	return (arch_vcpui && arch_vcpui->descriptor.monitor_fct(vcpu, enable));
}
EXPORT_SYMBOL(kvmi_monitor_desc_intercept);

static bool monitor_desc_fct_kvmi(struct kvm_vcpu *vcpu, bool enable)
{
	vcpu->arch.kvmi->descriptor.kvmi_intercepted = enable;

	if (enable)
		vcpu->arch.kvmi->descriptor.kvm_intercepted =
			static_call(kvm_x86_desc_intercepted)(vcpu);
	else if (vcpu->arch.kvmi->descriptor.kvm_intercepted)
		return true;

	return false;
}

static bool monitor_desc_fct_kvm(struct kvm_vcpu *vcpu, bool enable)
{
	if (!vcpu->arch.kvmi->descriptor.kvmi_intercepted)
		return false;

	vcpu->arch.kvmi->descriptor.kvm_intercepted = enable;

	if (!enable)
		return true;

	return false;
}

static int kvmi_control_desc_intercept(struct kvm_vcpu *vcpu, bool enable)
{
	if (!static_call(kvm_x86_desc_ctrl_supported)())
		return -KVM_EOPNOTSUPP;

	vcpu->arch.kvmi->descriptor.monitor_fct = monitor_desc_fct_kvmi;
	static_call(kvm_x86_control_desc_intercept)(vcpu, enable);
	vcpu->arch.kvmi->descriptor.monitor_fct = monitor_desc_fct_kvm;

	return 0;
}

static void kvmi_arch_disable_desc_intercept(struct kvm_vcpu *vcpu)
{
	kvmi_control_desc_intercept(vcpu, false);

	vcpu->arch.kvmi->descriptor.kvmi_intercepted = false;
	vcpu->arch.kvmi->descriptor.kvm_intercepted = false;
}

static unsigned long *msr_mask(struct kvm_vcpu *vcpu, unsigned int *msr,
			       bool kvmi)
{
	switch (*msr) {
	case 0 ... 0x1fff:
		return kvmi ? vcpu->arch.kvmi->msrw.kvmi_mask.low :
			      vcpu->arch.kvmi->msrw.kvm_mask.low;
	case 0xc0000000 ... 0xc0001fff:
		*msr &= 0x1fff;
		return kvmi ? vcpu->arch.kvmi->msrw.kvmi_mask.high :
			      vcpu->arch.kvmi->msrw.kvm_mask.high;
	}

	return NULL;
}

static bool test_msr_mask(struct kvm_vcpu *vcpu, unsigned int msr, bool kvmi)
{
	unsigned long *mask = msr_mask(vcpu, &msr, kvmi);

	if (!mask)
		return false;

	return !!test_bit(msr, mask);
}

/*
 * Returns true if one side (kvm or kvmi) tries to disable the MSR write
 * interception while the other side is still tracking it.
 */
bool kvmi_monitor_msrw_intercept(struct kvm_vcpu *vcpu, u32 msr, bool enable)
{
	struct kvmi_interception *arch_vcpui;

	if (!vcpu)
		return false;

	arch_vcpui = READ_ONCE(vcpu->arch.kvmi);

	return (arch_vcpui && arch_vcpui->msrw.monitor_fct(vcpu, msr, enable));
}
EXPORT_SYMBOL(kvmi_monitor_msrw_intercept);

static bool msr_control(struct kvm_vcpu *vcpu, unsigned int msr, bool enable,
			bool kvmi)
{
	unsigned long *mask = msr_mask(vcpu, &msr, kvmi);

	if (!mask)
		return false;

	if (enable)
		set_bit(msr, mask);
	else
		clear_bit(msr, mask);

	return true;
}

static bool msr_intercepted_by_kvmi(struct kvm_vcpu *vcpu, u32 msr)
{
	return test_msr_mask(vcpu, msr, true);
}

static bool msr_intercepted_by_kvm(struct kvm_vcpu *vcpu, u32 msr)
{
	return test_msr_mask(vcpu, msr, false);
}

static void record_msr_intercept_status_for_kvmi(struct kvm_vcpu *vcpu, u32 msr,
						 bool enable)
{
	msr_control(vcpu, msr, enable, true);
}

static void record_msr_intercept_status_for_kvm(struct kvm_vcpu *vcpu, u32 msr,
						bool enable)
{
	msr_control(vcpu, msr, enable, false);
}

static bool monitor_msrw_fct_kvmi(struct kvm_vcpu *vcpu, u32 msr, bool enable)
{
	bool ret = false;

	if (enable) {
		if (static_call(kvm_x86_msr_write_intercepted)(vcpu, msr))
			record_msr_intercept_status_for_kvm(vcpu, msr, true);
	} else {
		if (unlikely(!msr_intercepted_by_kvmi(vcpu, msr)))
			ret = true;

		if (msr_intercepted_by_kvm(vcpu, msr))
			ret = true;
	}

	record_msr_intercept_status_for_kvmi(vcpu, msr, enable);

	return ret;
}

static bool monitor_msrw_fct_kvm(struct kvm_vcpu *vcpu, u32 msr, bool enable)
{
	bool ret = false;

	if (!(msr_intercepted_by_kvmi(vcpu, msr)))
		return false;

	if (!enable)
		ret = true;

	record_msr_intercept_status_for_kvm(vcpu, msr, enable);

	return ret;
}

static unsigned int msr_mask_to_base(struct kvm_vcpu *vcpu, unsigned long *mask)
{
	if (mask == vcpu->arch.kvmi->msrw.kvmi_mask.high)
		return 0xc0000000;

	return 0;
}

void kvmi_control_msrw_intercept(struct kvm_vcpu *vcpu, u32 msr, bool enable)
{
	vcpu->arch.kvmi->msrw.monitor_fct = monitor_msrw_fct_kvmi;
	static_call(kvm_x86_control_msr_intercept)(vcpu, msr, MSR_TYPE_W,
						   enable);
	vcpu->arch.kvmi->msrw.monitor_fct = monitor_msrw_fct_kvm;
}

static void kvmi_arch_disable_msrw_intercept(struct kvm_vcpu *vcpu,
					     unsigned long *mask)
{
	unsigned int msr_base = msr_mask_to_base(vcpu, mask);
	int offset = -1;

	for (;;) {
		offset = find_next_bit(mask, KVMI_NUM_MSR, offset + 1);

		if (offset >= KVMI_NUM_MSR)
			break;

		kvmi_control_msrw_intercept(vcpu, msr_base + offset, false);
	}

	bitmap_zero(mask, KVMI_NUM_MSR);
}
#endif

int kvmi_arch_cmd_control_intercept(struct kvm_vcpu *vcpu,
				    unsigned int event_id, bool enable)
{
	int err = 0;
#if 0
	switch (event_id) {
	case KVMI_VCPU_EVENT_BREAKPOINT:
		err = kvmi_control_bp_intercept(vcpu, enable);
		break;
	case KVMI_VCPU_EVENT_DESCRIPTOR:
		err = kvmi_control_desc_intercept(vcpu, enable);
		break;
	default:
		break;
	}
#endif
	return err;
}

void kvmi_arch_breakpoint_event(struct kvm_vcpu *vcpu, u64 gva, u8 insn_len)
{
#if 0
	u32 action;
	u64 gpa;

	gpa = kvm_mmu_gva_to_gpa_system(vcpu, gva, 0, NULL);

	action = kvmi_msg_send_vcpu_bp(vcpu, gpa, insn_len);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		kvm_queue_exception(vcpu, BP_VECTOR);
		break;
	case KVMI_EVENT_ACTION_RETRY:
		/* rip was most likely adjusted past the INT 3 instruction */
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
	}
#endif
}

static void kvmi_arch_restore_interception(struct kvm_vcpu *vcpu)
{
#if 0
	struct kvmi_interception *arch_vcpui = vcpu->arch.kvmi;

	kvmi_arch_disable_bp_intercept(vcpu);
	kvmi_arch_disable_cr3w_intercept(vcpu);
	kvmi_arch_disable_desc_intercept(vcpu);
	kvmi_arch_disable_msrw_intercept(vcpu, arch_vcpui->msrw.kvmi_mask.low);
	kvmi_arch_disable_msrw_intercept(vcpu, arch_vcpui->msrw.kvmi_mask.high);
#endif
}

bool kvmi_arch_clean_up_interception(struct kvm_vcpu *vcpu)
{
	struct kvmi_interception *arch_vcpui = vcpu->arch.kvmi;

	if (!arch_vcpui)// || !arch_vcpui->cleanup)
		return false;

	if (arch_vcpui->restore_interception)
		kvmi_arch_restore_interception(vcpu);

	return true;
}

bool kvmi_arch_vcpu_alloc_interception(struct kvm_vcpu *vcpu)
{
	struct kvmi_interception *arch_vcpui;

	arch_vcpui = kzalloc(sizeof(*arch_vcpui), GFP_KERNEL);
	if (!arch_vcpui)
		return false;

#if 0
	arch_vcpui->breakpoint.monitor_fct = monitor_bp_fct_kvm;
	arch_vcpui->cr3w.monitor_fct = monitor_cr3w_fct_kvm;
	arch_vcpui->descriptor.monitor_fct = monitor_desc_fct_kvm;
	arch_vcpui->msrw.monitor_fct = monitor_msrw_fct_kvm;
#endif

	/*
	 * paired with:
	 *  - kvmi_monitor_bp_intercept()
	 *  - kvmi_monitor_cr3w_intercept()
	 *  - kvmi_monitor_desc_intercept()
	 *  - kvmi_monitor_msrw_intercept()
	 */
	smp_wmb();
	WRITE_ONCE(vcpu->arch.kvmi, arch_vcpui);

	return true;
}

void kvmi_arch_vcpu_free_interception(struct kvm_vcpu *vcpu)
{
	kfree(vcpu->arch.kvmi);
	WRITE_ONCE(vcpu->arch.kvmi, NULL);
}

bool kvmi_arch_vcpu_introspected(struct kvm_vcpu *vcpu)
{
	return !!READ_ONCE(vcpu->arch.kvmi);
}

void kvmi_arch_request_interception_cleanup(struct kvm_vcpu *vcpu,
					    bool restore_interception)
{
	struct kvmi_interception *arch_vcpui = READ_ONCE(vcpu->arch.kvmi);

	if (arch_vcpui) {
		arch_vcpui->restore_interception = restore_interception;
		//arch_vcpui->cleanup = true;
	}
}

int kvmi_arch_cmd_vcpu_control_cr(struct kvm_vcpu *vcpu, int cr, bool enable)
{
#if 0
	if (cr == 3)
		kvmi_control_cr3w_intercept(vcpu, enable);

	if (enable)
		set_bit(cr, VCPUI(vcpu)->arch.cr_mask);
	else
		clear_bit(cr, VCPUI(vcpu)->arch.cr_mask);
#endif

	return 0;
}

#if 0
static bool __kvmi_cr_event(struct kvm_vcpu *vcpu, unsigned int cr,
			    u64 old_value, unsigned long *new_value)
{
	u64 reply_value;
	u32 action;
	bool ret;

	if (!test_bit(cr, VCPUI(vcpu)->arch.cr_mask))
		return true;

	action = kvmi_msg_send_vcpu_cr(vcpu, cr, old_value, *new_value,
				       &reply_value);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		*new_value = reply_value;
		ret = true;
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
		ret = false;
	}

	return ret;
}
#endif

bool kvmi_cr_event(struct kvm_vcpu *vcpu, unsigned int cr,
		   unsigned long old_value, unsigned long *new_value)
{
#if 0
	struct kvm_introspection *kvmi;
	bool ret = true;

	if (old_value == *new_value)
		return true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_CR))
		ret = __kvmi_cr_event(vcpu, cr, old_value, new_value);

	kvmi_put(vcpu->kvm);

	return ret;
#endif
	return true;
}

bool kvmi_cr3_intercepted(struct kvm_vcpu *vcpu)
{
#if 0
	struct kvm_introspection *kvmi;
	bool ret;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return false;

	ret = test_bit(3, VCPUI(vcpu)->arch.cr_mask);

	kvmi_put(vcpu->kvm);

	return ret;
#endif
	return false;
}
EXPORT_SYMBOL(kvmi_cr3_intercepted);

int kvmi_arch_cmd_vcpu_inject_exception(struct kvm_vcpu *vcpu,
					const struct kvmi_vcpu_inject_exception *req)
{
	struct kvm_vcpu_arch_introspection *arch = &VCPUI(vcpu)->arch;

	arch->exception.pending = true;
	arch->exception.nr = req->nr;
	arch->exception.address = req->address;

	return 0;
}

static void kvmi_queue_exception(struct kvm_vcpu *vcpu)
{
	// TODO:
	/*struct kvm_vcpu_arch_introspection *arch = &VCPUI(vcpu)->arch;
	struct x86_exception e = {
		.vector = arch->exception.nr,
		.error_code_valid = arch->exception.error_code_valid,
		.error_code = arch->exception.error_code,
		.address = arch->exception.address,
	};

	if (e.vector == PF_VECTOR)
		kvm_inject_page_fault(vcpu, &e);
	else if (e.error_code_valid)
		kvm_queue_exception_e(vcpu, e.vector, e.error_code);
	else
		kvm_queue_exception(vcpu, e.vector);*/
}

static void kvmi_save_injected_event(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);

	vcpui->arch.exception.error_code = 0;
	vcpui->arch.exception.error_code_valid = false;

	vcpui->arch.exception.nr = vcpu->arch.fault.esr_el2;
	vcpui->arch.exception.address = vcpu->arch.fault.far_el2;
}

static void kvmi_inject_pending_exception(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);

	if (!vcpu->arch.exception.injected) {
		kvmi_queue_exception(vcpu);
		kvm_inject_pending_exception(vcpu);
	}

	kvmi_save_injected_event(vcpu);

	vcpui->arch.exception.pending = false;
	vcpui->arch.exception.send_event = true;
	kvm_make_request(KVM_REQ_INTROSPECTION, vcpu);
}

void kvmi_enter_guest(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_introspection *vcpui;
	struct kvm_introspection *kvmi;

	kvmi = kvmi_get(vcpu->kvm);
	if (kvmi) {
		vcpui = VCPUI(vcpu);

		if (vcpui->singlestep.loop)
			kvmi_arch_start_singlestep(vcpu);
		else if (vcpui->arch.exception.pending)
			kvmi_inject_pending_exception(vcpu);

		kvmi_put(vcpu->kvm);
	}
}

static void kvmi_send_trap_event(struct kvm_vcpu *vcpu)
{
	u32 action;

	action = kvmi_msg_send_vcpu_trap(vcpu);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
	}
}

void kvmi_arch_send_pending_event(struct kvm_vcpu *vcpu)
{
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);

	if (vcpui->arch.exception.send_event) {
		vcpui->arch.exception.send_event = false;
		kvmi_send_trap_event(vcpu);
	}
}

#if 0
static void __kvmi_xsetbv_event(struct kvm_vcpu *vcpu, u8 xcr,
				u64 old_value, u64 new_value)
{
	u32 action;

	action = kvmi_msg_send_vcpu_xsetbv(vcpu, xcr, old_value, new_value);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
	}
}
#endif

void kvmi_xsetbv_event(struct kvm_vcpu *vcpu)
{
#if 0
	struct kvm_introspection *kvmi;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return;

	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_XSETBV))
		__kvmi_xsetbv_event(vcpu, xcr, old_value, new_value);

	kvmi_put(vcpu->kvm);
#endif
}

#if 0
static bool __kvmi_descriptor_event(struct kvm_vcpu *vcpu, u8 descriptor,
				    bool write)
{
	bool ret = false;
	u32 action;

	action = kvmi_msg_send_vcpu_descriptor(vcpu, descriptor, write);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		ret = true;
		break;
	case KVMI_EVENT_ACTION_RETRY:
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
	}

	return ret;
}
#endif

bool kvmi_descriptor_event(struct kvm_vcpu *vcpu, u8 descriptor, u8 write)
{
	struct kvm_introspection *kvmi;
	bool ret = true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

#if 0
	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_DESCRIPTOR))
		ret = __kvmi_descriptor_event(vcpu, descriptor, write);
#endif

	kvmi_put(vcpu->kvm);

	return ret;
}
EXPORT_SYMBOL(kvmi_descriptor_event);

bool kvmi_msrw_intercept_originator(struct kvm_vcpu *vcpu)
{
#if 0
	struct kvmi_interception *arch_vcpui;

	if (!vcpu)
		return false;

	arch_vcpui = READ_ONCE(vcpu->arch.kvmi);

	return (arch_vcpui &&
		arch_vcpui->msrw.monitor_fct == monitor_msrw_fct_kvmi);
#endif
	return false;
}
EXPORT_SYMBOL(kvmi_msrw_intercept_originator);

#if 0
static bool __kvmi_msr_event(struct kvm_vcpu *vcpu, struct msr_data *msr)
{
	struct msr_data old_msr = {
		.host_initiated = true,
		.index = msr->index,
	};
	u64 reply_value;
	u32 action;
	bool ret;

	if (!test_msr_mask(vcpu, msr->index, true))
		return true;
	if (static_call(kvm_x86_get_msr)(vcpu, &old_msr))
		return true;
	if (old_msr.data == msr->data)
		return true;

	action = kvmi_msg_send_vcpu_msr(vcpu, msr->index, old_msr.data,
					msr->data, &reply_value);
	switch (action) {
	case KVMI_EVENT_ACTION_CONTINUE:
		msr->data = reply_value;
		ret = true;
		break;
	default:
		kvmi_handle_common_event_actions(vcpu, action);
		ret = false;
	}

	return ret;
}
#endif

bool kvmi_msr_event(struct kvm_vcpu *vcpu, struct msr_data *msr)
{
	struct kvm_introspection *kvmi;
	bool ret = true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

#if 0
	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_MSR))
		ret = __kvmi_msr_event(vcpu, msr);
#endif
	kvmi_put(vcpu->kvm);

	return ret;
}

#if 0
static const struct {
	unsigned int allow_bit;
	enum kvm_page_track_mode track_mode;
} track_modes[] = {
	{ KVMI_PAGE_ACCESS_R, KVM_PAGE_TRACK_PREREAD },
	{ KVMI_PAGE_ACCESS_W, KVM_PAGE_TRACK_PREWRITE },
	{ KVMI_PAGE_ACCESS_X, KVM_PAGE_TRACK_PREEXEC },
};
#endif

void kvmi_arch_update_page_tracking(struct kvm *kvm,
				    struct kvm_memory_slot *slot,
				    struct kvmi_mem_access *m)
{
#if 0
	struct kvmi_arch_mem_access *arch = &m->arch;
	int i;

	if (!slot) {
		slot = gfn_to_memslot(kvm, m->gfn);
		if (!slot)
			return;
	}

	for (i = 0; i < ARRAY_SIZE(track_modes); i++) {
		unsigned int allow_bit = track_modes[i].allow_bit;
		enum kvm_page_track_mode mode = track_modes[i].track_mode;
		bool slot_tracked = test_bit(slot->id, arch->active[mode]);

		if (m->access & allow_bit) {
			if (slot_tracked) {
				kvm_slot_page_track_remove_page(kvm, slot,
								m->gfn, mode);
				clear_bit(slot->id, arch->active[mode]);
			}
		} else if (!slot_tracked) {
			kvm_slot_page_track_add_page(kvm, slot, m->gfn, mode);
			set_bit(slot->id, arch->active[mode]);
		}
	}
#endif
}

void kvmi_arch_hook(struct kvm *kvm)
{
#if 0
	struct kvm_introspection *kvmi = KVMI(kvm);

	kvmi->arch.kptn_node.track_preread = kvmi_track_preread;
	kvmi->arch.kptn_node.track_prewrite = kvmi_track_prewrite;
	kvmi->arch.kptn_node.track_preexec = kvmi_track_preexec;
	kvmi->arch.kptn_node.track_create_slot = kvmi_track_create_slot;
	kvmi->arch.kptn_node.track_flush_slot = kvmi_track_flush_slot;

	kvm_page_track_register_notifier(kvm, &kvmi->arch.kptn_node);
#endif
}

void kvmi_arch_unhook(struct kvm *kvm)
{
#if 0
	struct kvm_introspection *kvmi = KVMI(kvm);

	kvm_page_track_unregister_notifier(kvm, &kvmi->arch.kptn_node);
#endif
}

#if 0
static bool kvmi_track_preread(struct kvm_vcpu *vcpu, gpa_t gpa, gva_t gva,
			       int bytes,
			       struct kvm_page_track_notifier_node *node)
{
	struct kvm_introspection *kvmi;
	bool ret = true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_PF))
		ret = kvmi_pf_event(vcpu, gpa, gva, KVMI_PAGE_ACCESS_R);

	kvmi_put(vcpu->kvm);

	return ret;
}

static bool kvmi_track_prewrite(struct kvm_vcpu *vcpu, gpa_t gpa, gva_t gva,
				const u8 *new, int bytes,
				struct kvm_page_track_notifier_node *node)
{
	struct kvm_introspection *kvmi;
	bool ret = true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_PF))
		ret = kvmi_pf_event(vcpu, gpa, gva, KVMI_PAGE_ACCESS_W);

	kvmi_put(vcpu->kvm);

	return ret;
}

static bool kvmi_track_preexec(struct kvm_vcpu *vcpu, gpa_t gpa, gva_t gva,
			       struct kvm_page_track_notifier_node *node)
{
	struct kvm_introspection *kvmi;
	bool ret = true;

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return true;

	if (is_vcpu_event_enabled(vcpu, KVMI_VCPU_EVENT_PF))
		ret = kvmi_pf_event(vcpu, gpa, gva, KVMI_PAGE_ACCESS_X);

	kvmi_put(vcpu->kvm);

	return ret;
}

static void kvmi_track_create_slot(struct kvm *kvm,
				   struct kvm_memory_slot *slot,
				   unsigned long npages,
				   struct kvm_page_track_notifier_node *node)
{
	struct kvm_introspection *kvmi;

	kvmi = kvmi_get(kvm);
	if (!kvmi)
		return;

	kvmi_add_memslot(kvm, slot, npages);

	kvmi_put(kvm);
}

static void kvmi_track_flush_slot(struct kvm *kvm, struct kvm_memory_slot *slot,
				  struct kvm_page_track_notifier_node *node)
{
	struct kvm_introspection *kvmi;

	kvmi = kvmi_get(kvm);
	if (!kvmi)
		return;

	kvmi_remove_memslot(kvm, slot);

	kvmi_put(kvm);
}
#endif

void kvmi_arch_features(struct kvmi_features *feat)
{
	//feat->singlestep = !!kvm_x86_ops.control_singlestep;
	feat->singlestep = false;
}

void kvmi_arch_start_singlestep(struct kvm_vcpu *vcpu)
{
	//static_call(kvm_x86_control_singlestep)(vcpu, true);
}

void kvmi_arch_stop_singlestep(struct kvm_vcpu *vcpu)
{
	//static_call(kvm_x86_control_singlestep)(vcpu, false);
}

bool kvmi_update_ad_flags(struct kvm_vcpu *vcpu)
{
	struct kvm_introspection *kvmi;
	bool ret = false;
#if 0
	gva_t gva;
	gpa_t gpa;
#endif

	kvmi = kvmi_get(vcpu->kvm);
	if (!kvmi)
		return false;

#if 0
	gva = static_call(kvm_x86_fault_gla)(vcpu);
	if (gva == ~0ull)
		goto out;

	gpa = kvm_mmu_gva_to_gpa_system(vcpu, gva, PFERR_WRITE_MASK, NULL);
	if (gpa == UNMAPPED_GVA) {
		struct x86_exception exception = { };

		gpa = kvm_mmu_gva_to_gpa_system(vcpu, gva, 0, &exception);
	}

	ret = (gpa != UNMAPPED_GVA);
	out:
#endif
	kvmi_put(vcpu->kvm);

	return ret;
}
