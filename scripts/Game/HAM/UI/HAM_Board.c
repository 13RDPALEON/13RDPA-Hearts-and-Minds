//------------------------------------------------------------------------------------------------
// Tableau H&M, ouvert depuis le tableau de tri Training posé dans une base tenue ou au QG :
// échange de ressources, transfert vers une base alliée, convoi d'hommes, missions secondaires,
// et « Définir comme QG » pour l'admin. Le serveur valide tout.
//------------------------------------------------------------------------------------------------

enum HAM_EBoardOp
{
	EXCHANGE = 1,
	TRANSFER = 2,
	DESIGNATE = 3,
	MISSION = 4,
	CANCEL_MISSION = 5,
	UNLOAD = 6
}

//------------------------------------------------------------------------------------------------
class HAM_BoardServer
{
	static const float BOARD_BASE_RADIUS = 300;
	static const float USER_RANGE = 15;

	//------------------------------------------------------------------------------------------------
	static string BaseOfBoard(IEntity board)
	{
		if (!board)
			return string.Empty;

		return HAM_Mission.NearestAlliedBase(board.GetOrigin(), BOARD_BASE_RADIUS);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool UserNear(int playerId, IEntity board)
	{
		IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!body || !board)
			return false;

		return vector.Distance(body.GetOrigin(), board.GetOrigin()) <= USER_RANGE;
	}

	//------------------------------------------------------------------------------------------------
	static void SendBoard(int playerId, RplId boardId)
	{
		SCR_PlayerController controller = HAM_Net.ControllerOf(playerId);
		if (!controller)
			return;

		array<int> ints = {};
		array<float> floats = {};
		array<string> bases = {};
		array<string> targets = {};
		array<string> missions = {};
		IEntity board = HAM_Util.EntityOf(boardId);
		if (!HAM_Mission.IsRunning() || !board)
		{
			controller.HAM_SendBoard(string.Empty, ints, floats, bases, targets, missions);
			return;
		}

		string baseName = BaseOfBoard(board);
		if (baseName.IsEmpty())
		{
			controller.HAM_SendBoard(string.Empty, ints, floats, bases, targets, missions);
			return;
		}

		HAM_Settings settings = HAM_Settings.Get();
		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		int supplies = -1;
		SCR_CampaignMilitaryBaseComponent conflictBase = HAM_HqDesignation.ConflictBaseOf(baseName);
		if (conflictBase)
			supplies = Math.Round(conflictBase.GetSupplies());

		int canDesignate = 0;
		if (HAM_HqDesignation.CanDesignate(playerId))
			canDesignate = 1;

		int designated = 0;
		if (HAM_Mission.s_sDesignatedHq == baseName)
			designated = 1;

		int graceLeft = -1;
		HAM_Cell cell = HAM_Mission.FindCell(baseName);
		if (cell && cell.m_fGraceLeft >= 0)
			graceLeft = Math.Round(cell.m_fGraceLeft);

		ints.Insert(canDesignate);
		ints.Insert(designated);
		ints.Insert(stock.m_iMen);
		ints.Insert(supplies);
		ints.Insert(settings.m_iMenThreshold);
		ints.Insert(graceLeft);

		floats.Insert(stock.m_fWater);
		floats.Insert(stock.m_fFood);
		floats.Insert(stock.m_fAmmo);
		floats.Insert(settings.m_fWaterThreshold);
		floats.Insert(settings.m_fFoodThreshold);
		floats.Insert(settings.m_fAmmoThreshold);
		floats.Insert(settings.m_fExchangeRatio);
		floats.Insert(settings.m_fExchangeStep);

		array<string> allied = {};
		HAM_Mission.AlliedBases(allied);
		foreach (string other : allied)
		{
			if (other != baseName)
				bases.Insert(other);
		}

		HAM_SideMissions.Targets(targets);
		HAM_SideMissions.StatusLines(missions);
		controller.HAM_SendBoard(baseName, ints, floats, bases, targets, missions);
	}

	//------------------------------------------------------------------------------------------------
	static void DoAction(int playerId, RplId boardId, int operation, string target, int kindA, int kindB, float amount)
	{
		IEntity board = HAM_Util.EntityOf(boardId);
		if (!HAM_Mission.IsRunning() || !board)
			return;

		if (!UserNear(playerId, board))
		{
			HAM_Net.NotifyPlayer(playerId, "Tableau", "Approche-toi du tableau.", 5);
			return;
		}

		string baseName = BaseOfBoard(board);
		if (baseName.IsEmpty())
		{
			HAM_Net.NotifyPlayer(playerId, "Tableau", "Ce tableau n'est pas dans une base tenue.", 6);
			return;
		}

		switch (operation)
		{
			case HAM_EBoardOp.EXCHANGE:
				HAM_Logistics.Exchange(baseName, kindA, kindB, amount, playerId);
				break;
			case HAM_EBoardOp.TRANSFER:
				HAM_Logistics.Transfer(baseName, target, kindA, amount, playerId);
				break;
			case HAM_EBoardOp.DESIGNATE:
				HAM_HqDesignation.Designate(baseName, playerId);
				break;
			case HAM_EBoardOp.MISSION:
				HAM_SideMissions.Launch(kindA, baseName, target, playerId);
				break;
			case HAM_EBoardOp.CANCEL_MISSION:
				HAM_SideMissions.Cancel(kindA, playerId);
				break;
			case HAM_EBoardOp.UNLOAD:
				HAM_Logistics.Unload(baseName, playerId, board.GetOrigin());
				break;
		}

		SendBoard(playerId, boardId);
	}
}

//------------------------------------------------------------------------------------------------
modded enum ChimeraMenuPreset
{
	HAM_ResourceBoard
}

//------------------------------------------------------------------------------------------------
enum HAM_EBoardPage
{
	MAIN,
	EXCHANGE_FROM,
	EXCHANGE_TO,
	EXCHANGE_AMOUNT,
	TRANSFER_KIND,
	TRANSFER_TARGET,
	TRANSFER_AMOUNT,
	CONVOY_TARGET,
	CONVOY_AMOUNT,
	MISSIONS,
	MISSION_TARGET,
	DESIGNATE_CONFIRM
}

//------------------------------------------------------------------------------------------------
class HAM_ResourceBoardMenu : ChimeraMenuBase
{
	protected static IEntity s_Board;
	protected static HAM_EBoardPage s_eStartPage;
	protected static HAM_ResourceBoardHandler s_Live;
	protected ref HAM_ResourceBoardHandler m_Handler;

	//------------------------------------------------------------------------------------------------
	static void Open(IEntity board, HAM_EBoardPage startPage = HAM_EBoardPage.MAIN)
	{
		if (!board)
			return;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus)
			return;

		s_Board = board;
		s_eStartPage = startPage;
		menus.OpenMenu(ChimeraMenuPreset.HAM_ResourceBoard);
	}

	//------------------------------------------------------------------------------------------------
	static void OnBoardData(string baseName, array<int> ints, array<float> floats, array<string> bases, array<string> targets, array<string> missions)
	{
		if (s_Live)
			s_Live.ApplyData(baseName, ints, floats, bases, targets, missions);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		if (!root)
			return;

		Widget fill = root.FindAnyWidget("PanelFill");
		if (fill)
			fill.SetFlags(WidgetFlags.IGNORE_CURSOR);

		m_Handler = new HAM_ResourceBoardHandler(this, s_Board, root, s_eStartPage);
		s_Live = m_Handler;
		root.AddHandler(m_Handler);
		m_Handler.Bind();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(root);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();
		s_Live = null;
		m_Handler = null;
		s_Board = null;
	}

	//------------------------------------------------------------------------------------------------
	static void NoteReputation()
	{
		if (!s_Live)
			return;

		s_Live.RefreshReputation();
	}
}

class HAM_ResourceBoardHandler : ScriptedWidgetEventHandler
{
	protected static const int PAGE_SIZE = 12;

	protected HAM_ResourceBoardMenu m_Menu;
	protected IEntity m_Board;
	protected Widget m_wRoot;
	protected float m_fNextClick;
	protected int m_iPage;
	protected HAM_EBoardPage m_ePage;

	protected string m_sBase;
	protected ref array<int> m_aInts = {};
	protected ref array<float> m_aFloats = {};
	protected ref array<string> m_aBases = {};
	protected ref array<string> m_aTargets = {};
	protected ref array<string> m_aMissions = {};

	protected HAM_EResource m_eKindA;
	protected HAM_EResource m_eKindB;
	protected string m_sTarget;
	protected int m_iMissionKind;
	protected string m_sSearch;

	protected ref array<string> m_aRowLabels = {};
	protected ref array<string> m_aRowKeys = {};

	//------------------------------------------------------------------------------------------------
	void HAM_ResourceBoardHandler(HAM_ResourceBoardMenu menu, IEntity board, Widget root, HAM_EBoardPage startPage)
	{
		m_Menu = menu;
		m_Board = board;
		m_wRoot = root;
		m_ePage = startPage;
	}

	//------------------------------------------------------------------------------------------------
	void Bind()
	{
		Place("Panel", 560, 80, 800, 880);
		Widget panel = m_wRoot.FindAnyWidget("Panel");
		if (panel)
			panel.SetColor(new Color(0.027, 0.051, 0.094, 0.96));

		Place("Title", 28, 20, 740, 36);
		Place("Info", 28, 58, 740, 60);
		Widget searchBg = m_wRoot.FindAnyWidget("SearchBg");
		if (searchBg)
			searchBg.SetColor(new Color(0.16, 0.22, 0.34, 1));

		Widget searchBox = m_wRoot.FindAnyWidget("Search");
		if (searchBox)
		{
			searchBox.SetColor(new Color(0.95, 0.96, 0.97, 1));
			searchBox.AddHandler(this);
		}

		BindButton("SearchBtn");
		for (int row = 0; row < PAGE_SIZE; row++)
		{
			BindButton("Player" + row.ToString());
		}

		Place("Prev", 28, 748, 360, 40);
		Place("Next", 412, 748, 360, 40);
		Place("Close", 28, 804, 744, 52);
		Widget closeButton = m_wRoot.FindAnyWidget("Close");
		if (closeButton)
			closeButton.SetColor(new Color(0.55, 0.16, 0.16, 1));

		BindButton("Prev");
		BindButton("Next");
		BindButton("Close");
		SetText("PrevText", "Retour");
		SetText("NextText", "Page suivante");
		SetText("Title", "Tableau Hearts and Minds");
		SetText("Info", "Chargement…");
		Paint();
		Ask();
	}

	//------------------------------------------------------------------------------------------------
	protected void Ask()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplId boardId = HAM_Util.IdOf(m_Board);
		if (controller && boardId.IsValid())
			controller.HAM_AskBoard(boardId);
	}

	//------------------------------------------------------------------------------------------------
	void ApplyData(string baseName, array<int> ints, array<float> floats, array<string> bases, array<string> targets, array<string> missions)
	{
		m_sBase = baseName;
		Copy(ints, m_aInts);
		CopyF(floats, m_aFloats);
		CopyS(bases, m_aBases);
		CopyS(targets, m_aTargets);
		CopyS(missions, m_aMissions);
		Paint();
	}

	//------------------------------------------------------------------------------------------------
	protected void Copy(array<int> source, array<int> target)
	{
		target.Clear();
		if (!source)
			return;

		foreach (int value : source)
		{
			target.Insert(value);
		}
	}

	protected void CopyF(array<float> source, array<float> target)
	{
		target.Clear();
		if (!source)
			return;

		foreach (float value : source)
		{
			target.Insert(value);
		}
	}

	protected void CopyS(array<string> source, array<string> target)
	{
		target.Clear();
		if (!source)
			return;

		foreach (string value : source)
		{
			target.Insert(value);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int IntAt(int index)
	{
		if (index < m_aInts.Count())
			return m_aInts[index];

		return 0;
	}

	protected float FloatAt(int index)
	{
		if (index < m_aFloats.Count())
			return m_aFloats[index];

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected string StockLine()
	{
		string line = string.Format("Eau %1/%2 · Nourriture %3/%4 · Munitions %5/%6 · Hommes %7/%8", HAM_Util.FormatInt(FloatAt(0)), HAM_Util.FormatInt(FloatAt(3)), HAM_Util.FormatInt(FloatAt(1)), HAM_Util.FormatInt(FloatAt(4)), HAM_Util.FormatInt(FloatAt(2)), HAM_Util.FormatInt(FloatAt(5)), IntAt(2), IntAt(4));
		if (IntAt(3) >= 0)
			line = line + string.Format(" · Fournitures %1", IntAt(3));

		if (IntAt(5) >= 0)
			line = line + string.Format("\nDélai de grâce : %1 min", IntAt(5) / 60);

		return line;
	}

	//------------------------------------------------------------------------------------------------
	protected string InfoLine()
	{
		float reputation = HAM_ClientState.s_fGlobalRep;
		if (Replication.IsServer())
			reputation = HAM_Mission.s_fGlobalRep;

		return StockLine() + "\nRéputation " + HAM_Mission.FormatRep(reputation);
	}

	//------------------------------------------------------------------------------------------------
	void RefreshReputation()
	{
		if (m_sBase.IsEmpty())
			return;

		SetText("Info", InfoLine());
	}

	//------------------------------------------------------------------------------------------------
	protected void Row(string key, string label)
	{
		m_aRowKeys.Insert(key);
		m_aRowLabels.Insert(label);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildRows()
	{
		m_aRowKeys.Clear();
		m_aRowLabels.Clear();
		if (m_sBase.IsEmpty())
			return;

		float step = FloatAt(7);
		if (step < 1)
			step = 50;

		int men = IntAt(2);
		array<int> sizes = {2, 4, 6, 10, 20};

		if (m_ePage == HAM_EBoardPage.MAIN)
		{
			Row("exchange", "Échanger une ressource");
			Row("transfer", "Transférer vers une base alliée");
			Row("convoy", "Envoyer un convoi d'hommes");
			Row("missions", "Missions secondaires");
			if (IntAt(0) == 1 && IntAt(1) == 0)
				Row("designate", "Définir cette base comme QG");

			Row("refresh", "Actualiser");
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_FROM)
		{
			Row("res0", "Donner de l'eau");
			Row("res1", "Donner de la nourriture");
			Row("res2", "Donner des munitions");
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_TO)
		{
			for (int kind = 0; kind < 3; kind++)
			{
				if (kind != m_eKindA)
					Row("res" + kind.ToString(), "Recevoir " + HAM_Stock.Label(kind));
			}
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_AMOUNT || m_ePage == HAM_EBoardPage.TRANSFER_AMOUNT)
		{
			float factor = 1;
			if (m_ePage == HAM_EBoardPage.TRANSFER_AMOUNT && m_eKindA == HAM_EResource.SUPPLIES)
				factor = 5;

			Row("amt" + HAM_Util.FormatInt(step * factor), HAM_Util.FormatInt(step * factor));
			Row("amt" + HAM_Util.FormatInt(step * 2 * factor), HAM_Util.FormatInt(step * 2 * factor));
			Row("amt" + HAM_Util.FormatInt(step * 5 * factor), HAM_Util.FormatInt(step * 5 * factor));
		}
		else if (m_ePage == HAM_EBoardPage.TRANSFER_KIND)
		{
			Row("res0", "Eau");
			Row("res1", "Nourriture");
			Row("res2", "Munitions");
			if (IntAt(3) >= 0)
				Row("res4", "Fournitures (entre bases Conflit)");
		}
		else if (m_ePage == HAM_EBoardPage.TRANSFER_TARGET || m_ePage == HAM_EBoardPage.CONVOY_TARGET)
		{
			foreach (string baseName : m_aBases)
			{
				Row("base:" + baseName, baseName);
			}
		}
		else if (m_ePage == HAM_EBoardPage.CONVOY_AMOUNT)
		{
			foreach (int size : sizes)
			{
				if (size <= men)
					Row("amt" + size.ToString(), size.ToString() + " hommes");
			}
		}
		else if (m_ePage == HAM_EBoardPage.MISSIONS)
		{
			foreach (int missionIdx, string status : m_aMissions)
			{
				Row("mission" + missionIdx.ToString(), status);
			}
		}
		else if (m_ePage == HAM_EBoardPage.MISSION_TARGET)
		{
			string query = m_sSearch;
			query.ToLower();
			if (query.IsEmpty())
				Row("target:", "Zone au hasard sur la ligne de front");

			foreach (string target : m_aTargets)
			{
				int split = target.IndexOf("|");
				string zone = target;
				string label = target;
				if (split > 0)
				{
					zone = target.Substring(0, split);
					label = target.Substring(split + 1, target.Length() - split - 1);
				}

				string haystack = label;
				haystack.ToLower();
				if (!query.IsEmpty() && !haystack.Contains(query))
					continue;

				Row("target:" + zone, label);
			}

			if (m_aTargets.IsEmpty())
				Row("none", "Aucune zone ennemie sur la ligne de front");
			else if (m_aRowKeys.IsEmpty())
				Row("none", "Aucune zone ne correspond à « " + m_sSearch + " »");
		}
		else if (m_ePage == HAM_EBoardPage.DESIGNATE_CONFIRM)
		{
			Row("confirm", "Confirmer : " + m_sBase + " devient le QG");
			Row("cancel", "Annuler");
		}
	}

	//------------------------------------------------------------------------------------------------
	//! La page de cible de mission a sa barre de recherche au-dessus de la liste.
	protected void Layout()
	{
		bool search = m_ePage == HAM_EBoardPage.MISSION_TARGET && !m_sBase.IsEmpty();
		float top = 130;
		float step = 50;
		float height = 44;
		if (search)
		{
			Place("SearchBg", 28, 126, 600, 40);
			Place("Search", 28, 126, 600, 40);
			Place("SearchBtn", 640, 126, 132, 40);
			top = 176;
			step = 47;
			height = 43;
		}
		else
		{
			Hide("SearchBg");
			Hide("Search");
			Hide("SearchBtn");
		}

		for (int row = 0; row < PAGE_SIZE; row++)
		{
			Place("Player" + row.ToString(), 28, top + row * step, 744, height);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Paint()
	{
		Layout();
		BuildRows();
		if (m_sBase.IsEmpty())
		{
			SetText("Info", "Ce tableau n'est pas dans une base tenue (maître tenu ou QG), ou le front n'est pas actif.");
		}
		else
		{
			string header = m_sBase;
			if (IntAt(1) == 1)
				header = header + " (QG)";

			SetText("Title", "Tableau H&M — " + header);
			SetText("Info", InfoLine());
		}

		for (int row = 0; row < PAGE_SIZE; row++)
		{
			int index = m_iPage * PAGE_SIZE + row;
			Widget button = m_wRoot.FindAnyWidget("Player" + row.ToString());
			if (!button)
				continue;

			bool show = index < m_aRowLabels.Count();
			button.SetVisible(show);
			button.SetEnabled(show);
			button.SetColor(new Color(0.05, 0.08, 0.14, 1));
			string label = "—";
			if (show)
				label = m_aRowLabels[index];

			SetText("Player" + row.ToString() + "Text", label);
		}
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return HandlePress(w);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		return HandlePress(w);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box || box.GetName() != "Search")
			return false;

		m_sSearch = box.GetText();
		m_iPage = 0;
		Paint();
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetSearch()
	{
		m_sSearch = string.Empty;
		EditBoxWidget box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Search"));
		if (box)
			box.SetText(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HandlePress(Widget w)
	{
		string name = ButtonName(w);
		if (name.IsEmpty())
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextClick)
			return true;

		m_fNextClick = now + 350;
		if (name == "Close")
		{
			if (m_Menu)
				m_Menu.Close();

			return true;
		}

		if (name == "SearchBtn")
		{
			EditBoxWidget box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Search"));
			if (box)
				m_sSearch = box.GetText();

			m_iPage = 0;
			Paint();
			return true;
		}

		if (name == "Prev")
		{
			m_ePage = HAM_EBoardPage.MAIN;
			m_iPage = 0;
			ResetSearch();
			Paint();
			return true;
		}

		if (name == "Next")
		{
			if ((m_iPage + 1) * PAGE_SIZE < m_aRowKeys.Count())
				m_iPage++;
			else
				m_iPage = 0;

			Paint();
			return true;
		}

		int local = name.Substring(6, name.Length() - 6).ToInt();
		int index = m_iPage * PAGE_SIZE + local;
		if (index < 0 || index >= m_aRowKeys.Count())
			return true;

		Choose(m_aRowKeys[index]);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Choose(string key)
	{
		m_iPage = 0;
		if (key == "refresh")
		{
			Ask();
			return;
		}

		if (m_ePage == HAM_EBoardPage.MAIN)
		{
			if (key == "exchange")
				m_ePage = HAM_EBoardPage.EXCHANGE_FROM;
			else if (key == "transfer")
				m_ePage = HAM_EBoardPage.TRANSFER_KIND;
			else if (key == "convoy")
				m_ePage = HAM_EBoardPage.CONVOY_TARGET;
			else if (key == "missions")
				m_ePage = HAM_EBoardPage.MISSIONS;
			else if (key == "designate")
				m_ePage = HAM_EBoardPage.DESIGNATE_CONFIRM;
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_FROM)
		{
			m_eKindA = KindOf(key);
			m_ePage = HAM_EBoardPage.EXCHANGE_TO;
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_TO)
		{
			m_eKindB = KindOf(key);
			m_ePage = HAM_EBoardPage.EXCHANGE_AMOUNT;
		}
		else if (m_ePage == HAM_EBoardPage.EXCHANGE_AMOUNT)
		{
			Send(HAM_EBoardOp.EXCHANGE, string.Empty, m_eKindA, m_eKindB, AmountOf(key));
			m_ePage = HAM_EBoardPage.MAIN;
		}
		else if (m_ePage == HAM_EBoardPage.TRANSFER_KIND)
		{
			m_eKindA = KindOf(key);
			m_ePage = HAM_EBoardPage.TRANSFER_TARGET;
		}
		else if (m_ePage == HAM_EBoardPage.TRANSFER_TARGET)
		{
			m_sTarget = key.Substring(5, key.Length() - 5);
			m_ePage = HAM_EBoardPage.TRANSFER_AMOUNT;
		}
		else if (m_ePage == HAM_EBoardPage.TRANSFER_AMOUNT)
		{
			Send(HAM_EBoardOp.TRANSFER, m_sTarget, m_eKindA, 0, AmountOf(key));
			m_ePage = HAM_EBoardPage.MAIN;
		}
		else if (m_ePage == HAM_EBoardPage.CONVOY_TARGET)
		{
			m_sTarget = key.Substring(5, key.Length() - 5);
			m_ePage = HAM_EBoardPage.CONVOY_AMOUNT;
		}
		else if (m_ePage == HAM_EBoardPage.CONVOY_AMOUNT)
		{
			Send(HAM_EBoardOp.TRANSFER, m_sTarget, HAM_EResource.MEN, 0, AmountOf(key));
			m_ePage = HAM_EBoardPage.MAIN;
		}
		else if (m_ePage == HAM_EBoardPage.MISSIONS)
		{
			m_iMissionKind = key.Substring(7, key.Length() - 7).ToInt();
			bool running = m_iMissionKind < m_aMissions.Count() && m_aMissions[m_iMissionKind].Contains("en cours");
			if (running)
			{
				Send(HAM_EBoardOp.CANCEL_MISSION, string.Empty, m_iMissionKind, 0, 0);
				m_ePage = HAM_EBoardPage.MAIN;
			}
			else
			{
				ResetSearch();
				m_ePage = HAM_EBoardPage.MISSION_TARGET;
			}
		}
		else if (m_ePage == HAM_EBoardPage.MISSION_TARGET)
		{
			if (key == "none")
				return;

			Send(HAM_EBoardOp.MISSION, key.Substring(7, key.Length() - 7), m_iMissionKind, 0, 0);
			ResetSearch();
			m_ePage = HAM_EBoardPage.MAIN;
		}
		else if (m_ePage == HAM_EBoardPage.DESIGNATE_CONFIRM)
		{
			if (key == "confirm")
				Send(HAM_EBoardOp.DESIGNATE, string.Empty, 0, 0, 0);

			m_ePage = HAM_EBoardPage.MAIN;
		}

		Paint();
	}

	//------------------------------------------------------------------------------------------------
	protected HAM_EResource KindOf(string key)
	{
		return key.Substring(3, key.Length() - 3).ToInt();
	}

	//------------------------------------------------------------------------------------------------
	protected float AmountOf(string key)
	{
		return key.Substring(3, key.Length() - 3).ToFloat();
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(HAM_EBoardOp operation, string target, int kindA, int kindB, float amount)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplId boardId = HAM_Util.IdOf(m_Board);
		if (controller && boardId.IsValid())
			controller.HAM_AskBoardAction(boardId, operation, target, kindA, kindB, amount);
	}

	//------------------------------------------------------------------------------------------------
	protected string ButtonName(Widget w)
	{
		while (w)
		{
			string name = w.GetName();
			if (name == "Close" || name == "Prev" || name == "Next" || name == "SearchBtn")
				return name;

			if (name.IndexOf("Player") == 0 && !name.Contains("Text"))
				return name;

			w = w.GetParent();
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void Place(string name, float x, float y, float width, float height)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (!widget)
			return;

		FrameSlot.SetAnchorMin(widget, 0, 0);
		FrameSlot.SetAnchorMax(widget, 0, 0);
		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
		widget.SetVisible(true);
		widget.SetOpacity(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void Hide(string name)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (widget)
			widget.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetText(string name, string text)
	{
		TextWidget widget = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (widget)
			widget.SetText(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void BindButton(string name)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (!widget)
			return;

		widget.AddHandler(this);
		Widget child = widget.GetChildren();
		while (child)
		{
			child.SetFlags(WidgetFlags.IGNORE_CURSOR);
			child = child.GetSibling();
		}
	}
}

//------------------------------------------------------------------------------------------------
//! Action locale du tableau Training : ouvre le tableau H&M.
class HAM_ResourceBoardUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user != null && HAM_Mission.IsActive();
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Missions et échanges";
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ResourceBoardMenu.Open(pOwnerEntity);
	}
}

//------------------------------------------------------------------------------------------------
//! Poste de missions constructible : ouvre le tableau H&M directement sur les missions secondaires.
class HAM_MissionPostUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user != null && HAM_Mission.IsActive();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ResourceBoardMenu.Open(pOwnerEntity, HAM_EBoardPage.MISSIONS);
	}
}
