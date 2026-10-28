/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef I82371EB_SMI_H
#define I82371EB_SMI_H
void i82371eb_smm_prepare(void);
void i82371eb_smm_enable_apmc(void);
/* Platform confirms SMRAM is locked before post-device SMI probes. */
void i82371eb_smm_mark_ready(void);
#endif
