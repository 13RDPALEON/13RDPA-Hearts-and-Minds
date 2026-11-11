modded class SCR_GadgetComponent
{
	[RplProp()]
	protected bool m_bHAM_ListenOnly;

	void HAM_MarkListenOnly()
	{
		m_bHAM_ListenOnly = true;
		Replication.BumpMe();
	}

	bool HAM_IsListenOnly()
	{
		return m_bHAM_ListenOnly;
	}
}

modded class SCR_GadgetManagerComponent
{
	override void OnItemAdded(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
		if (gadget && gadget.HAM_IsListenOnly())
		{
			if (!m_VONController && GetGame().GetPlayerController())
				m_VONController = SCR_VONController.Cast(GetGame().GetPlayerController().FindComponent(SCR_VONController));

			SCR_RadioComponent radioGadget = SCR_RadioComponent.Cast(gadget);
			if (!radioGadget || !m_VONController)
				return;

			BaseRadioComponent radioComp = radioGadget.GetRadioComponent();
			if (!radioComp || radioComp.TransceiversCount() < 1)
				return;

			SCR_VONEntryRadio radioEntry = new SCR_VONEntryRadio();
			radioEntry.SetRadioEntry(radioComp.GetTransceiver(0), 100, gadget);
			m_VONController.AddEntry(radioEntry);
			return;
		}

		super.OnItemAdded(item, storageOwner);
	}

	override protected void RegisterInitialGadgets(IEntity character)
	{
		super.RegisterInitialGadgets(character);

		array<SCR_GadgetComponent> gadgets = GetGadgetsByType(EGadgetType.NIGHT_VISION);
		if (!gadgets)
			return;

		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || gadget.HAM_IsListenOnly() || gadget.GetMode() == EGadgetMode.IN_SLOT)
				continue;

			IEntity item = gadget.GetOwner();
			if (!item)
				continue;

			InventoryItemComponent invComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (!invComp)
				continue;

			if (!LoadoutSlotInfo.Cast(invComp.GetParentSlot()))
				continue;

			SetGadgetMode(item, EGadgetMode.IN_SLOT);
		}
	}
}
