//------------------------------------------------------------------------------------------------
// Panneau téléport : joueurs OU bases capturées (même layout).
//------------------------------------------------------------------------------------------------

enum HAM_ETeleportBoardMode
{
	PLAYERS,
	BASES
}

modded enum ChimeraMenuPreset
{
	HAM_PlayerTeleport
}

class HAM_PlayerTeleportMenu : ChimeraMenuBase
{
	protected static IEntity s_Board;
	protected static HAM_ETeleportBoardMode s_Mode;
	protected ref HAM_PlayerTeleportBoardHandler m_Handler;

	static void SetBoard(IEntity board, HAM_ETeleportBoardMode mode)
	{
		s_Board = board;
		s_Mode = mode;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		Widget root = GetRootWidget();
		if (!root)
			return;

		Widget fill = root.FindAnyWidget("PanelFill");
		if (fill)
			fill.SetFlags(WidgetFlags.IGNORE_CURSOR);

		m_Handler = new HAM_PlayerTeleportBoardHandler(s_Board, root, s_Mode);
		root.AddHandler(m_Handler);
		m_Handler.Bind();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(root);
	}

	override void OnMenuClose()
	{
		super.OnMenuClose();
		m_Handler = null;
		s_Board = null;
		HAM_PlayerTeleportBoard.ForgetMenu();
	}
}

class HAM_PlayerTeleportBoard
{
	protected static MenuBase s_Menu;
	protected static Widget s_wRoot;
	protected static ref HAM_PlayerTeleportBoardHandler s_Handler;
	protected static HAM_PlayerTeleportBoardHandler s_Live;
	protected static HAM_ETeleportBoardMode s_Mode;

	static void SetHandler(HAM_PlayerTeleportBoardHandler handler)
	{
		s_Live = handler;
	}

	static void ApplyServerList(string idsPacked, string namesPacked)
	{
		if (s_Live)
			s_Live.ApplyServerList(idsPacked, namesPacked);
	}

	static void Toggle(IEntity board, HAM_ETeleportBoardMode mode)
	{
		if (s_Menu || s_wRoot)
		{
			Close();
			return;
		}

		Open(board, mode);
	}

	static void Open(IEntity board, HAM_ETeleportBoardMode mode)
	{
		if (!board)
			return;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus)
			return;

		s_Mode = mode;
		HAM_PlayerTeleportMenu.SetBoard(board, mode);
		s_Menu = menus.OpenMenu(ChimeraMenuPreset.HAM_PlayerTeleport);
		HAM_PlayerTeleportMenu ours = HAM_PlayerTeleportMenu.Cast(s_Menu);
		if (ours && ours.GetRootWidget() && ours.GetRootWidget().FindAnyWidget("Panel"))
			return;

		Widget host;
		if (ours)
			host = ours.GetRootWidget();
		else
			s_Menu = null;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		s_wRoot = workspace.CreateWidgets("{B5E9F6D69B661964}UI/layouts/Menu/HAM_PlayerTeleport.layout", host);
		if (!s_wRoot)
		{
			Print("[13RDPA H&M Teleport] layout introuvable", LogLevel.ERROR);
			return;
		}

		s_Handler = new HAM_PlayerTeleportBoardHandler(board, s_wRoot, mode);
		s_wRoot.AddHandler(s_Handler);
		s_Handler.Bind();
	}

	static void Close()
	{
		if (s_wRoot)
			s_wRoot.RemoveFromHierarchy();

		MenuBase menu = s_Menu;
		s_wRoot = null;
		s_Menu = null;
		s_Handler = null;
		s_Live = null;
		if (menu)
			menu.Close();
	}

	static void ForgetMenu()
	{
		if (!s_Menu)
			return;

		s_wRoot = null;
		s_Menu = null;
		s_Handler = null;
		s_Live = null;
	}
}

class HAM_PlayerTeleportBoardHandler : ScriptedWidgetEventHandler
{
	protected static const int PAGE_SIZE = 12;

	protected IEntity m_Board;
	protected Widget m_wRoot;
	protected HAM_ETeleportBoardMode m_eMode;
	protected float m_fNextClick;
	protected int m_iPage;
	protected string m_sSearch;
	protected ref array<int> m_aAllIds;
	protected ref array<string> m_aAllNames;
	protected ref array<int> m_aPlayerIds;
	protected ref array<string> m_aPlayerNames;

	void HAM_PlayerTeleportBoardHandler(IEntity board, Widget root, HAM_ETeleportBoardMode mode)
	{
		m_Board = board;
		m_wRoot = root;
		m_eMode = mode;
		m_sSearch = "";
		m_aAllIds = {};
		m_aAllNames = {};
		m_aPlayerIds = {};
		m_aPlayerNames = {};
	}

	void Bind()
	{
		HAM_PlayerTeleportBoard.SetHandler(this);
		HAM_Place(m_wRoot, "Panel", 560, 80, 800, 880);
		Widget panel = m_wRoot.FindAnyWidget("Panel");
		if (panel)
			panel.SetColor(new Color(0.027, 0.051, 0.094, 0.96));

		HAM_Place(m_wRoot, "Title", 28, 20, 740, 36);
		HAM_Place(m_wRoot, "Info", 28, 58, 740, 22);
		HAM_Place(m_wRoot, "SearchBg", 28, 96, 600, 40);
		HAM_Place(m_wRoot, "Search", 28, 96, 600, 40);
		HAM_Place(m_wRoot, "SearchBtn", 640, 96, 132, 40);
		Widget searchBg = m_wRoot.FindAnyWidget("SearchBg");
		if (searchBg)
			searchBg.SetColor(new Color(0.16, 0.22, 0.34, 1));
		Widget searchBox = m_wRoot.FindAnyWidget("Search");
		if (searchBox)
		{
			searchBox.SetColor(new Color(0.95, 0.96, 0.97, 1));
			searchBox.AddHandler(this);
		}

		TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		if (title)
		{
			if (m_eMode == HAM_ETeleportBoardMode.BASES)
				title.SetText("Téléportation bases");
			else
				title.SetText("Téléportation joueurs");
		}

		for (int row = 0; row < PAGE_SIZE; row++)
		{
			HAM_Place(m_wRoot, "Player" + row.ToString(), 28, 160 + row * 48, 744, 44);
			BindButton("Player" + row.ToString());
		}

		HAM_Place(m_wRoot, "Prev", 28, 748, 180, 40);
		HAM_Place(m_wRoot, "Next", 220, 748, 180, 40);
		HAM_Place(m_wRoot, "Close", 28, 804, 744, 52);
		Widget closeButton = m_wRoot.FindAnyWidget("Close");
		if (closeButton)
			closeButton.SetColor(new Color(0.55, 0.16, 0.16, 1));
		BindButton("Prev");
		BindButton("Next");
		BindButton("Close");
		BindButton("SearchBtn");

		TextWidget info = TextWidget.Cast(m_wRoot.FindAnyWidget("Info"));
		if (info)
		{
			if (m_eMode == HAM_ETeleportBoardMode.BASES)
				info.SetText("Chargement des bases…");
			else
				info.SetText("Chargement des joueurs…");
		}

		Paint();
		AskServerList();
	}

	void ApplyServerList(string idsPacked, string namesPacked)
	{
		m_aAllIds.Clear();
		m_aAllNames.Clear();
		m_iPage = 0;

		if (!idsPacked.IsEmpty())
		{
			array<string> idParts = {};
			array<string> nameParts = {};
			idsPacked.Split(";", idParts, true);
			namesPacked.Split("|", nameParts, true);

			int count = idParts.Count();
			if (nameParts.Count() < count)
				count = nameParts.Count();

			for (int i = 0; i < count; i++)
			{
				int id = idParts[i].ToInt();
				// Joueurs : id >= 1. Bases : index peut être 0.
				if (m_eMode != HAM_ETeleportBoardMode.BASES && id < 1)
					continue;

				string name = nameParts[i];
				if (name.IsEmpty())
				{
					if (m_eMode == HAM_ETeleportBoardMode.BASES)
						name = "Base " + id.ToString();
					else
						name = "Joueur " + id.ToString();
				}

				m_aAllIds.Insert(id);
				m_aAllNames.Insert(name);
			}
		}

		ApplySearchFilter();
	}

	protected void AskServerList()
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		if (m_eMode == HAM_ETeleportBoardMode.BASES)
			player.HAM_RequestBaseTeleportList();
		else
			player.HAM_RequestTeleportList();
	}

	protected void HAM_Place(Widget root, string name, float x, float y, float width, float height)
	{
		if (!root)
			return;

		Widget widget = root.FindAnyWidget(name);
		if (!widget)
			return;

		FrameSlot.SetAnchorMin(widget, 0, 0);
		FrameSlot.SetAnchorMax(widget, 0, 0);
		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
		widget.SetVisible(true);
		widget.SetOpacity(1);
	}

	protected void BindButton(string name)
	{
		if (!m_wRoot)
			return;

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

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return HandlePress(w);
	}

	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box || box.GetName() != "Search")
			return false;

		m_sSearch = box.GetText();
		m_iPage = 0;
		ApplySearchFilter();
		Paint();
		return false;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		return HandlePress(w);
	}

	protected bool HandlePress(Widget w)
	{
		if (!w)
			return false;

		string name = ButtonName(w);
		if (name.IsEmpty())
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextClick)
			return true;

		m_fNextClick = now + 350;

		if (name == "Close")
		{
			HAM_PlayerTeleportBoard.Close();
			return true;
		}

		if (name == "SearchBtn")
		{
			ReadSearch();
			ApplySearchFilter();
			return true;
		}

		if (name == "Prev")
		{
			if (m_iPage > 0)
				m_iPage = m_iPage - 1;
			Paint();
			return true;
		}

		if (name == "Next")
		{
			int last = 0;
			if (m_aPlayerIds.Count() > PAGE_SIZE)
				last = (m_aPlayerIds.Count() - 1) / PAGE_SIZE;

			if (m_iPage < last)
				m_iPage = m_iPage + 1;

			Paint();
			return true;
		}

		if (name.IndexOf("Player") != 0)
			return false;

		int local = name.Substring(6, name.Length() - 6).ToInt();
		int index = m_iPage * PAGE_SIZE + local;
		if (index < 0 || index >= m_aPlayerIds.Count())
			return true;

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return true;

		if (m_eMode == HAM_ETeleportBoardMode.BASES)
			player.HAM_RequestTeleportToBase(m_aPlayerIds[index]);
		else
			player.HAM_RequestTeleportToPlayer(m_aPlayerIds[index]);

		HAM_PlayerTeleportBoard.Close();
		return true;
	}

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

	protected void ReadSearch()
	{
		EditBoxWidget box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Search"));
		if (!box)
		{
			m_sSearch = "";
			return;
		}

		m_sSearch = box.GetText();
	}

	protected void ApplySearchFilter()
	{
		m_aPlayerIds.Clear();
		m_aPlayerNames.Clear();
		m_iPage = 0;

		string query = m_sSearch;
		query.ToLower();

		int count = m_aAllIds.Count();
		for (int i = 0; i < count; i++)
		{
			string name = m_aAllNames[i];
			if (!query.IsEmpty())
			{
				string label = name;
				label.ToLower();
				if (!label.Contains(query))
					continue;
			}

			m_aPlayerIds.Insert(m_aAllIds[i]);
			m_aPlayerNames.Insert(name);
		}

		Paint();
	}

	protected void Paint()
	{
		TextWidget info = TextWidget.Cast(m_wRoot.FindAnyWidget("Info"));
		if (info)
		{
			if (m_eMode == HAM_ETeleportBoardMode.BASES)
			{
				if (m_aAllIds.IsEmpty())
					info.SetText("Aucune autre base capturée.");
				else if (m_aPlayerIds.IsEmpty())
					info.SetText("Aucune base ne correspond à la recherche.");
				else
					info.SetText("Clique une base pour t'y téléporter.");
			}
			else if (m_aAllIds.IsEmpty())
			{
				info.SetText("Aucun autre joueur déployé.");
			}
			else if (m_aPlayerIds.IsEmpty())
			{
				info.SetText("Aucun joueur ne correspond à la recherche.");
			}
			else
			{
				info.SetText("Clique un nom pour te téléporter près de lui.");
			}
		}

		for (int row = 0; row < PAGE_SIZE; row++)
		{
			int index = m_iPage * PAGE_SIZE + row;
			Widget button = m_wRoot.FindAnyWidget("Player" + row.ToString());
			TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("Player" + row.ToString() + "Text"));
			if (!button)
				continue;

			bool show = index < m_aPlayerIds.Count();
			button.SetVisible(show);
			button.SetEnabled(show);
			button.SetColor(new Color(0.05, 0.08, 0.14, 1));
			if (label)
			{
				if (show)
					label.SetText(m_aPlayerNames[index]);
				else
					label.SetText("—");
			}
		}
	}
}
