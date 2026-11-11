//------------------------------------------------------------------------------------------------
// Ouverture du menu véhicules depuis un panneau. 1 = sol, 2 = air.
//------------------------------------------------------------------------------------------------

class HAM_VehicleServiceOpenUserAction : ScriptedUserAction
{
	[Attribute("1", UIWidgets.EditBox, "1 = véhicules au sol, 2 = hélicoptères")]
	protected int m_iTRNBoardKind;

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override bool GetActionNameScript(out string outName)
	{
		if (m_iTRNBoardKind == 2)
			outName = "Hélicoptères";
		else
			outName = "Véhicules";

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int kind = m_iTRNBoardKind;
		if (kind != 2)
			kind = 1;

		HAM_VehicleSpawnUI.Open(kind);
	}
}

// Le véhicule choisi doit pouvoir être posé par le placement du Game Master.
modded class SCR_PlacingEditorComponentClass
{
	protected static ResourceName s_sTRNExtra;

	static void HAM_Allow(string prefab)
	{
		s_sTRNExtra = prefab;
	}

	static ResourceName HAM_Extra()
	{
		return s_sTRNExtra;
	}

	protected bool HAM_HasExtra()
	{
		return !s_sTRNExtra.IsEmpty();
	}

	override int CountPrefabs()
	{
		int count = super.CountPrefabs();
		if (HAM_HasExtra())
			count = count + 1;

		return count;
	}

	override ResourceName GetPrefab(int index)
	{
		int vanillaCount = super.CountPrefabs();
		if (HAM_HasExtra() && index == vanillaCount)
			return s_sTRNExtra;

		return super.GetPrefab(index);
	}

	override int GetPrefabID(ResourceName prefab)
	{
		int id = super.GetPrefabID(prefab);
		if (id != -1)
			return id;

		if (HAM_HasExtra() && prefab == s_sTRNExtra)
			return super.CountPrefabs();

		return -1;
	}

	override int GetPrefabs(out notnull array<ResourceName> outPrefabs, bool onlyExposed = false)
	{
		super.GetPrefabs(outPrefabs, onlyExposed);
		if (HAM_HasExtra() && outPrefabs.Find(s_sTRNExtra) == -1)
			outPrefabs.Insert(s_sTRNExtra);

		return outPrefabs.Count();
	}
}

modded class SCR_PlacingEditorComponent
{
	override bool SetSelectedPrefab(ResourceName prefab = "", bool onConfirm = false, bool showBudgetMaxNotification = true, set<SCR_EditableEntityComponent> recipients = null, SCR_BaseEditorAction sourceAction = null)
	{
		if (!m_PreviewManager && !prefab.IsEmpty())
			return false;

		bool placed = super.SetSelectedPrefab(prefab, onConfirm, showBudgetMaxNotification, recipients, sourceAction);
		if (prefab.IsEmpty())
			SCR_PlacingEditorComponentClass.HAM_Allow(string.Empty);

		return placed;
	}

	override bool CanPlaceEntityServer(IEntityComponentSource editableEntitySource, out EEditableEntityBudget blockingBudget, bool updatePreview, bool showNotification, int prefabID = -1, int playerID = -1, SCR_EditorPreviewParams params = null)
	{
		ResourceName extra = SCR_PlacingEditorComponentClass.HAM_Extra();
		if (!extra.IsEmpty())
		{
			SCR_PlacingEditorComponentClass data = SCR_PlacingEditorComponentClass.Cast(GetEditorComponentData());
			if (data && data.GetPrefab(prefabID) == extra)
			{
				blockingBudget = -1;
				return true;
			}
		}

		return super.CanPlaceEntityServer(editableEntitySource, blockingBudget, updatePreview, showNotification, prefabID, playerID, params);
	}
}

// Le clic gauche du Game Master est ignoré si le curseur touche l'interface, ou si le fantôme n'a pas bougé.
// Ici il pose le véhicule choisi. Le clic droit annule dès qu'il est enfoncé.
modded class SCR_PlacingEditorUIComponent
{
	protected bool m_bTRNListener;

	protected void HAM_CancelOnRightClick(float value, EActionTrigger reason)
	{
		if (!HAM_IsOurPrefab())
			return;

		SCR_PlacingEditorComponent placing = HAM_Placing();
		if (!placing)
			return;

		placing.SetSelectedPrefab(ResourceName.Empty, true);
	}

	override void Place(bool cancelAfterwards, bool canBePlayer = false)
	{
		if (!HAM_IsOurPrefab())
		{
			super.Place(cancelAfterwards, canBePlayer);
			return;
		}

		HAM_VehicleSpawnUI.ConfirmEditorPlace();
	}

	protected bool HAM_IsOurPrefab()
	{
		if (SCR_PlacingEditorComponentClass.HAM_Extra().IsEmpty())
			return false;

		SCR_PlacingEditorComponent placing = HAM_Placing();
		return placing && placing.IsPlacing();
	}

	protected SCR_PlacingEditorComponent HAM_Placing()
	{
		return SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
	}

	override void HandlerAttachedScripted(Widget w)
	{
		super.HandlerAttachedScripted(w);
		if (m_bTRNListener)
			return;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		input.AddActionListener("EditorCancelPlacing", EActionTrigger.DOWN, HAM_CancelOnRightClick);
		input.AddActionListener("EditorPlace", EActionTrigger.DOWN, HAM_PlaceOnLeftClick);
		m_bTRNListener = true;
	}

	protected void HAM_PlaceOnLeftClick(float value, EActionTrigger reason)
	{
		if (!HAM_IsOurPrefab())
			return;

		HAM_VehicleSpawnUI.ConfirmEditorPlace();
	}

	override void HandlerDeattached(Widget w)
	{
		if (m_bTRNListener)
		{
			InputManager input = GetGame().GetInputManager();
			if (input)
			{
				input.RemoveActionListener("EditorCancelPlacing", EActionTrigger.DOWN, HAM_CancelOnRightClick);
				input.RemoveActionListener("EditorPlace", EActionTrigger.DOWN, HAM_PlaceOnLeftClick);
			}

			m_bTRNListener = false;
		}

		super.HandlerDeattached(w);
	}
}
