// Garnison de bâtiment : le placement GM suivant est posté dans le bâtiment visé.
// Porté depuis Training, sans dépendance à cet addon.
//------------------------------------------------------------------------------------------------
modded class SCR_PlacingEditorComponent
{
	protected bool m_bHAM_GarrisonArmed;
	protected bool m_bHAM_GarrisonHooked;
	protected vector m_vHAM_GarrisonBuildingPos;
	protected float m_fHAM_GarrisonArmTimeMs;

	protected static const float HAM_GARRISON_ARM_TIMEOUT_MS = 120000;
	protected static const float HAM_GARRISON_POS_TOLERANCE = 3.0;

	void HAM_ArmGarrison(vector buildingPos)
	{
		Rpc(HAM_ArmGarrison_S, buildingPos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArmGarrison_S(vector buildingPos)
	{
		m_bHAM_GarrisonArmed = true;
		m_vHAM_GarrisonBuildingPos = buildingPos;
		m_fHAM_GarrisonArmTimeMs = GetGame().GetWorld().GetWorldTime();

		if (!m_bHAM_GarrisonHooked)
		{
			m_bHAM_GarrisonHooked = true;
			GetOnPlaceEntityServer().Insert(HAM_OnPlacedEntityServer);
		}
	}

	protected void HAM_OnPlacedEntityServer(int prefabID, SCR_EditableEntityComponent entity, int playerID)
	{
		if (!m_bHAM_GarrisonArmed)
			return;

		m_bHAM_GarrisonArmed = false;
		if (!entity || !entity.GetOwner())
			return;

		if (GetGame().GetWorld().GetWorldTime() - m_fHAM_GarrisonArmTimeMs > HAM_GARRISON_ARM_TIMEOUT_MS)
			return;

		vector delta = entity.GetOwner().GetOrigin() - m_vHAM_GarrisonBuildingPos;
		delta[1] = 0;
		if (delta.Length() > HAM_GARRISON_POS_TOLERANCE)
			return;

		HAM_BuildingGarrison.HAM_GarrisonPlacedEntity(entity, m_vHAM_GarrisonBuildingPos);
	}
}
