bootblock-y += bootblock.c
romstage-$(CONFIG_BOARD_SHUTTLE_AB61) += variants/$(VARIANT_DIR)/romstage.c

ramstage-$(CONFIG_GENERATE_PIRQ_TABLE) += variants/$(VARIANT_DIR)/irq_tables.c
ramstage-$(CONFIG_GENERATE_MP_TABLE) += variants/$(VARIANT_DIR)/mptable.c
