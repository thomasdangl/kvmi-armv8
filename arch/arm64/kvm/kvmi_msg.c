// SPDX-License-Identifier: GPL-2.0
/*
 * KVM introspection (message handling) - ARM64
 *
 * Copyright (C) 2020-2021 Bitdefender S.R.L.
 *
 */

#include "../../../virt/kvm/introspection/kvmi_int.h"
#include "kvmi.h"

static int handle_vcpu_get_info(const struct kvmi_vcpu_msg_job *job,
				const struct kvmi_msg_hdr *msg,
				const void *req)
{
	struct kvmi_vcpu_get_info_reply rpl;

	memset(&rpl, 0, sizeof(rpl));
#if 0
	if (kvm_has_tsc_control)
		rpl.tsc_speed = 1000ul * job->vcpu->arch.virtual_tsc_khz;
#endif

	return kvmi_msg_vcpu_reply(job, msg, 0, &rpl, sizeof(rpl));
}

static int handle_vcpu_get_registers(const struct kvmi_vcpu_msg_job *job,
				     const struct kvmi_msg_hdr *msg,
				     const void *req)
{
	struct kvmi_vcpu_get_registers_reply rpl;
	memset(&rpl, 0, sizeof(rpl));

	kvmi_arch_cmd_vcpu_get_registers(job->vcpu, &rpl);

	return kvmi_msg_vcpu_reply(job, msg, 0, &rpl, sizeof(rpl));
}

static int handle_vcpu_set_registers(const struct kvmi_vcpu_msg_job *job,
				     const struct kvmi_msg_hdr *msg,
				     const void *req)
{
	const struct kvm_regs *regs = req;
	size_t cmd_size;
	int ec = 0;

	cmd_size = sizeof(struct kvmi_vcpu_hdr) + sizeof(*regs);

	if (cmd_size > msg->size)
		ec = -KVM_EINVAL;
	else if (!VCPUI(job->vcpu)->waiting_for_reply)
		ec = -KVM_EOPNOTSUPP;
	else
		kvmi_arch_cmd_vcpu_set_registers(job->vcpu, regs);

	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
}

static int handle_vcpu_get_cpuid(const struct kvmi_vcpu_msg_job *job,
				 const struct kvmi_msg_hdr *msg,
				 const void *_req)
{
#if 0
	const struct kvmi_vcpu_get_cpuid *req = _req;
	struct kvmi_vcpu_get_cpuid_reply rpl;
	struct kvm_cpuid_entry2 *entry;
	int ec = 0;

	entry = kvm_find_cpuid_entry(job->vcpu, req->function, req->index);
	if (!entry) {
		ec = -KVM_ENOENT;
	} else {
		memset(&rpl, 0, sizeof(rpl));

		rpl.eax = entry->eax;
		rpl.ebx = entry->ebx;
		rpl.ecx = entry->ecx;
		rpl.edx = entry->edx;
	}

	return kvmi_msg_vcpu_reply(job, msg, ec, &rpl, sizeof(rpl));
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_control_cr(const struct kvmi_vcpu_msg_job *job,
				  const struct kvmi_msg_hdr *msg,
				  const void *_req)
{
	const struct kvmi_vcpu_control_cr *req = _req;
	int ec;

	if (req->padding1 || req->padding2 || req->enable > 1)
		ec = -KVM_EINVAL;
	else if (req->cr != 3)
		ec = -KVM_EINVAL;
	else
		ec = kvmi_arch_cmd_vcpu_control_cr(job->vcpu, req->cr,
						   req->enable == 1);

	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
}

static int handle_vcpu_inject_exception(const struct kvmi_vcpu_msg_job *job,
					const struct kvmi_msg_hdr *msg,
					const void *_req)
{
#if 0
	const struct kvmi_vcpu_inject_exception *req = _req;
	struct kvm_vcpu *vcpu = job->vcpu;
	int ec;

	if (!kvmi_is_event_allowed(KVMI(vcpu->kvm), KVMI_VCPU_EVENT_TRAP))
		ec = -KVM_EPERM;
	else if (req->padding1 || req->padding2)
		ec = -KVM_EINVAL;
	else if (VCPUI(vcpu)->arch.exception.pending ||
			VCPUI(vcpu)->arch.exception.send_event ||
			VCPUI(vcpu)->singlestep.loop)
		ec = -KVM_EBUSY;
	else
		ec = kvmi_arch_cmd_vcpu_inject_exception(vcpu, req);

	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_get_xcr(const struct kvmi_vcpu_msg_job *job,
			       const struct kvmi_msg_hdr *msg,
			       const void *_req)
{
#if 0
	const struct kvmi_vcpu_get_xcr *req = _req;
	struct kvmi_vcpu_get_xcr_reply rpl;
	int ec = 0;

	memset(&rpl, 0, sizeof(rpl));

	if (non_zero_padding(req->padding, ARRAY_SIZE(req->padding)))
		ec = -KVM_EINVAL;
	else if (req->xcr != 0)
		ec = -KVM_EINVAL;
	else
		rpl.value = job->vcpu->arch.xcr0;

	return kvmi_msg_vcpu_reply(job, msg, ec, &rpl, sizeof(rpl));
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_get_xsave(const struct kvmi_vcpu_msg_job *job,
				 const struct kvmi_msg_hdr *msg,
				 const void *req)
{
#if 0
	struct kvmi_vcpu_get_xsave_reply *rpl;
	int err, ec = 0;

	rpl = kvmi_msg_alloc();
	if (!rpl)
		ec = -KVM_ENOMEM;
	else
		kvm_vcpu_ioctl_x86_get_xsave(job->vcpu, &rpl->xsave);

	err = kvmi_msg_vcpu_reply(job, msg, ec, rpl, sizeof(*rpl));

	kvmi_msg_free(rpl);
	return err;
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_set_xsave(const struct kvmi_vcpu_msg_job *job,
				 const struct kvmi_msg_hdr *msg,
				 const void *req)
{
#if 0
	const struct kvm_xsave *area = req;
	size_t cmd_size;
	int ec = 0;

	cmd_size = sizeof(struct kvmi_vcpu_hdr) + sizeof(*area);

	if (cmd_size > msg->size)
		ec = -KVM_EINVAL;
	else if (kvm_vcpu_ioctl_x86_set_xsave(job->vcpu,
					      (struct kvm_xsave *) area))
		ec = -KVM_EINVAL;

	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_get_mtrr_type(const struct kvmi_vcpu_msg_job *job,
				     const struct kvmi_msg_hdr *msg,
				     const void *_req)
{
#if 0
	const struct kvmi_vcpu_get_mtrr_type *req = _req;
	struct kvmi_vcpu_get_mtrr_type_reply rpl;
	gfn_t gfn;

	gfn = gpa_to_gfn(req->gpa);

	memset(&rpl, 0, sizeof(rpl));
	rpl.type = kvm_mtrr_get_guest_memory_type(job->vcpu, gfn);

	return kvmi_msg_vcpu_reply(job, msg, 0, &rpl, sizeof(rpl));
#endif
	return -KVM_EOPNOTSUPP;
}

#if 0
static bool is_valid_msr(unsigned int msr)
{
	//return msr <= 0x1fff || (msr >= 0xc0000000 && msr <= 0xc0001fff);
	return false;
}
#endif

static int handle_vcpu_control_msr(const struct kvmi_vcpu_msg_job *job,
				   const struct kvmi_msg_hdr *msg,
				   const void *_req)
{
#if 0
	const struct kvmi_vcpu_control_msr *req = _req;
	int ec = 0;

	if (req->padding1 || req->padding2 || req->enable > 1)
		ec = -KVM_EINVAL;
	else if (!is_valid_msr(req->msr))
		ec = -KVM_EINVAL;
	else if (req->enable &&
		 !kvm_msr_allowed(job->vcpu, req->msr,
				  KVM_MSR_FILTER_WRITE))
		ec = -KVM_EPERM;
	else
		kvmi_control_msrw_intercept(job->vcpu, req->msr,
					    req->enable == 1);

	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
#endif
	return -KVM_EOPNOTSUPP;
}

static int handle_vcpu_control_singlestep(const struct kvmi_vcpu_msg_job *job,
					  const struct kvmi_msg_hdr *msg,
					  const void *_req)
{
	const struct kvmi_vcpu_control_singlestep *req = _req;
	struct kvm_vcpu *vcpu = job->vcpu;
	int ec = 0;

	if (!kvmi_is_event_allowed(KVMI(vcpu->kvm),
				   KVMI_VCPU_EVENT_SINGLESTEP)) {
		ec = -KVM_EPERM;
		goto reply;
	}

	if (non_zero_padding(req->padding, ARRAY_SIZE(req->padding)) ||
	    req->enable > 1) {
		ec = -KVM_EINVAL;
		goto reply;
	}

	if (req->enable)
		kvmi_arch_start_singlestep(vcpu);
	else
		kvmi_arch_stop_singlestep(vcpu);

	VCPUI(vcpu)->singlestep.loop = !!req->enable;

reply:
	return kvmi_msg_vcpu_reply(job, msg, ec, NULL, 0);
}

static int handle_vcpu_translate_gva(const struct kvmi_vcpu_msg_job *job,
				     const struct kvmi_msg_hdr *msg,
				     const void *_req)
{
#if 0
	const struct kvmi_vcpu_translate_gva *req = _req;
	struct kvmi_vcpu_translate_gva_reply rpl;

	memset(&rpl, 0, sizeof(rpl));

	rpl.gpa = kvm_mmu_gva_to_gpa_system(job->vcpu, req->gva, 0, NULL);

	return kvmi_msg_vcpu_reply(job, msg, 0, &rpl, sizeof(rpl));
#endif
	return -KVM_EOPNOTSUPP; // must be in userland for ARM (at least for now)
}

static const kvmi_vcpu_msg_job_fct msg_vcpu[] = {
	[KVMI_VCPU_CONTROL_CR]         = handle_vcpu_control_cr,
	[KVMI_VCPU_CONTROL_MSR]        = handle_vcpu_control_msr,
	[KVMI_VCPU_CONTROL_SINGLESTEP] = handle_vcpu_control_singlestep,
	[KVMI_VCPU_GET_CPUID]          = handle_vcpu_get_cpuid,
	[KVMI_VCPU_GET_INFO]           = handle_vcpu_get_info,
	[KVMI_VCPU_GET_MTRR_TYPE]      = handle_vcpu_get_mtrr_type,
	[KVMI_VCPU_GET_REGISTERS]      = handle_vcpu_get_registers,
	[KVMI_VCPU_GET_XCR]            = handle_vcpu_get_xcr,
	[KVMI_VCPU_GET_XSAVE]          = handle_vcpu_get_xsave,
	[KVMI_VCPU_INJECT_EXCEPTION]   = handle_vcpu_inject_exception,
	[KVMI_VCPU_SET_REGISTERS]      = handle_vcpu_set_registers,
	[KVMI_VCPU_SET_XSAVE]          = handle_vcpu_set_xsave,
	[KVMI_VCPU_TRANSLATE_GVA]      = handle_vcpu_translate_gva,
};

kvmi_vcpu_msg_job_fct kvmi_arch_vcpu_msg_handler(u16 id)
{
	return id < ARRAY_SIZE(msg_vcpu) ? msg_vcpu[id] : NULL;
}

u32 kvmi_msg_send_vcpu_cr(struct kvm_vcpu *vcpu, u32 cr, u64 old_value,
			  u64 new_value, u64 *ret_value)
{
	struct kvmi_vcpu_event_cr e;
	struct kvmi_vcpu_event_cr_reply r;
	u32 action;
	int err;

	memset(&e, 0, sizeof(e));
	e.cr = cr;
	e.old_value = old_value;
	e.new_value = new_value;

	err = kvmi_send_vcpu_event(vcpu, KVMI_VCPU_EVENT_CR, &e, sizeof(e),
				   &r, sizeof(r), &action);
	if (err) {
		action = KVMI_EVENT_ACTION_CONTINUE;
		*ret_value = new_value;
	} else {
		*ret_value = r.new_val;
	}

	return action;
}

u32 kvmi_msg_send_vcpu_trap(struct kvm_vcpu *vcpu)
{
#if 0
	struct kvm_vcpu_introspection *vcpui = VCPUI(vcpu);
	struct kvmi_vcpu_event_trap e;
	u32 action;
	int err;

	memset(&e, 0, sizeof(e));
	e.nr = vcpui->arch.exception.nr;
	e.error_code = vcpui->arch.exception.error_code;
	e.address = vcpui->arch.exception.address;

	err = __kvmi_send_vcpu_event(vcpu, KVMI_VCPU_EVENT_TRAP,
				     &e, sizeof(e), NULL, 0, &action);
	if (err)
		action = KVMI_EVENT_ACTION_CONTINUE;

	return action;
#endif
	return -KVM_EOPNOTSUPP;
}

u32 kvmi_msg_send_vcpu_xsetbv(struct kvm_vcpu *vcpu, u8 xcr,
			      u64 old_value, u64 new_value)
{
	struct kvmi_vcpu_event_xsetbv e;
	u32 action;
	int err;

	memset(&e, 0, sizeof(e));
	e.xcr = xcr;
	e.old_value = old_value;
	e.new_value = new_value;

	err = kvmi_send_vcpu_event(vcpu, KVMI_VCPU_EVENT_XSETBV,
				   &e, sizeof(e), NULL, 0, &action);
	if (err)
		action = KVMI_EVENT_ACTION_CONTINUE;

	return action;
}

u32 kvmi_msg_send_vcpu_descriptor(struct kvm_vcpu *vcpu, u8 desc, bool write)
{
	struct kvmi_vcpu_event_descriptor e;
	u32 action;
	int err;

	memset(&e, 0, sizeof(e));
	e.descriptor = desc;
	e.write = write ? 1 : 0;

	err = kvmi_send_vcpu_event(vcpu, KVMI_VCPU_EVENT_DESCRIPTOR,
				   &e, sizeof(e), NULL, 0, &action);
	if (err)
		action = KVMI_EVENT_ACTION_CONTINUE;

	return action;

}

u32 kvmi_msg_send_vcpu_msr(struct kvm_vcpu *vcpu, u32 msr, u64 old_value,
			   u64 new_value, u64 *ret_value)
{
	struct kvmi_vcpu_event_msr e;
	struct kvmi_vcpu_event_msr_reply r;
	int err, action;

	memset(&e, 0, sizeof(e));
	e.msr = msr;
	e.old_value = old_value;
	e.new_value = new_value;

	err = kvmi_send_vcpu_event(vcpu, KVMI_VCPU_EVENT_MSR, &e, sizeof(e),
				   &r, sizeof(r), &action);
	if (err) {
		action = KVMI_EVENT_ACTION_CONTINUE;
		*ret_value = new_value;
	} else {
		*ret_value = r.new_val;
	}

	return action;
}
