//------------------------------------------------------------------------------------------------
// Flipchart arsenal : Tri (inclus/exclus) + Paramétrage (liste → détail).
//------------------------------------------------------------------------------------------------

enum HAM_EArsenalBoardMode
{
	FILTER,
	PARAMS,
	VEHICLE_FILTER,
	VEHICLE_PARAMS
}

modded enum ChimeraMenuPreset
{
	HAM_ArsenalBoard
}

class HAM_ArsenalFilterUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !user)
			return false;

		return HAM_ArsenalBoardStore.IsStaff(manager.GetPlayerIdFromControlledEntity(user));
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ArsenalBoardUI.Open(pOwnerEntity, HAM_EArsenalBoardMode.FILTER);
	}
}

class HAM_ArsenalParamUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !user)
			return false;

		return HAM_ArsenalBoardStore.IsStaff(manager.GetPlayerIdFromControlledEntity(user));
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ArsenalBoardUI.Open(pOwnerEntity, HAM_EArsenalBoardMode.PARAMS);
	}
}

class HAM_VehicleFilterUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !user)
			return false;

		return HAM_ArsenalBoardStore.IsStaff(manager.GetPlayerIdFromControlledEntity(user));
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehicleBoardAskSync();

		HAM_ArsenalBoardUI.Open(pOwnerEntity, HAM_EArsenalBoardMode.VEHICLE_FILTER);
	}
}

class HAM_VehicleParamUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !user)
			return false;

		return HAM_ArsenalBoardStore.IsStaff(manager.GetPlayerIdFromControlledEntity(user));
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehicleBoardAskSync();

		HAM_ArsenalBoardUI.Open(pOwnerEntity, HAM_EArsenalBoardMode.VEHICLE_PARAMS);
	}
}

class HAM_ArsenalBoardMenu : ChimeraMenuBase
{
	protected static IEntity s_Board;
	protected static HAM_EArsenalBoardMode s_Mode;
	protected ref HAM_ArsenalBoardHandler m_Handler;

	static void SetBoard(IEntity board, HAM_EArsenalBoardMode mode)
	{
		s_Board = board;
		s_Mode = mode;
	}

	static IEntity GetBoard()
	{
		return s_Board;
	}

	static HAM_EArsenalBoardMode GetMode()
	{
		return s_Mode;
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

		m_Handler = new HAM_ArsenalBoardHandler(s_Board, root, s_Mode);
		root.AddHandler(m_Handler);
		m_Handler.Bind();
		HAM_ArsenalBoardUI.Remember(this);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(root);
	}

	override void OnMenuClose()
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
		{
			player.HAM_ArsenalBoardAskFlush();
			player.HAM_VehicleBoardAskFlush();
		}

		super.OnMenuClose();
		m_Handler = null;
		s_Board = null;
		HAM_ArsenalBoardUI.ForgetMenu();
	}
}

class HAM_ArsenalBoardUI
{
	protected static MenuBase s_Menu;

	static void Remember(MenuBase menu)
	{
		s_Menu = menu;
	}

	static void Open(IEntity board, HAM_EArsenalBoardMode mode)
	{
		if (!board)
			return;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus)
			return;

		if (s_Menu)
		{
			Close();
			return;
		}

		HAM_ArsenalBoardMenu.SetBoard(board, mode);
		s_Menu = menus.OpenMenu(ChimeraMenuPreset.HAM_ArsenalBoard);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_ArsenalBoardAskSync();
	}

	static void Close()
	{
		MenuBase menu = s_Menu;
		s_Menu = null;
		if (menu)
			menu.Close();
	}

	static void ForgetMenu()
	{
		s_Menu = null;
	}
}

class HAM_ArsenalBoardHandler : ScriptedWidgetEventHandler
{
	protected static const int PAGE_SIZE = 12;

	protected IEntity m_Board;
	protected Widget m_wRoot;
	protected float m_fNextClick;
	protected HAM_EArsenalBoardMode m_eMode;
	protected string m_sSearch;
	protected int m_iPage;
	protected int m_iCategory;
	protected bool m_bDetail;
	protected string m_sDetailPrefab;
	protected string m_sListPrefab;
	protected int m_iListIndex;
	protected int m_iRankIndex;
	protected bool m_bDetailIsBag;
	protected ref array<string> m_aPrefab;
	protected ref array<string> m_aLabel;
	protected ref array<int> m_aShown;
	protected ref array<ref SCR_ArsenalItem> m_aItems;
	protected ref array<SCR_ECharacterRank> m_aRanks;
	protected ref array<string> m_aCatNames;

	void HAM_ArsenalBoardHandler(IEntity board, Widget root, HAM_EArsenalBoardMode mode)
	{
		m_Board = board;
		m_wRoot = root;
		m_eMode = mode;
		m_sSearch = string.Empty;
		m_iListIndex = -1;
		m_iPage = 0;
		m_iCategory = 0;
		m_bDetail = false;
		m_aPrefab = {};
		m_aLabel = {};
		m_aShown = {};
		m_aItems = {};
		m_aRanks = {};
		m_aCatNames = {};
		m_aCatNames.Insert("Tous");
		m_aCatNames.Insert("Armes");
		m_aCatNames.Insert("Munitions");
		m_aCatNames.Insert("Équipement");
		m_aCatNames.Insert("Soins");
		m_aCatNames.Insert("Explosifs");
		m_aCatNames.Insert("Autre");
		BuildRankList();
	}

	void Bind()
	{
		float sw = 1920;
		float sh = 1080;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			workspace.GetScreenSize(sw, sh);
			sw = workspace.DPIUnscale(sw);
			sh = workspace.DPIUnscale(sh);
		}

		if (sw < 640)
			sw = 1920;

		if (sh < 480)
			sh = 1080;

		float panelW = sw * 0.82;
		float panelH = sh * 0.84;
		if (panelW > 1560)
			panelW = 1560;

		if (panelH > 920)
			panelH = 920;

		if (panelW > sw - 40)
			panelW = sw - 40;

		if (panelH > sh - 40)
			panelH = sh - 40;

		float innerW = panelW - 32;
		Place("Panel", 0, 0, panelW, panelH);
		Place("Title", 16, 12, innerW * 0.55, 36);
		Place("Info", 16, 50, innerW * 0.55, 28);

		float listW = innerW * 0.52;
		if (listW < 420)
			listW = 420;

		if (listW > innerW - 280)
			listW = innerW - 280;

		Place("SearchBg", 16, 86, listW - 150, 36);
		Place("Search", 16, 86, listW - 150, 36);
		Place("SearchBtn", 16 + listW - 140, 86, 130, 36);
		Widget searchBg = m_wRoot.FindAnyWidget("SearchBg");
		if (searchBg)
			searchBg.SetColor(new Color(0.16, 0.22, 0.34, 1));
		Widget searchBox = m_wRoot.FindAnyWidget("Search");
		if (searchBox)
		{
			searchBox.SetColor(new Color(0.95, 0.96, 0.97, 1));
			searchBox.AddHandler(this);
		}
		Place("CatPrev", 16, 128, 44, 32);
		Place("CatValue", 68, 128, listW - 120, 32);
		Place("CatNext", 16 + listW - 44, 128, 44, 32);

		float listTop = 172;
		float rowH = 40;
		float footer = 56;
		float avail = panelH - listTop - footer;
		float fitH = avail / PAGE_SIZE;
		if (fitH < rowH)
			rowH = fitH;

		if (rowH < 28)
			rowH = 28;

		for (int row = 0; row < PAGE_SIZE; row++)
		{
			Place("Item" + row.ToString(), 16, listTop + row * rowH, listW, rowH - 4);
			BindButton("Item" + row.ToString());
		}

		Place("Prev", 16, panelH - 48, 140, 36);
		Place("Next", 168, panelH - 48, 140, 36);
		Place("Close", panelW - 176, panelH - 48, 150, 36);

		float cardX = 16 + listW + 16;
		float cardW = panelW - cardX - 16;
		Place("DetailName", cardX, 86, cardW, 36);
		float previewH = panelH - 280;
		if (previewH < 180)
			previewH = 180;

		if (previewH > 420)
			previewH = 420;

		Place("ItemPreview", cardX, 130, cardW, previewH);
		Place("SaveBtn", cardX, 130 + previewH + 8, cardW, 40);
		float fieldY = 130 + previewH + 56;
		Place("CostLabel", cardX, fieldY, 140, 28);
		Place("CostEdit", cardX + 148, fieldY, cardW - 148, 28);
		Place("RankLabel", cardX, fieldY + 36, 140, 28);
		Place("RankPrev", cardX + 148, fieldY + 36, 40, 28);
		Place("RankValue", cardX + 196, fieldY + 36, cardW - 250, 28);
		Place("RankNext", cardX + cardW - 44, fieldY + 36, 40, 28);
		Place("BagLabel", cardX, fieldY + 72, 140, 28);
		Place("BagEdit", cardX + 148, fieldY + 72, cardW - 148, 28);
		Place("SaveBtn", cardX, fieldY + 112, 130, 34);
		if (IsFilterMode())
			Place("SaveBtn", cardX, 130 + previewH + 8, cardW, 40);

		Place("ResetBtn", cardX + 140, fieldY + 112, 140, 34);
		Place("BackBtn", cardX + 290, fieldY + 112, 120, 34);

		BindButton("SearchBtn");
		BindButton("CatPrev");
		BindButton("CatNext");
		BindButton("Prev");
		BindButton("Next");
		BindButton("Close");
		BindButton("SaveBtn");
		BindButton("ResetBtn");
		BindButton("BackBtn");
		BindButton("RankPrev");
		BindButton("RankNext");

		if (IsVehicleMode())
		{
			m_aCatNames.Clear();
			m_aCatNames.Insert("Tous");
			m_aCatNames.Insert("Léger");
			m_aCatNames.Insert("Char");
			m_aCatNames.Insert("Lourd");
			m_aCatNames.Insert("Aérien");
			m_aCatNames.Insert("Autre");
		}

		if (m_eMode == HAM_EArsenalBoardMode.FILTER)
			SetText("Title", "Tri arsenal");
		else if (m_eMode == HAM_EArsenalBoardMode.PARAMS)
			SetText("Title", "Paramétrage arsenal");
		else if (m_eMode == HAM_EArsenalBoardMode.VEHICLE_FILTER)
			SetText("Title", "Tri véhicules");
		else
			SetText("Title", "Paramétrage véhicules");

		ShowListPage();
		RefreshCatLabel();
		Rebuild();
	}

	protected string ButtonName(Widget w)
	{
		while (w)
		{
			string name = w.GetName();
			if (name == "Close" || name == "Prev" || name == "Next" || name == "SearchBtn")
				return name;

			if (name == "CatPrev" || name == "CatNext" || name == "SaveBtn" || name == "ResetBtn" || name == "BackBtn")
				return name;

			if (name == "RankPrev" || name == "RankNext")
				return name;

			if (name == "ItemPreview")
			{
				w = w.GetParent();
				continue;
			}

			if (name.IndexOf("Item") == 0 && !name.Contains("Text"))
				return name;

			w = w.GetParent();
		}

		return string.Empty;
	}

	protected void Place(string name, float x, float y, float width, float height)
	{
		if (!m_wRoot)
			return;

		Widget widget = m_wRoot.FindAnyWidget(name);
		if (!widget)
			return;

		if (name == "Panel")
		{
			FrameSlot.SetAnchorMin(widget, 0.5, 0.5);
			FrameSlot.SetAnchorMax(widget, 0.5, 0.5);
			FrameSlot.SetPos(widget, -width * 0.5, -height * 0.5);
			FrameSlot.SetSize(widget, width, height);
			widget.SetVisible(true);
			widget.SetOpacity(1);
			return;
		}

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

	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box || box.GetName() != "Search")
			return false;

		m_sSearch = box.GetText();
		m_iPage = 0;
		Rebuild();
		return false;
	}

	override bool OnClick(Widget w, int x, int y, int button)
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
			HAM_ArsenalBoardUI.Close();
			return true;
		}

		if (name == "SearchBtn")
		{
			EditBoxWidget box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Search"));
			if (box)
				m_sSearch = box.GetText();
			else
				m_sSearch = string.Empty;

			m_iPage = 0;
			Rebuild();
			return true;
		}

		if (name == "CatPrev")
		{
			ShiftCategory(-1);
			return true;
		}

		if (name == "CatNext")
		{
			ShiftCategory(1);
			return true;
		}

		if (name == "Prev")
		{
			if (m_iPage > 0)
			{
				m_iPage = m_iPage - 1;
				PaintRows();
			}
			return true;
		}

		if (name == "Next")
		{
			int maxPage = 0;
			if (m_aPrefab.Count() > 0)
				maxPage = (m_aPrefab.Count() - 1) / PAGE_SIZE;

			if (m_iPage < maxPage)
			{
				m_iPage = m_iPage + 1;
				PaintRows();
			}
			return true;
		}

		if (name == "BackBtn")
		{
			ShowListPage();
			Rebuild();
			return true;
		}

		if (name == "RankPrev")
		{
			ShiftRank(-1);
			return true;
		}

		if (name == "RankNext")
		{
			ShiftRank(1);
			return true;
		}

		if (name == "SaveBtn")
		{
			if (IsFilterMode())
				ApplyFilter();
			else
				SaveDetail();
			return true;
		}

		if (name == "ResetBtn")
		{
			ResetDetail();
			return true;
		}

		if (name.Length() >= 4 && name.Substring(0, 4) == "Item")
		{
			int local = name.Substring(4, name.Length() - 4).ToInt();
			int index = m_iPage * PAGE_SIZE + local;
			if (index < 0 || index >= m_aPrefab.Count())
				return false;

			OnRowClick(index);
			return true;
		}

		return false;
	}

	protected void OnRowClick(int index)
	{
		string prefab = m_aPrefab[index];
		m_sListPrefab = prefab;
		if (!m_bDetail)
			ShowListPreview();

		if (IsFilterMode())
		{
			m_iListIndex = index;
			ShowListPreview();
			return;
		}

		OpenDetail(prefab, index);
	}

	protected void ApplyFilter()
	{
		if (m_sListPrefab.IsEmpty() || m_iListIndex < 0 || m_iListIndex >= m_aShown.Count())
			return;

		string prefab = m_sListPrefab;
		bool next = m_aShown[m_iListIndex] == 0;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (IsVehicleMode())
		{
			if (player)
				player.HAM_VehicleBoardAskSetShown(prefab, next);

			HAM_VehicleBoardStore.SetShown(prefab, next);
		}
		else
		{
			if (player)
				player.HAM_ArsenalBoardAskSetShown(prefab, next);

			HAM_ArsenalBoardStore.SetShown(prefab, next);
		}

		Rebuild();
		ShowListPreview();
	}

	protected void OpenDetail(string prefab, int index)
	{
		if (IsVehicleMode())
		{
			OpenVehicleDetail(prefab);
			return;
		}
		m_bDetail = true;
		m_sDetailPrefab = prefab;
		SCR_ArsenalItem item;
		if (index >= 0 && index < m_aItems.Count())
			item = m_aItems[index];

		int cost = 0;
		SCR_ECharacterRank rank = SCR_ECharacterRank.PRIVATE;
		if (item)
		{
			cost = item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false);
			SCR_ArsenalItem.HAM_SetRawRank(true);
			rank = item.GetRequiredRank();
			SCR_ArsenalItem.HAM_SetRawRank(false);
			m_bDetailIsBag = HAM_ArsenalBoardStore.IsBackpackItem(item);
		}
		else
		{
			m_bDetailIsBag = HAM_ArsenalCatalog.IsCarrierPrefab(prefab);
		}

		cost = HAM_ArsenalBoardStore.GetCostOverride(prefab, cost);
		rank = HAM_ArsenalBoardStore.GetRankOverride(prefab, rank);
		m_iRankIndex = RankToIndex(rank);

		ShowDetailPage();
		SetText("DetailName", DisplayName(prefab));
		ShowItemPreview(prefab);
		SetText("BagLabel", "Emport");
		SetEdit("CostEdit", cost.ToString());
		RefreshRankLabel();

		Widget bagLabel = m_wRoot.FindAnyWidget("BagLabel");
		Widget bagEdit = m_wRoot.FindAnyWidget("BagEdit");
		if (bagLabel)
			bagLabel.SetVisible(m_bDetailIsBag);

		if (bagEdit)
		{
			bagEdit.SetVisible(m_bDetailIsBag);
			if (m_bDetailIsBag)
			{
				float vol = HAM_ArsenalBoardStore.GetBagOverride(prefab, 0);
				SetEdit("BagEdit", vol.ToString());
			}
		}
	}

	protected void SaveDetail()
	{
		if (IsVehicleMode())
		{
			SaveVehicleDetail();
			return;
		}
		if (m_sDetailPrefab.IsEmpty())
			return;

		int cost = EditInt("CostEdit", 0);
		SCR_ECharacterRank rank = IndexToRank(m_iRankIndex);
		float bag = 0;
		bool setBag = m_bDetailIsBag;
		if (setBag)
			bag = EditFloat("BagEdit", 0);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_ArsenalBoardAskApplyParams(m_sDetailPrefab, cost, rank, bag, setBag);

		HAM_ArsenalBoardStore.ApplyItemParams(m_sDetailPrefab, cost, rank, bag, setBag);
		SetText("Info", "Enregistré : " + DisplayName(m_sDetailPrefab));
	}

	protected void ResetDetail()
	{
		if (IsVehicleMode())
		{
			ResetVehicleDetail();
			return;
		}
		if (m_sDetailPrefab.IsEmpty())
			return;

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_ArsenalBoardAskResetParams(m_sDetailPrefab);

		HAM_ArsenalBoardStore.ClearAllOverrides(m_sDetailPrefab);
		ShowListPage();
		Rebuild();
		SetText("Info", "Réinitialisé : " + DisplayName(m_sDetailPrefab));
	}

	protected void Rebuild()
	{
		if (IsVehicleMode())
		{
			RebuildVehicles();
			return;
		}
		m_aPrefab.Clear();
		m_aLabel.Clear();
		m_aShown.Clear();
		m_aItems.Clear();

		array<SCR_ArsenalItem> raw = {};
		// Liste UI non filtrée (pour ré-inclure les exclus).
		CollectAllRaw(raw);

		array<string> seen = {};
		foreach (SCR_ArsenalItem item : raw)
		{
			if (!item)
				continue;

			string prefab = item.GetItemResourceName();
			int prefabLength = prefab.Length();
			if (prefabLength > 0)
				prefab = prefab.Substring(0, prefabLength);

			string key = PrefabKey(prefab);
			// Un nom vide n'identifie pas l'objet : le jeter viderait le tableau.
			if (!key.IsEmpty())
			{
				if (seen.Contains(key))
					continue;

				seen.Insert(key);
			}

			AddListedPrefab(prefab, item);
		}

		array<string> catalogPrefabs = {};
		HAM_ArsenalCatalog.CollectPrefabs(catalogPrefabs);
		foreach (string catalogPrefab : catalogPrefabs)
		{
			string key = PrefabKey(catalogPrefab);
			if (!key.IsEmpty())
			{
				if (seen.Contains(key))
					continue;

				seen.Insert(key);
			}

			AddListedPrefab(catalogPrefab, null);
		}

		string info = "Objets : " + m_aPrefab.Count().ToString();
		if (m_aPrefab.IsEmpty())
		{
			if (!SCR_EntityCatalogManagerComponent.GetInstance())
				info = info + " — gestionnaire absent";
			else
				info = info + " — catalogue vide";
		}

		if (IsFilterMode())
			SetText("Info", "Vert = inclus. Rouge = exclus. " + info);
		else
			SetText("Info", "Choisir un item. * = override. " + info);

		PaintRows();
		if (!m_bDetail)
			ShowListPreview();
	}

	protected string PrefabKey(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace >= 0 && brace + 1 < prefab.Length())
			return prefab.Substring(brace + 1, prefab.Length() - brace - 1);

		return prefab;
	}

	protected void AddListedPrefab(string prefab, SCR_ArsenalItem item)
	{
		string label = DisplayName(prefab);
		if (!SearchHit(label))
			return;

		int category = 6;
		if (item)
			category = CategoryOf(item);
		else
			category = HAM_ArsenalCatalog.CategoryOfPrefab(prefab) + 1;

		if (m_iCategory > 0 && category != m_iCategory)
			return;

		bool shown = HAM_ArsenalBoardStore.IsShown(prefab);
		if (m_eMode != HAM_EArsenalBoardMode.FILTER && !shown)
			return;

		if (m_eMode == HAM_EArsenalBoardMode.FILTER)
		{
			if (shown)
				label = label + "    Inclus";
			else
				label = label + "    Exclus";
		}
		else
		{
			if (HAM_ArsenalBoardStore.HasCostOverride(prefab) || HAM_ArsenalBoardStore.HasRankOverride(prefab) || HAM_ArsenalBoardStore.HasBagOverride(prefab))
				label = label + "    *";
		}

		m_aPrefab.Insert(prefab);
		m_aLabel.Insert(label);
		if (item)
			m_aItems.Insert(item);

		if (shown)
			m_aShown.Insert(1);
		else
			m_aShown.Insert(0);
	}

	protected void FillFromPrefabs()
	{
		array<string> prefabs = {};
		HAM_ArsenalCatalog.CollectPrefabs(prefabs);
		foreach (string prefab : prefabs)
			AddListedPrefab(prefab, null);
	}

	protected void CollectAllRaw(out array<SCR_ArsenalItem> items)
	{
		items.Clear();
		HAM_ArsenalBoardStore.SetFilterPaused(true);

		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalog)
			catalog.GetAllArsenalItems(items);

		if (items.IsEmpty())
			CollectFromFactions(items);

		HAM_ArsenalBoardStore.SetFilterPaused(false);
	}

	protected void CollectFromFactions(out array<SCR_ArsenalItem> items)
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return;

		array<Faction> factions = {};
		manager.GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			SCR_Faction scr = SCR_Faction.Cast(faction);
			if (!scr)
				continue;

			SCR_EntityCatalog itemCatalog = scr.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, false);
			if (itemCatalog)
				AppendCatalogItems(itemCatalog, items);
		}
	}

	protected void AppendCatalogItems(notnull SCR_EntityCatalog itemCatalog, notnull array<SCR_ArsenalItem> items)
	{
		array<SCR_EntityCatalogEntry> entries = {};
		array<SCR_BaseEntityCatalogData> dataList = {};
		itemCatalog.GetEntityListWithData(SCR_ArsenalItem, entries, dataList);
		foreach (SCR_BaseEntityCatalogData data : dataList)
		{
			SCR_ArsenalItem item = SCR_ArsenalItem.Cast(data);
			if (!item)
				continue;

			string prefab = item.GetItemResourceName();
			int prefabLength = prefab.Length();
			if (prefabLength > 0)
				prefab = prefab.Substring(0, prefabLength);

			if (prefab.IsEmpty())
			{
				items.Insert(item);
				continue;
			}

			bool exists = false;
			foreach (SCR_ArsenalItem known : items)
			{
				if (!known)
					continue;

				string knownPrefab = known.GetItemResourceName();
				int knownLength = knownPrefab.Length();
				if (knownLength > 0)
					knownPrefab = knownPrefab.Substring(0, knownLength);

				if (knownPrefab == prefab)
				{
					exists = true;
					break;
				}
			}

			if (!exists)
				items.Insert(item);
		}
	}

	protected void PaintRows()
	{
		int start = m_iPage * PAGE_SIZE;
		for (int i = 0; i < PAGE_SIZE; i++)
		{
			int index = start + i;
			ButtonWidget btn = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Item" + i.ToString()));
			TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("Item" + i.ToString() + "Text"));
			if (!btn)
				continue;

			if (index >= m_aPrefab.Count())
			{
				btn.SetVisible(false);
				if (label)
					label.SetText("—");
				continue;
			}

			btn.SetVisible(true);
			if (label)
				label.SetText(m_aLabel[index]);

			if (IsFilterMode())
			{
				if (m_aShown[index] != 0)
					btn.SetColor(new Color(0.05, 0.28, 0.14, 1));
				else
					btn.SetColor(new Color(0.28, 0.12, 0.12, 1));
			}
			else
			{
				btn.SetColor(new Color(0.118, 0.227, 0.373, 1));
			}
		}
	}

	protected void ShowListPage()
	{
		m_bDetail = false;
		SetNamedVisible("SearchBg", true);
		SetNamedVisible("Search", true);
		SetNamedVisible("SearchBtn", true);
		SetNamedVisible("CatPrev", true);
		SetNamedVisible("CatValue", true);
		SetNamedVisible("CatNext", true);
		SetNamedVisible("Prev", true);
		SetNamedVisible("Next", true);
		for (int i = 0; i < PAGE_SIZE; i++)
			SetNamedVisible("Item" + i.ToString(), true);

		SetNamedVisible("DetailName", true);
		SetNamedVisible("ItemPreview", true);
		SetNamedVisible("CostLabel", false);
		SetNamedVisible("CostEdit", false);
		SetNamedVisible("RankLabel", false);
		SetNamedVisible("RankPrev", false);
		SetNamedVisible("RankValue", false);
		SetNamedVisible("RankNext", false);
		SetNamedVisible("BagLabel", false);
		SetNamedVisible("BagEdit", false);
		SetNamedVisible("SaveBtn", IsFilterMode());
		if (IsFilterMode())
			SetText("SaveBtn", "Valider");
		SetNamedVisible("ResetBtn", false);
		SetNamedVisible("BackBtn", false);
	}

	protected void ShowListPreview()
	{
		if (m_bDetail)
			return;

		if (m_sListPrefab.IsEmpty())
		{
			SetText("DetailName", "Sélectionner un objet");
			ShowItemPreview("");
			return;
		}

		SetText("DetailName", DisplayName(m_sListPrefab));
		ShowItemPreview(m_sListPrefab);
	}

	protected void ShowDetailPage()
	{
		SetNamedVisible("SearchBg", false);
		SetNamedVisible("Search", false);
		SetNamedVisible("SearchBtn", false);
		SetNamedVisible("CatPrev", false);
		SetNamedVisible("CatValue", false);
		SetNamedVisible("CatNext", false);
		SetNamedVisible("Prev", false);
		SetNamedVisible("Next", false);
		for (int i = 0; i < PAGE_SIZE; i++)
			SetNamedVisible("Item" + i.ToString(), false);

		SetNamedVisible("DetailName", true);
		SetNamedVisible("ItemPreview", true);
		SetNamedVisible("CostLabel", true);
		SetNamedVisible("CostEdit", true);
		SetNamedVisible("RankLabel", true);
		SetNamedVisible("RankPrev", true);
		SetNamedVisible("RankValue", true);
		SetNamedVisible("RankNext", true);
		SetNamedVisible("SaveBtn", true);
		SetNamedVisible("ResetBtn", true);
		SetNamedVisible("BackBtn", true);
		SetNamedVisible("BagLabel", m_bDetailIsBag);
		SetNamedVisible("BagEdit", m_bDetailIsBag);
	}

	protected void SetNamedVisible(string name, bool visible)
	{
		if (!m_wRoot)
			return;

		Widget widget = m_wRoot.FindAnyWidget(name);
		if (widget)
			widget.SetVisible(visible);
	}

	protected void ShowItemPreview(string prefab)
	{
		if (!m_wRoot)
			return;

		ItemPreviewWidget preview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("ItemPreview"));
		if (!preview)
			return;

		if (prefab.IsEmpty())
		{
			preview.SetVisible(false);
			return;
		}

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
		if (!manager)
			return;

		preview.SetVisible(true);
		manager.SetPreviewItemFromPrefab(preview, prefab);
	}

	protected void BuildRankList()
	{
		m_aRanks = {};
		m_aRanks.Insert(SCR_ECharacterRank.PRIVATE);
		m_aRanks.Insert(SCR_ECharacterRank.CORPORAL);
		m_aRanks.Insert(SCR_ECharacterRank.SERGEANT);
		m_aRanks.Insert(SCR_ECharacterRank.LIEUTENANT);
		m_aRanks.Insert(SCR_ECharacterRank.CAPTAIN);
		m_aRanks.Insert(SCR_ECharacterRank.MAJOR);
		m_aRanks.Insert(SCR_ECharacterRank.COLONEL);
	}

	protected int RankToIndex(SCR_ECharacterRank rank)
	{
		for (int i = 0; i < m_aRanks.Count(); i++)
		{
			if (m_aRanks[i] == rank)
				return i;
		}

		return 0;
	}

	protected SCR_ECharacterRank IndexToRank(int index)
	{
		if (index < 0 || index >= m_aRanks.Count())
			return SCR_ECharacterRank.PRIVATE;

		return m_aRanks[index];
	}

	protected void ShiftRank(int step)
	{
		m_iRankIndex = m_iRankIndex + step;
		if (m_iRankIndex < 0)
			m_iRankIndex = m_aRanks.Count() - 1;

		if (m_iRankIndex >= m_aRanks.Count())
			m_iRankIndex = 0;

		RefreshRankLabel();
	}

	protected void RefreshRankLabel()
	{
		SCR_ECharacterRank rank = IndexToRank(m_iRankIndex);
		SetText("RankValue", typename.EnumToString(SCR_ECharacterRank, rank));
	}

	protected bool SearchHit(string label)
	{
		if (m_sSearch.IsEmpty())
			return true;

		string low = label;
		low.ToLower();
		string query = m_sSearch;
		query.ToLower();
		return low.Contains(query);
	}

	protected void ShiftCategory(int step)
	{
		m_iCategory = m_iCategory + step;
		if (m_iCategory < 0)
			m_iCategory = m_aCatNames.Count() - 1;

		if (m_iCategory >= m_aCatNames.Count())
			m_iCategory = 0;

		m_iPage = 0;
		RefreshCatLabel();
		Rebuild();
	}

	protected void RefreshCatLabel()
	{
		if (m_iCategory < 0 || m_iCategory >= m_aCatNames.Count())
			SetText("CatValue", "Tous");
		else
			SetText("CatValue", m_aCatNames[m_iCategory]);
	}

	protected int CategoryOf(SCR_ArsenalItem item)
	{
		if (!item)
			return 6;

		return HAM_ArsenalCatalog.CategoryOfPrefab(item.GetItemResourceName()) + 1;
	}

	protected string DisplayName(string prefab)
	{
		if (IsVehicleMode())
			return HAM_VehicleCatalog.Label(prefab);

		return HAM_ArsenalCatalog.ItemLabel(prefab);
	}

	protected bool IsVehicleMode()
	{
		return m_eMode == HAM_EArsenalBoardMode.VEHICLE_FILTER || m_eMode == HAM_EArsenalBoardMode.VEHICLE_PARAMS;
	}

	protected bool IsFilterMode()
	{
		return m_eMode == HAM_EArsenalBoardMode.FILTER || m_eMode == HAM_EArsenalBoardMode.VEHICLE_FILTER;
	}

	protected void RebuildVehicles()
	{
		m_aPrefab.Clear();
		m_aLabel.Clear();
		m_aShown.Clear();
		m_aItems.Clear();

		array<string> prefabs = {};
		HAM_VehicleCatalog.Collect(prefabs);
		foreach (string prefab : prefabs)
		{
			string label = DisplayName(prefab);
			if (!SearchHit(label))
				continue;

			int category = HAM_VehicleCatalog.Category(prefab);
			if (m_iCategory > 0 && category != m_iCategory)
				continue;

			bool shown = HAM_VehicleBoardStore.IsShown(prefab);
			if (!IsFilterMode() && !shown)
				continue;

			if (IsFilterMode())
			{
				if (shown)
					label = label + "    Inclus";
				else
					label = label + "    Exclus";
			}
			else if (HAM_VehicleBoardStore.HasCostOverride(prefab) || HAM_VehicleBoardStore.HasRankOverride(prefab) || HAM_VehicleBoardStore.HasCapacityOverride(prefab))
			{
				label = label + "    *";
			}

			m_aPrefab.Insert(prefab);
			m_aLabel.Insert(label);
			if (shown)
				m_aShown.Insert(1);
			else
				m_aShown.Insert(0);
		}

		string info = "Véhicules : " + m_aPrefab.Count().ToString();
		if (m_aPrefab.IsEmpty())
		{
			if (!SCR_EntityCatalogManagerComponent.GetInstance())
				info = info + " — gestionnaire absent";
			else
				info = info + " — catalogue vide";
		}

		if (IsFilterMode())
			SetText("Info", "Vert = inclus. Rouge = exclus. " + info);
		else
			SetText("Info", "Choisir un véhicule. * = override. " + info);

		PaintRows();
		if (!m_bDetail)
		{
			if (m_aPrefab.Find(m_sListPrefab) < 0)
				m_sListPrefab = string.Empty;

			ShowListPreview();
		}
	}

	protected void OpenVehicleDetail(string prefab)
	{
		m_bDetail = true;
		m_sDetailPrefab = prefab;
		m_bDetailIsBag = true;
		int cost = HAM_VehicleBoardStore.GetCostOverride(prefab, HAM_VehicleCatalog.VanillaCost(prefab));
		SCR_ECharacterRank rank = HAM_VehicleBoardStore.GetRankOverride(prefab, HAM_VehicleCatalog.VanillaRank(prefab));
		m_iRankIndex = RankToIndex(rank);

		ShowDetailPage();
		SetText("DetailName", DisplayName(prefab));
		ShowItemPreview(prefab);
		SetText("BagLabel", "Volume coffre");
		SetEdit("CostEdit", cost.ToString());
		if (HAM_VehicleBoardStore.HasCapacityOverride(prefab))
			SetEdit("BagEdit", HAM_VehicleBoardStore.GetCapacityOverride(prefab, 0).ToString());
		else
			SetEdit("BagEdit", "");
		RefreshRankLabel();
	}

	protected void SaveVehicleDetail()
	{
		if (m_sDetailPrefab.IsEmpty())
			return;

		int cost = EditInt("CostEdit", 0);
		SCR_ECharacterRank rank = IndexToRank(m_iRankIndex);
		float cargo = -1;
		EditBoxWidget cargoBox = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("BagEdit"));
		if (cargoBox)
		{
			string cargoText = cargoBox.GetText();
			cargoText.Trim();
			if (!cargoText.IsEmpty())
				cargo = cargoText.ToFloat();
		}
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehicleBoardAskApplyParams(m_sDetailPrefab, cost, rank, cargo);

		HAM_VehicleBoardStore.ApplyVehicleParams(m_sDetailPrefab, cost, rank, cargo);
		SetText("Info", "Enregistré : " + DisplayName(m_sDetailPrefab));
	}

	protected void ResetVehicleDetail()
	{
		if (m_sDetailPrefab.IsEmpty())
			return;

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehicleBoardAskResetParams(m_sDetailPrefab);

		HAM_VehicleBoardStore.ClearAllOverrides(m_sDetailPrefab);
		ShowListPage();
		Rebuild();
		SetText("Info", "Réinitialisé : " + DisplayName(m_sDetailPrefab));
	}

	protected string CurrentRep()
	{
		float value = HAM_ClientState.s_fGlobalRep;
		if (Replication.IsServer())
			value = HAM_Mission.s_fGlobalRep;

		return HAM_Mission.FormatRep(value);
	}

	protected void SetText(string name, string text)
	{
		TextWidget w = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (w)
			w.SetText(text);
	}

	protected void SetEdit(string name, string text)
	{
		EditBoxWidget w = EditBoxWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (w)
			w.SetText(text);
	}

	protected int EditInt(string name, int fallback)
	{
		EditBoxWidget w = EditBoxWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!w)
			return fallback;

		return w.GetText().ToInt();
	}

	protected float EditFloat(string name, float fallback)
	{
		EditBoxWidget w = EditBoxWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!w)
			return fallback;

		return w.GetText().ToFloat();
	}
}
