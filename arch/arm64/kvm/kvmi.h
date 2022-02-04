/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ARCH_ARM64_KVM_KVMI_H
#define ARCH_ARM64_KVM_KVMI_H

int kvmi_arch_cmd_vcpu_get_registers(struct kvm_vcpu *vcpu,
				struct kvmi_vcpu_get_registers_reply *rpl);
void kvmi_arch_cmd_vcpu_set_registers(struct kvm_vcpu *vcpu,
				      const struct kvm_regs *regs);
int kvmi_arch_cmd_vcpu_control_cr(struct kvm_vcpu *vcpu, int cr, bool enable);
int kvmi_arch_cmd_vcpu_inject_exception(struct kvm_vcpu *vcpu,
				const struct kvmi_vcpu_inject_exception *req);

u32 kvmi_msg_send_vcpu_cr(struct kvm_vcpu *vcpu, u32 cr, u64 old_value,
			  u64 new_value, u64 *ret_value);
u32 kvmi_msg_send_vcpu_trap(struct kvm_vcpu *vcpu);
u32 kvmi_msg_send_vcpu_xsetbv(struct kvm_vcpu *vcpu, u8 xcr,
			      u64 old_value, u64 new_value);
u32 kvmi_msg_send_vcpu_descriptor(struct kvm_vcpu *vcpu, u8 desc, bool write);
void kvmi_control_msrw_intercept(struct kvm_vcpu *vcpu, u32 msr, bool enable);
u32 kvmi_msg_send_vcpu_msr(struct kvm_vcpu *vcpu, u32 msr, u64 old_value,
			   u64 new_value, u64 *ret_value);

#endif
