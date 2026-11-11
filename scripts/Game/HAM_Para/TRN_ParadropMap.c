//------------------------------------------------------------------------------------------------
// Barre sur la carte vanilla pendant le choix du point de saut.
//------------------------------------------------------------------------------------------------

void HAM_ParadropOnMapSelect(float value, EActionTrigger reason)
{
	if (!HAM_ParadropPick.IsPicking())
		return;

	SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
	if (!mapEntity)
		return;

	if (HAM_ParadropPick.IsConfirmOpen())
		return;

	float worldX;
	float worldZ;
	mapEntity.GetMapCursorWorldPosition(worldX, worldZ);
	HAM_ParadropPick.Take(worldX, worldZ);
	HAM_ParadropPick.OpenConfirm();
}

class HAM_ParadropConfirmDialog : SCR_ConfigurableDialogUi
{
	void HAM_ParadropConfirmDialog()
	{
		SCR_ConfigurableDialogUi.CreateFromPreset(SCR_CommonDialogs.DIALOGS_CONFIG, "respawn_confirmation", this);
	}

	override void OnMenuOpen(SCR_ConfigurableDialogUiPreset preset)
	{
		SetTitle("Paradrop");
		SetMessage("Confirmer le déploiement ?\n" + HAM_ParadropPick.PointText());

		SCR_InputButtonComponent confirm = FindButton("confirm");
		if (confirm)
			confirm.SetLabel("Confirmer");

		SCR_InputButtonComponent cancel = FindButton("cancel");
		if (cancel)
			cancel.SetLabel("Annuler");
	}

	override void OnConfirm()
	{
		HAM_ParadropPick.CloseConfirm();
		HAM_ParadropPick.Confirm();
		super.OnConfirm();
	}

	override void OnCancel()
	{
		HAM_ParadropPick.Cancel();
		super.OnCancel();
		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
	}
}

class HAM_ParadropMapHandler : ScriptedWidgetEventHandler
{
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return Press(w);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return Press(w);
	}

	protected bool Press(Widget w)
	{
		if (!w)
			return false;

		string name = w.GetName();
		if (name == "TakePoint")
		{
			HAM_ParadropOnMapSelect(0, EActionTrigger.DOWN);
			return true;
		}

		if (name == "ConfirmDrop")
		{
			if (!HAM_ParadropPick.HasPoint())
			{
				SCR_HintManagerComponent.ShowCustomHint("Clique d'abord sur la carte.", "13RDPA Training", 5);
				return true;
			}

			HAM_ParadropPick.Confirm();
			return true;
		}

		if (name == "CancelDrop")
		{
			HAM_ParadropPick.Cancel();
			MenuManager menus = GetGame().GetMenuManager();
			if (menus)
				menus.CloseMenuByPreset(ChimeraMenuPreset.MapMenu);

			return true;
		}

		return false;
	}
}

modded class SCR_MapMenuUI
{
	protected Widget m_wTRNParadropBar;
	protected ref HAM_ParadropMapHandler m_TRNParadropHandler;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		if (!HAM_ParadropPick.IsPicking())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !GetRootWidget())
			return;

		GetGame().GetInputManager().AddActionListener("MapSelect", EActionTrigger.UP, HAM_ParadropOnMapSelect);
		m_wTRNParadropBar = workspace.CreateWidgets("{6AA33470EE000010}UI/layouts/Menu/HAM_ParadropMapBar.layout", GetRootWidget());
		if (!m_wTRNParadropBar)
			return;

		m_wTRNParadropBar.SetZOrder(200);
		m_TRNParadropHandler = new HAM_ParadropMapHandler();
		Widget take = m_wTRNParadropBar.FindAnyWidget("TakePoint");
		Widget confirm = m_wTRNParadropBar.FindAnyWidget("ConfirmDrop");
		Widget cancel = m_wTRNParadropBar.FindAnyWidget("CancelDrop");
		if (take)
			take.AddHandler(m_TRNParadropHandler);
		if (confirm)
			confirm.AddHandler(m_TRNParadropHandler);
		if (cancel)
			cancel.AddHandler(m_TRNParadropHandler);

		TextWidget label = TextWidget.Cast(m_wTRNParadropBar.FindAnyWidget("PointText"));
		if (label)
			label.SetText(HAM_ParadropPick.PointText());
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (HAM_ParadropPick.IsPicking())
		{
			HAM_ParadropAudio.Tick();
			if (m_wTRNParadropBar)
			{
				TextWidget label = TextWidget.Cast(m_wTRNParadropBar.FindAnyWidget("PointText"));
				if (label)
					label.SetText(HAM_ParadropPick.PointText());
			}
		}
	}

	override void OnMenuClose()
	{
		if (m_wTRNParadropBar)
		{
			m_wTRNParadropBar.RemoveFromHierarchy();
			m_wTRNParadropBar = null;
		}

		GetGame().GetInputManager().RemoveActionListener("MapSelect", EActionTrigger.UP, HAM_ParadropOnMapSelect);
		if (HAM_ParadropPick.IsPicking() && !HAM_ParadropPick.IsConfirmOpen())
			HAM_ParadropPick.Cancel();

		super.OnMenuClose();
	}
}
