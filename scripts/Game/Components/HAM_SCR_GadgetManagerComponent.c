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
	protected int m_iHAM_NullGadgetLog;

	//! Le vanilla imprime une erreur à chaque appel si le tableau copié contient un null.
	//! On travaille sur cette copie (GetGadgetsByType fait arr.Copy) : le tableau interne reste intact.
	override IEntity GetGadgetByType(EGadgetType type)
	{
		array<SCR_GadgetComponent> gadgets = GetGadgetsByType(type);
		if (!gadgets || gadgets.IsEmpty())
			return null;

		int nulls = 0;
		for (int i = gadgets.Count() - 1; i >= 0; i--)
		{
			if (gadgets[i])
				continue;

			nulls = nulls + 1;
			gadgets.Remove(i);
		}

		if (nulls > 0)
			HAM_LogNullGadgets(type, nulls);

		if (gadgets.IsEmpty())
			return null;

		return gadgets[0].GetOwner();
	}

	protected void HAM_LogNullGadgets(EGadgetType type, int nulls)
	{
		int now = System.GetTickCount();
		if (m_iHAM_NullGadgetLog != 0)
		{
			int since = now - m_iHAM_NullGadgetLog;
			if (since >= 0 && since < 30000)
				return;
		}

		m_iHAM_NullGadgetLog = now;
		IEntity owner = GetOwner();
		string prefab = "?";
		int playerId = 0;
		if (owner)
		{
			if (owner.GetPrefabData())
				prefab = HAM_Util.PrefabFileName(owner.GetPrefabData().GetPrefabName());

			PlayerManager players = GetGame().GetPlayerManager();
			if (players)
				playerId = players.GetPlayerIdFromControlledEntity(owner);
		}

		Print(string.Format("[13RDPA H&M] Gadget nul retiré : %1, %2 entrée(s), joueur %3, propriétaire %4", typename.EnumToString(EGadgetType, type), nulls, HAM_Util.PlayerTag(playerId), prefab), LogLevel.WARNING);
	}

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
