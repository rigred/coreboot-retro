/* SPDX-License-Identifier: GPL-2.0-only */


#include <device/pci_ops.h>
#include <southbridge/intel/i82371eb/i82371eb.h>
#include <acpi/acpi.h>
#include <device/device.h>
#include <identity.h>
#include <smbios.h>
#include <string.h>

struct chip_operations mainboard_ops = {
	.init = mainboard_init
};

void mainboard_fill_fadt(acpi_fadt_t *fadt)
{
	fadt->preferred_pm_profile = PM_DESKTOP;
	fadt->iapc_boot_arch |= ACPI_FADT_LEGACY_DEVICES | ACPI_FADT_8042;
}

/**
 * Mainboard specific enables.
 *
 * @param chip_info Ignored
 */
static void mainboard_init(void *chip_info)
{
	
}

u8 smbios_mainboard_feature_flags(void)
{
	return SMBIOS_FEATURE_FLAGS_HOSTING_BOARD | SMBIOS_FEATURE_FLAGS_REPLACEABLE;
}

smbios_wakeup_type smbios_system_wakeup_type(void)
{
	return SMBIOS_WAKEUP_TYPE_POWER_SWITCH;
}

const char *smbios_system_product_name(void)
{
	return "MS-6117";
}

const char *smbios_mainboard_product_name(void)
{
	if (CONFIG(BOARD_MSI_MS6117)) {
			return "MS-6117";
	}

	return mainboard_part_number;
}

/* Only baseboard serial number is populated */
const char *smbios_system_serial_number(void)
{
	return "Default string";
}

const char *smbios_system_sku(void)
{
	return "Default string";
}

#if CONFIG(GENERATE_SMBIOS_TABLES)
static const struct port_information smbios_type8_info[] = {
	{
		.internal_reference_designator = "PS/2 Keyboard",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "Keyboard",
		.external_connector_type       = CONN_PS_2,
		.port_type                     = TYPE_KEYBOARD_PORT
	},
	{
		.internal_reference_designator = "PS/2 Mouse",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "PS2Mouse",
		.external_connector_type       = CONN_PS_2,
		.port_type                     = TYPE_MOUSE_PORT
	},
	{
		.internal_reference_designator = "USB1",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "USB2",
		.internal_connector_type       = CONN_NONE,
		.external_reference_designator = "USB 2.0 Type-A",
		.external_connector_type       = CONN_ACCESS_BUS_USB,
		.port_type                     = TYPE_USB
	},
	{
		.internal_reference_designator = "JC1 - CHASSIS INTRUSION",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JFP1 - FRONT PANEL",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "BZ1 - PC SPEAKER",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "JBAT1 - CLEAR CMOS",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "CFAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "PSFAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SFAN1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "ATX_PWR1",
		.internal_connector_type       = CONN_OTHER,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "COM1",
		.internal_connector_type       = CONN_9_PIN_DUAL_INLINE,
		.external_reference_designator = "DB-9 Serial COM1",
		.external_connector_type       = CONN_DB_9_PIN_MALE,
		.port_type                     = TYPE_SERIAL_PORT_16450_COMPATIBLE
	},
	{
		.internal_reference_designator = "COM2",
		.internal_connector_type       = CONN_9_PIN_DUAL_INLINE,
		.external_reference_designator = "DB-9 Serial COM2",
		.external_connector_type       = CONN_DB_9_PIN_MALE,
		.port_type                     = TYPE_SERIAL_PORT_16450_COMPATIBLE
	},
	{
		.internal_reference_designator = "LPT1",
		.internal_connector_type       = CONN_25_PIN_DUAL_INLINE,
		.external_reference_designator = "DB-25 Parrallel LPT1",
		.external_connector_type       = CONN_DB_25_PIN_FEMALE,
		.port_type                     = TYPE_PARALLEL_PORT_ECP_EPP
	},
	{
		.internal_reference_designator = "FDD",
		.internal_connector_type       = CONN_ON_BOARD_FLOPPY,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_8251_FIFO_COMPATIBLE
	},
	{
		.internal_reference_designator = "PRIMARY IDE",
		.internal_connector_type       = CONN_ON_BOARD_IDE,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
	{
		.internal_reference_designator = "SECONDARY IDE",
		.internal_connector_type       = CONN_ON_BOARD_IDE,
		.external_reference_designator = "",
		.external_connector_type       = CONN_NONE,
		.port_type                     = TYPE_OTHER_PORT
	},
};

static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	// add port information
	len += smbios_write_type8(
		current, handle,
		smbios_type8_info,
		ARRAY_SIZE(smbios_type8_info)
		);

	return len;
}
#endif

static void mainboard_enable(struct device *dev)
{
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
#endif
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mainboard_enable,
};
