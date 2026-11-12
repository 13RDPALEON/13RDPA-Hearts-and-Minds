//------------------------------------------------------------------------------------------------
// Deux arrêts VM de l'inventaire vanilla :
// - une arme de mod sans pastille de chargeur (ouverture d'arsenal, déploiement d'un véhicule) ;
// - le reste est dans GetEntityCatalogOfType pour la clé de réparation.
//------------------------------------------------------------------------------------------------
modded class SCR_InventoryMenuUI
{
	//------------------------------------------------------------------------------------------------
	//! Changer de page détruit la fiche pendant qu'un rappel la déplace encore.
	protected override void UpdateItemInfoPosition(bool postponed = false)
	{
		if (!m_pItemInfo || !m_pItemInfo.GetInfoWidget())
			return;

		super.UpdateItemInfoPosition(postponed);
	}
}

modded class SCR_InventorySlotUI
{
	protected override void Init()
	{
		m_iPage = -1;

		m_wTextQuickSlot = TextWidget.Cast(m_widget.FindAnyWidget("TextQuickSlot"));
		m_sItemName = GetItemName();

		if (m_pItem)
			m_pItem.m_OnLockedStateChangedInvoker.Insert(OnChangeLockState);

		SetItemFunctionality();
		if (m_eSlotFunction == ESlotFunction.TYPE_WEAPON)
		{
			m_wMagazineNumber = TextWidget.Cast(m_widget.FindAnyWidget("magazineCount"));
			m_wCurrentMagazine = m_widget.FindAnyWidget("currentMagazine");
			if (m_wCurrentMagazine)
			{
				m_wCurrentMagazineAmmoCount = ImageWidget.Cast(m_widget.FindAnyWidget("currentMagazineAmmoCount"));
				ImageWidget outline = ImageWidget.Cast(m_wCurrentMagazine.FindAnyWidget("Outline"));
				BaseMuzzleComponent muzzleComp = null;
				if (outline && m_wCurrentMagazineAmmoCount && m_pItem && m_pItem.GetOwner())
					muzzleComp = BaseMuzzleComponent.Cast(m_pItem.GetOwner().FindComponent(BaseMuzzleComponent));

				MuzzleUIInfo info = null;
				if (muzzleComp)
					info = MuzzleUIInfo.Cast(muzzleComp.GetUIInfo());

				if (info && info.m_MagIndicator)
				{
					if (muzzleComp.IsInherited(MuzzleInMagComponent))
					{
						m_wCurrentMagazineAmmoCount.LoadImageFromSet(0, info.m_MagIndicator.m_sImagesetIcons, info.m_MagIndicator.m_sOutline);
						outline.LoadImageFromSet(0, info.m_MagIndicator.m_sImagesetIcons, info.m_MagIndicator.m_sOutline);
					}
					else
					{
						m_wCurrentMagazineAmmoCount.LoadImageFromSet(0, info.m_MagIndicator.m_sImagesetIcons, info.m_MagIndicator.m_sProgress);
						outline.LoadImageFromSet(0, info.m_MagIndicator.m_sImagesetIcons, info.m_MagIndicator.m_sProgress);
					}
				}
			}
		}

		SetSlotVisible(m_bVisible);

		if (!BaseInventoryStorageComponent.Cast(m_pItem) && !WeaponAttachmentsStorageComponent.Cast(m_pItem))
			return;

		if (m_pStorageUI && m_pStorageUI.GetInventoryMenuHandler())
			m_pStorageUI.GetInventoryMenuHandler().RegisterUIStorage(this);
	}
}
