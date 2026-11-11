//------------------------------------------------------------------------------------------------
// Dialogue d'un civil. La liste se parcourt avec les touches du menu (haut / bas, manette),
// se valide avec la touche de confirmation du jeu, et se ferme avec retour.
//------------------------------------------------------------------------------------------------

class HAM_CivilianDialog : SCR_ConfigurableDialogUi
{
	protected bool m_bAnswered;
	protected bool m_bPay;
	protected bool m_bListening;
	protected int m_iChoice;

	override protected void OnMenuOpen(SCR_ConfigurableDialogUiPreset preset)
	{
		SetTitle("Civil");
		m_OnClose.Insert(OnDialogClosed);
		Listen(true);
		if (HAM_CivilianTalkUI.PayOnly())
		{
			m_bAnswered = true;
			m_bPay = true;
			SetMessage("« Je n'ai rien vu, mais je peux vous aider si vous payez. Cent ressources. »");
			SetConfirm("Payer 100 ressources", true);
		}
		else
		{
			m_iChoice = 0;
			RefreshChoice();
			SetConfirm("Choisir", true);
		}
	}

	override void OnMenuClose()
	{
		Listen(false);
	}

	override protected void OnConfirm()
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		if (m_bPay)
		{
			player.HAM_CivilianPay(HAM_CivilianTalkUI.Civilian());
			return;
		}

		if (m_bAnswered)
		{
			Close();
			return;
		}

		m_bAnswered = true;
		SetConfirm(string.Empty, false);
		player.HAM_CivilianAsk(HAM_CivilianTalkUI.Civilian(), m_iChoice + 1);
	}

	void ShowAnswer(string text, bool canPay, string confirm)
	{
		m_bAnswered = true;
		m_bPay = canPay;
		SetMessage(text);
		if (canPay)
		{
			if (confirm.IsEmpty())
				confirm = "Payer 100 ressources";

			SetConfirm(confirm, true);
		}
		else if (!confirm.IsEmpty())
		{
			SetConfirm(confirm, true);
		}
		else
		{
			SetConfirm(string.Empty, false);
		}
	}

	protected void OnDialogClosed(SCR_ConfigurableDialogUi dialog)
	{
		Listen(false);
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_CivilianTalkClosed(HAM_CivilianTalkUI.Civilian());

		HAM_CivilianTalkUI.Forget(this);
	}

	protected void OnUp(float value, EActionTrigger reason)
	{
		MoveChoice(-1);
	}

	protected void OnDown(float value, EActionTrigger reason)
	{
		MoveChoice(1);
	}

	protected void MoveChoice(int step)
	{
		if (m_bAnswered)
			return;

		m_iChoice = m_iChoice + step;
		if (m_iChoice < 0)
			m_iChoice = 2;

		if (m_iChoice > 2)
			m_iChoice = 0;

		RefreshChoice();
	}

	protected void RefreshChoice()
	{
		string first = "  Qu'avez-vous vu ?";
		string second = "  De quoi avez-vous besoin ?";
		string third = "  Comment pouvons-nous vous aider ?";
		if (m_iChoice == 0)
			first = "> Qu'avez-vous vu ?";
		else if (m_iChoice == 1)
			second = "> De quoi avez-vous besoin ?";
		else
			third = "> Comment pouvons-nous vous aider ?";

		SetMessage("Le civil vous regarde. Une seule réponse.\n\n" + first + "\n" + second + "\n" + third);
	}

	protected void SetConfirm(string label, bool visible)
	{
		SCR_InputButtonComponent button = FindButton("confirm");
		if (!button)
			return;

		if (!label.IsEmpty())
			button.SetLabel(label);

		button.SetVisible(visible, false);
		button.SetEnabled(visible);
	}

	protected void Listen(bool enabled)
	{
		if (m_bListening == enabled)
			return;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		m_bListening = enabled;
		if (enabled)
		{
			input.AddActionListener("MenuUp", EActionTrigger.DOWN, OnUp);
			input.AddActionListener("MenuDown", EActionTrigger.DOWN, OnDown);
		}
		else
		{
			input.RemoveActionListener("MenuUp", EActionTrigger.DOWN, OnUp);
			input.RemoveActionListener("MenuDown", EActionTrigger.DOWN, OnDown);
		}
	}
}

class HAM_CivilianTalkUI
{
	protected static const ResourceName DIALOG_PRESET = "{E4C8A17B0BC59050}Configs/UI/HAM_CivilianDialogs.conf";
	protected static HAM_CivilianDialog s_Dialog;
	protected static RplId s_Civilian;
	protected static bool s_bPayOnly;

	static void Open(RplId civilianId, bool payOnly)
	{
		s_Civilian = civilianId;
		s_bPayOnly = payOnly;
		if (s_Dialog)
			s_Dialog.Close();

		HAM_CivilianDialog dialog = new HAM_CivilianDialog();
		s_Dialog = dialog;
		if (!SCR_ConfigurableDialogUi.CreateFromPreset(DIALOG_PRESET, "talk", dialog))
		{
			s_Dialog = null;
			Print("[13RDPA H&M] Dialogue du civil introuvable", LogLevel.ERROR);
		}
	}

	static void Forget(HAM_CivilianDialog dialog)
	{
		if (s_Dialog == dialog)
			s_Dialog = null;
	}

	static RplId Civilian()
	{
		return s_Civilian;
	}

	static bool PayOnly()
	{
		return s_bPayOnly;
	}

	static void ShowAnswer(string text, bool canPay, string confirm)
	{
		if (s_Dialog)
			s_Dialog.ShowAnswer(text, canPay, confirm);
	}
}
