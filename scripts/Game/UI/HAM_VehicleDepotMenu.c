//------------------------------------------------------------------------------------------------
// Menu d'entretien : arsenal à gauche, quantités au centre, coffre et presets à droite.
//------------------------------------------------------------------------------------------------

modded enum ChimeraMenuPreset
{
	HAM_VehicleDepot
}

void HAM_DepotSuppliesTick()
{
	HAM_VehicleDepotBoard.TickSupplies();
}

class HAM_VehicleDepotUserAction : ScriptedUserAction
{
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

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_VehicleDepotBoard.Toggle(pOwnerEntity);
	}
}

class HAM_VehicleDepotFilterUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (!user)
			return false;

		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return false;

		return HAM_VehicleDepotFilter.IsStaff(manager.GetPlayerIdFromControlledEntity(user));
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_VehicleDepotBoard.OpenFilter(pOwnerEntity);
	}
}

class HAM_VehicleDepotMenu : ChimeraMenuBase
{
	protected static IEntity s_Post;

	static void SetPost(IEntity post)
	{
		s_Post = post;
	}

	static IEntity GetPost()
	{
		return s_Post;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		if (!root)
			return;

		HAM_VehicleDepotHandler handler = new HAM_VehicleDepotHandler(s_Post, root);
		root.AddHandler(handler);
		handler.Bind();
		HAM_VehicleDepotBoard.Remember(this, handler);
	}

	override void OnMenuClose()
	{
		HAM_VehicleDepotBoard.StopSupplies();
		HAM_VehicleDepotBoard.ReleaseTrunk();
		super.OnMenuClose();
		s_Post = null;
		HAM_VehicleDepotBoard.Forget();
	}
}

class HAM_VehicleDepotBoard
{
	static const int SUPPLIES_PENDING = -2;

	protected static MenuBase s_Menu;
	protected static HAM_VehicleDepotHandler s_Handler;
	protected static string s_sCargo;
	protected static string s_sPresets;
	protected static int s_iPersistent;
	protected static int s_iSupplies;
	protected static bool s_bSupplies;
	protected static bool s_bKeepInventory;
	protected static bool s_bPendingTrunk;
	protected static bool s_bWantFilter;

	static void Remember(MenuBase menu, HAM_VehicleDepotHandler handler)
	{
		s_Menu = menu;
		s_Handler = handler;
	}

	static void Forget()
	{
		s_Menu = null;
		s_Handler = null;
	}

	static void StartSupplies()
	{
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (!queue)
			return;

		queue.Remove(HAM_DepotSuppliesTick);
		queue.CallLater(HAM_DepotSuppliesTick, 2000, true); // 2 s (avant 500 ms) : une RPC par tick tant que le menu est ouvert
	}

	static void StopSupplies()
	{
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (!queue)
			return;

		queue.Remove(HAM_DepotSuppliesTick);
	}

	static void TickSupplies()
	{
		if (!s_Handler)
			return;

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_DepotSuppliesAsk();
	}

	static void ReleaseTrunk()
	{
		if (s_bKeepInventory)
			return;

		if (s_Handler)
			s_Handler.ReleaseTrunk();
	}

	static void KeepInventory(bool keep)
	{
		s_bKeepInventory = keep;
	}

	static void RequestTrunk()
	{
		s_bPendingTrunk = true;
	}

	static bool TakePendingTrunk()
	{
		bool pending = s_bPendingTrunk;
		s_bPendingTrunk = false;
		return pending;
	}

	static void OpenVanilla(IEntity depot, IEntity vehicle, IEntity user)
	{
		if (!user)
			user = SCR_PlayerController.GetLocalControlledEntity();

		if (!user || !vehicle)
			return;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return;

		Close();
		s_iOpenTries = 0;
		s_OpenDepot = depot;
		s_OpenVehicle = vehicle;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_DepotMirror(HAM_RplUtil.IdOf(depot));

		inventory.OpenInventory();
		GetGame().GetCallqueue().CallLater(OpenVanillaDelayed, 200, false);
	}

	protected static IEntity s_OpenDepot;
	protected static IEntity s_OpenVehicle;
	protected static int s_iOpenTries;

	static IEntity Nearest(array<IEntity> vehicles, IEntity user)
	{
		if (!vehicles || vehicles.IsEmpty())
			return null;

		if (!user)
			return vehicles[0];

		IEntity best;
		float bestDist = -1;
		foreach (IEntity vehicle : vehicles)
		{
			if (!vehicle)
				continue;

			vector delta = vehicle.GetOrigin() - user.GetOrigin();
			delta[1] = 0;
			float dist = delta.LengthSq();
			bool closer = false;
			if (!best)
				closer = true;
			else if (dist < bestDist)
				closer = true;

			if (closer)
			{
				best = vehicle;
				bestDist = dist;
			}
		}

		return best;
	}

	protected static void OpenVanillaDelayed()
	{
		MenuBase menu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.Inventory20Menu);
		SCR_InventoryMenuUI ui = SCR_InventoryMenuUI.Cast(menu);
		if (!ui)
		{
			s_iOpenTries = s_iOpenTries + 1;
			if (s_iOpenTries < 8)
				GetGame().GetCallqueue().CallLater(OpenVanillaDelayed, 150, false);

			return;
		}

		BaseInventoryStorageComponent arsenal = FindArsenal(s_OpenDepot);
		BaseInventoryStorageComponent trunk = HAM_VehicleDepot.Trunk(s_OpenVehicle);
		if (!arsenal)
			Print("[13RDPA H&M Depot] arsenal introuvable", LogLevel.WARNING);

		if (!trunk)
			Print("[13RDPA H&M Depot] coffre introuvable", LogLevel.WARNING);

		ui.HAM_ShowDepot(arsenal, trunk);
	}

	protected static BaseInventoryStorageComponent FindArsenal(IEntity depot)
	{
		if (!depot)
			return null;

		IEntity child = depot.GetChildren();
		while (child)
		{
			BaseInventoryStorageComponent found = FindArsenal(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		if (depot.FindComponent(SCR_ArsenalInventoryStorageManagerComponent) && depot.FindComponent(SCR_ArsenalComponent))
			return BaseInventoryStorageComponent.Cast(depot.FindComponent(BaseInventoryStorageComponent));

		return null;
	}

	static void Close()
	{
		if (s_Menu)
			s_Menu.Close();
	}

	static void Toggle(IEntity post)
	{
		MenuManager menus = GetGame().GetMenuManager();
		if (!menus || !post)
			return;

		// Référence périmée (menu fermé sans OnMenuClose) : l'action restait bloquée sur « fermer ».
		MenuBase opened = menus.FindMenuByPreset(ChimeraMenuPreset.HAM_VehicleDepot);
		if (s_Menu && !opened)
			Forget();

		if (opened)
		{
			opened.Close();
			Forget();
			return;
		}

		HAM_VehicleDepotMenu.SetPost(post);
		s_Menu = menus.OpenMenu(ChimeraMenuPreset.HAM_VehicleDepot);
	}

	static void OpenFilter(IEntity post)
	{
		if (s_Menu)
			s_Menu.Close();

		s_bWantFilter = true;
		MenuManager menus = GetGame().GetMenuManager();
		if (!menus || !post)
			return;

		HAM_VehicleDepotMenu.SetPost(post);
		s_Menu = menus.OpenMenu(ChimeraMenuPreset.HAM_VehicleDepot);
	}

	static bool ConsumeFilter()
	{
		bool value = s_bWantFilter;
		s_bWantFilter = false;
		return value;
	}

	static void OnFilterPack(string pack, bool reset, bool done)
	{
		HAM_VehicleDepotFilter.ApplyPack(pack, reset);
		if (done && s_Handler)
			s_Handler.RefreshFilterList();
	}

	static void ApplyView(string cargo, string presets, int persistent, int supplies)
	{
		s_sCargo = cargo;
		s_sPresets = presets;
		s_iPersistent = persistent;
		s_iSupplies = supplies;
		s_bSupplies = true;
		if (s_Handler)
			s_Handler.ShowServer(cargo, presets, persistent);
	}

	static int KnownSupplies()
	{
		if (s_bSupplies)
			return s_iSupplies;

		if (Replication.IsServer())
		{
			vector pos = vector.Zero;
			IEntity post = HAM_VehicleDepotMenu.GetPost();
			if (post)
				pos = post.GetOrigin();

			return HAM_VehicleDepot.HomeSupplies(pos);
		}

		return SUPPLIES_PENDING;
	}

	static void OnSupplies(int amount)
	{
		s_iSupplies = amount;
		s_bSupplies = true;
		if (s_Handler)
			s_Handler.RefreshSupplies();
	}

	static string Cargo()
	{
		return s_sCargo;
	}

	static string Presets()
	{
		return s_sPresets;
	}

	static int Persistent()
	{
		return s_iPersistent;
	}
}

class HAM_VehicleDepotHandler : ScriptedWidgetEventHandler
{
	protected int m_iRows;
	protected static const string ROW = "{C096E3F6CA253663}UI/layouts/Menu/HAM_VehicleDepotRow.layout";

	protected IEntity m_Post;
	protected Widget m_wRoot;
	protected float m_fNextClick;
	protected float m_fRowW;
	protected float m_fRowStep;
	protected float m_fCargoRowW;
	protected float m_fCargoRowStep;
	protected int m_iCategory;
	protected int m_iItemPage;
	protected int m_iCargoPage;
	protected int m_iVehicle;
	protected int m_iItem;
	protected int m_iCargo;
	protected int m_iPreset;
	protected bool m_bCargoPick;
	protected bool m_bTrunkOpen;
	protected bool m_bFilter;
	protected string m_sSearch;
	protected ref array<IEntity> m_aVehicles;
	protected ref array<string> m_aItemPrefab;
	protected ref array<string> m_aItemLabel;
	protected ref array<int> m_aItemCost;
	protected ref array<int> m_aItemShown;
	protected string m_sFilterNote;
	protected ref array<string> m_aCargoPrefab;
	protected ref array<int> m_aCargoAmmo;
	protected ref array<int> m_aCargoCount;
	protected ref array<string> m_aPresetNames;
	protected ref array<Widget> m_aRows;

	void HAM_VehicleDepotHandler(IEntity post, Widget root)
	{
		m_Post = post;
		m_wRoot = root;
		m_iRows = 12;
		m_iCategory = -1;
		m_sSearch = "";
		m_aVehicles = {};
		m_aItemPrefab = {};
		m_aItemLabel = {};
		m_aItemCost = {};
		m_aItemShown = {};
		m_aCargoPrefab = {};
		m_aCargoAmmo = {};
		m_aCargoCount = {};
		m_aPresetNames = {};
		m_aRows = {};
		m_bFilter = HAM_VehicleDepotBoard.ConsumeFilter();
	}

	void Bind()
	{
		LayoutScreen();
		BindButton("Close");
		BindButton("Supplies");
		BindButton("Persist");
		BindButton("ConfirmCargo");
		BindButton("VehPrev");
		BindButton("VehNext");
		BindButton("Chercher");
		BindButton("CatAll");
		BindButton("Cat0");
		BindButton("Cat1");
		BindButton("Cat2");
		BindButton("Cat3");
		BindButton("Cat4");
		BindButton("Cat5");
		BindButton("ItemPrev");
		BindButton("ItemNext");
		BindButton("Add1");
		BindButton("Add5");
		BindButton("Add10");
		BindButton("Rem1");
		BindButton("Rem5");
		BindButton("Rem10");
		BindButton("ClearCargo");
		BindButton("OpenVanilla");
		BindButton("PresetPrev");
		BindButton("PresetNext");
		BindButton("PresetSave");
		BindButton("PresetDelete");
		BindButton("PresetApply");
		BindButton("CargoPrev");
		BindButton("CargoNext");
		BindButton("CargoPlus");
		BindButton("CargoMinus");
		RebuildItems();
		RefreshVehicles();
		ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
		if (m_bFilter)
			BeginFilterMode();
		else
			AskView();

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_DepotFilterAsk();

		if (HAM_VehicleDepotBoard.TakePendingTrunk())
			ShowTrunk();

		ShowSupplies();
		HAM_VehicleDepotBoard.StartSupplies();
	}

	void RefreshFilterList()
	{
		RebuildItems();
	}

	protected void BeginFilterMode()
	{
		HideWidget("VehPrev");
		HideWidget("VehNext");
		HideWidget("VehPreview");
		HideWidget("Persist");
		HideWidget("ConfirmCargo");
		HideWidget("Add1");
		HideWidget("Add5");
		HideWidget("Add10");
		HideWidget("Rem1");
		HideWidget("Rem5");
		HideWidget("Rem10");
		HideWidget("ClearCargo");
		HideWidget("PresetPrev");
		HideWidget("PresetNext");
		HideWidget("PresetEdit");
		HideWidget("PresetSave");
		HideWidget("PresetDelete");
		HideWidget("PresetApply");
		HideWidget("CargoPrev");
		HideWidget("CargoNext");
		HideWidget("CargoPlus");
		HideWidget("CargoMinus");
		HideWidget("CargoHost");
		SetWidgetText("Title", "Option de tri");
		SetWidgetText("VehName", "Tous les objets");
		SetWidgetText("OpenVanilla", "Basculer");
		TextWidget message = TextWidget.Cast(m_wRoot.FindAnyWidget("Message"));
		if (message)
			message.SetText("Vert = inclus. Rouge = exclus.");
	}

	void RefreshSupplies()
	{
		ShowSupplies();
	}

	protected void ShowSupplies()
	{
		int amount = HAM_VehicleDepotBoard.KnownSupplies();
		if (amount == HAM_VehicleDepotBoard.SUPPLIES_PENDING)
			SetWidgetText("Supplies", "QG : …");
		else if (amount == HAM_VehicleDepot.SUPPLIES_UNLIMITED)
			SetWidgetText("Supplies", "QG : sans limite");
		else
			SetWidgetText("Supplies", "QG : " + amount.ToString());
	}

	protected void HideWidget(string name)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (widget)
			widget.SetVisible(false);
	}

	protected void SetWidgetText(string name, string text)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (!widget)
			return;

		TextWidget direct = TextWidget.Cast(widget);
		if (direct)
		{
			direct.SetText(text);
			return;
		}

		TextWidget child = TextWidget.Cast(widget.GetChildren());
		if (child)
			child.SetText(text);
	}

	void ShowServer(string cargo, string presets, int persistent)
	{
		m_aCargoPrefab.Clear();
		m_aCargoAmmo.Clear();
		m_aCargoCount.Clear();
		HAM_VehicleDepot.SplitPack(cargo, m_aCargoPrefab, m_aCargoAmmo, m_aCargoCount);
		m_aPresetNames.Clear();
		string rest = presets;
		while (!rest.IsEmpty())
		{
			int cut = rest.IndexOf("|");
			string part = rest;
			if (cut >= 0)
			{
				part = rest.Substring(0, cut);
				rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
			}
			else
			{
				rest = "";
			}

			if (!part.IsEmpty())
				m_aPresetNames.Insert(part);
		}

		if (m_iPreset >= m_aPresetNames.Count())
			m_iPreset = 0;

		TextWidget persist = TextWidget.Cast(m_wRoot.FindAnyWidget("PersistLabel"));
		if (persist)
		{
			if (persistent == 1)
				persist.SetText("Inventaire persistant : oui");
			else
				persist.SetText("Inventaire persistant : non");
		}

		TextWidget presetName = TextWidget.Cast(m_wRoot.FindAnyWidget("PresetName"));
		if (presetName)
		{
			if (m_aPresetNames.IsEmpty())
				presetName.SetText("Aucun preset");
			else
				presetName.SetText(m_aPresetNames[m_iPreset]);
		}

		PaintRows("CargoHost", m_aCargoPrefab, m_aCargoCount, m_aCargoAmmo, m_iCargoPage, m_iCargo, "Cargo");
		ShowSupplies();
		if (!m_bTrunkOpen)
			return;

		if (m_aCargoPrefab.IsEmpty())
			PaintEmptyTrunk();

		int total = 0;
		foreach (int count : m_aCargoCount)
			total = total + count;

		TextWidget message = TextWidget.Cast(m_wRoot.FindAnyWidget("Message"));
		if (message)
			message.SetText("Coffre : " + total.ToString() + " objet(s)");
	}

	protected void ShowTrunk()
	{
		m_bTrunkOpen = true;
		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.CloseMenuByPreset(ChimeraMenuPreset.Inventory20Menu);

		IEntity vehicle = CurrentVehicle();
		TextWidget message = TextWidget.Cast(m_wRoot.FindAnyWidget("Message"));
		if (!vehicle)
		{
			if (message)
				message.SetText("Selectionne un vehicule.");

			return;
		}

		HAM_VehicleDepot.ReadTrunk(vehicle, m_aCargoPrefab, m_aCargoAmmo, m_aCargoCount);
		m_iCargoPage = 0;
		PaintRows("CargoHost", m_aCargoPrefab, m_aCargoCount, m_aCargoAmmo, m_iCargoPage, m_iCargo, "Cargo");
		if (m_aCargoPrefab.IsEmpty())
			PaintEmptyTrunk();

		int total = 0;
		foreach (int count : m_aCargoCount)
			total = total + count;

		if (message)
			message.SetText("Coffre : " + total.ToString() + " objet(s)");

		AskView();
	}

	protected void PaintEmptyTrunk()
	{
		Widget host = m_wRoot.FindAnyWidget("CargoHost");
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!host || !workspace)
			return;

		Widget row = workspace.CreateWidgets(ROW, host);
		if (!row)
			return;

		m_aRows.Insert(row);
		row.SetName("TrunkEmpty");
		SizeRow(row, 4, 0, m_fCargoRowW, m_fCargoRowStep);
		TextWidget label = TextWidget.Cast(row.FindAnyWidget("Label"));
		if (label)
			label.SetText("Coffre vide");

		Widget preview = row.FindAnyWidget("Preview");
		if (preview)
			preview.SetVisible(false);
	}

	protected void SizeRow(Widget row, float x, float y, float width, float step)
	{
		if (width < 120)
			width = 120;

		float rowH = step - 4;
		if (rowH < 20)
			rowH = 20;

		FrameSlot.SetPos(row, x, y);
		FrameSlot.SetSize(row, width, rowH);
		row.SetColor(new Color(1, 1, 1, 0));

		float previewSize = 84;
		if (previewSize > rowH - 8)
			previewSize = rowH - 8;

		Widget preview = row.FindAnyWidget("Preview");
		if (preview)
		{
			FrameSlot.SetAnchorMin(preview, 0, 0);
			FrameSlot.SetAnchorMax(preview, 0, 0);
			FrameSlot.SetPos(preview, 6, 6);
			FrameSlot.SetSize(preview, previewSize, previewSize);
		}

		TextWidget label = TextWidget.Cast(row.FindAnyWidget("Label"));
		if (!label)
			return;

		float labelX = previewSize + 16;
		float labelW = width - labelX - 8;
		if (labelW < 32)
			labelW = 32;

		FrameSlot.SetPos(label, labelX, 4);
		FrameSlot.SetSize(label, labelW, rowH - 8);
		label.SetColor(new Color(1, 1, 0.85, 1));
	}

	protected void MarkRow(Widget row, bool selected)
	{
		Widget hit = row.FindAnyWidget("Hit");
		if (!hit)
			return;

		if (selected)
			hit.SetColor(new Color(0.118, 0.227, 0.373, 1));
		else
			hit.SetColor(new Color(0.05, 0.08, 0.14, 0.35));
	}

	protected void MarkFilterRow(Widget row, bool selected, bool shown)
	{
		Widget hit = row.FindAnyWidget("Hit");
		if (!hit)
			return;

		if (shown)
		{
			if (selected)
				hit.SetColor(new Color(0.118, 0.227, 0.373, 1));
			else
				hit.SetColor(new Color(0.05, 0.08, 0.14, 0.55));

			return;
		}

		if (selected)
			hit.SetColor(new Color(0.55, 0.16, 0.16, 1));
		else
			hit.SetColor(new Color(0.28, 0.08, 0.08, 0.7));
	}

	protected void PrepareRow(Widget row)
	{
		row.AddHandler(this);
		Widget hit = row.FindAnyWidget("Hit");
		if (hit)
			hit.AddHandler(this);

		Widget child = row.GetChildren();
		while (child)
		{
			if (child.GetName() != "Hit")
				child.SetFlags(WidgetFlags.IGNORE_CURSOR);

			child = child.GetSibling();
		}
	}

	protected void ShowPreview(Widget widget, string prefab)
	{
		if (!widget)
			return;

		ItemPreviewWidget preview = ItemPreviewWidget.Cast(widget);
		if (!preview)
			preview = ItemPreviewWidget.Cast(widget.FindAnyWidget("Preview"));

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

	void ReleaseTrunk()
	{
		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.CloseMenuByPreset(ChimeraMenuPreset.Inventory20Menu);
	}

	protected void AskView()
	{
		IEntity vehicle = CurrentVehicle();
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player || !m_Post)
			return;

		player.HAM_DepotRefresh(HAM_RplUtil.IdOf(m_Post), HAM_RplUtil.IdOf(vehicle));
	}

	protected void RefreshVehicles()
	{
		m_aVehicles.Clear();
		if (m_Post)
			HAM_VehicleDepot.Collect(m_Post.GetOrigin(), m_aVehicles);

		if (m_iVehicle >= m_aVehicles.Count())
			m_iVehicle = 0;

		TextWidget name = TextWidget.Cast(m_wRoot.FindAnyWidget("VehName"));
		if (!name)
			return;

		if (m_aVehicles.IsEmpty())
		{
			name.SetText("Aucun véhicule à " + Math.Floor(HAM_VehicleDepot.Radius()).ToString() + " m");
			ShowPreview(m_wRoot.FindAnyWidget("VehPreview"), "");
			return;
		}

		name.SetText(NiceName(HAM_VehicleDepotPrefab(m_aVehicles[m_iVehicle])));
		ShowPreview(m_wRoot.FindAnyWidget("VehPreview"), HAM_VehicleDepotPrefab(m_aVehicles[m_iVehicle]));
	}

	protected void FillFromPrefabs()
	{
		array<string> prefabs = {};
		HAM_ArsenalCatalog.CollectPrefabs(prefabs);
		foreach (string prefab : prefabs)
		{
			if (prefab.IsEmpty())
				continue;

			if (!HAM_ArsenalBoardStore.IsShown(prefab))
				continue;

			bool shown = HAM_VehicleDepotFilter.IsShown(prefab, true);
			if (!m_bFilter && !shown)
				continue;

			if (m_iCategory >= 0 && HAM_ArsenalCatalog.CategoryOfPrefab(prefab) != m_iCategory)
				continue;

			string label = DisplayName(prefab);
			if (m_bFilter)
			{
				if (shown)
					label = label + "    Inclus";
				else
					label = label + "    Exclus";
			}

			if (!SearchHit(label))
				continue;

			m_aItemPrefab.Insert(prefab);
			m_aItemLabel.Insert(label);
			int cost = 0;
			if (HAM_ArsenalBoardStore.HasCostOverride(prefab))
				cost = HAM_ArsenalBoardStore.BoardPrice(prefab);

			m_aItemCost.Insert(cost);
			if (shown)
				m_aItemShown.Insert(1);
			else
				m_aItemShown.Insert(0);
		}
	}

	protected void CollectUsArsenalItems(out array<SCR_ArsenalItem> items)
	{
		items.Clear();
		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalog)
			return;

		// GetArsenalItems = catalogue global seulement (souvent « perso »).
		// GetAllArsenalItems = toutes les factions + global (armes, mods, etc.).
		catalog.GetAllArsenalItems(items, -1, -1, -1, -1);
	}

	protected void RebuildItems()
	{
		m_aItemPrefab.Clear();
		m_aItemLabel.Clear();
		m_aItemCost.Clear();
		m_aItemShown.Clear();

		array<SCR_ArsenalItem> items = {};
		CollectUsArsenalItems(items);
		if (items.IsEmpty())
		{
			FillFromPrefabs();
		}
		else
		{
		foreach (SCR_ArsenalItem item : items)
		{
			if (!item)
				continue;

			string prefab = item.GetItemResourceName();
			if (!HAM_ArsenalBoardStore.IsShown(prefab))
				continue;

			bool shown = HAM_VehicleDepotFilter.IsShown(prefab, Kept(item));
			if (!m_bFilter && !shown)
				continue;

			if (m_iCategory >= 0 && CategoryOf(item) != m_iCategory)
				continue;

			string label = DisplayName(prefab);
			if (m_bFilter)
			{
				if (shown)
					label = label + "    Inclus";
				else
					label = label + "    Exclus";
			}
			if (!SearchHit(label))
				continue;

			int cost = item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false);
			if (HAM_ArsenalBoardStore.HasCostOverride(prefab))
				cost = HAM_ArsenalBoardStore.BoardPrice(prefab);

			m_aItemPrefab.Insert(prefab);
			m_aItemLabel.Insert(label);
			m_aItemCost.Insert(cost);
			if (shown)
				m_aItemShown.Insert(1);
			else
				m_aItemShown.Insert(0);
		}
		}

		if (m_iItem >= m_aItemPrefab.Count())
			m_iItem = 0;

		PaintItemRows();
		PaintCategories();
		TextWidget message = TextWidget.Cast(m_wRoot.FindAnyWidget("Message"));
		if (!message)
			return;

		if (m_bFilter && !m_sFilterNote.IsEmpty())
			message.SetText(m_sFilterNote);
		else if (m_bFilter)
			message.SetText("Liste du tableau. Vert = au garage. Rouge = retiré du garage seulement. Objets : " + m_aItemPrefab.Count().ToString());
		else
			message.SetText("Objets : " + m_aItemPrefab.Count().ToString());
	}

	protected void PaintItemRows()
	{
		ClearHost("ItemHost");
		Widget host = m_wRoot.FindAnyWidget("ItemHost");
		if (!host)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		int start = m_iItemPage * m_iRows;
		for (int i = 0; i < m_iRows; i++)
		{
			int index = start + i;
			if (index >= m_aItemPrefab.Count())
				break;

			Widget row = workspace.CreateWidgets(ROW, host);
			if (!row)
				continue;

			m_aRows.Insert(row);
			row.SetName("Item" + i.ToString());
			SizeRow(row, 4, i * m_fRowStep, m_fRowW, m_fRowStep);
			TextWidget label = TextWidget.Cast(row.FindAnyWidget("Label"));
			if (label)
				label.SetText(m_aItemLabel[index] + "    " + m_aItemCost[index].ToString());

			ShowPreview(row, m_aItemPrefab[index]);
			if (m_bFilter)
			{
				bool shown = true;
				if (index < m_aItemShown.Count() && m_aItemShown[index] == 0)
					shown = false;

				MarkFilterRow(row, index == m_iItem, shown);
			}
			else
			{
				MarkRow(row, index == m_iItem);
			}

			PrepareRow(row);
		}
	}

	protected void PaintRows(string hostName, array<string> prefabs, array<int> counts, array<int> ammos, int page, int selected, string prefix)
	{
		ClearHost(hostName);
		Widget host = m_wRoot.FindAnyWidget(hostName);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!host || !workspace)
			return;

		float rowW = m_fRowW;
		float rowStep = m_fRowStep;
		if (hostName == "CargoHost")
		{
			rowW = m_fCargoRowW;
			rowStep = m_fCargoRowStep;
		}

		int start = page * m_iRows;
		for (int i = 0; i < m_iRows; i++)
		{
			int index = start + i;
			if (index >= prefabs.Count())
				break;

			Widget row = workspace.CreateWidgets(ROW, host);
			if (!row)
				continue;

			m_aRows.Insert(row);
			row.SetName(prefix + i.ToString());
			SizeRow(row, 4, i * rowStep, rowW, rowStep);
			TextWidget label = TextWidget.Cast(row.FindAnyWidget("Label"));
			if (label)
			{
				string text = counts[index].ToString() + " x " + DisplayName(prefabs[index]);
				if (ammos[index] >= 0)
					text = text + " [" + ammos[index].ToString() + "]";

				label.SetText(text);
			}

			ShowPreview(row, prefabs[index]);
			MarkRow(row, index == selected);

			PrepareRow(row);
		}
	}

	protected void ClearHost(string hostName)
	{
		for (int i = m_aRows.Count() - 1; i >= 0; i--)
		{
			Widget row = m_aRows[i];
			if (!row)
			{
				m_aRows.Remove(i);
				continue;
			}

			Widget parent = row.GetParent();
			if (!parent || parent.GetName() != hostName)
				continue;

			row.RemoveFromHierarchy();
			m_aRows.Remove(i);
		}
	}

	protected void PaintCategories()
	{
		Widget all = m_wRoot.FindAnyWidget("CatAll");
		if (all)
		{
			if (m_iCategory < 0)
				all.SetColor(new Color(0.118, 0.227, 0.373, 1));
			else
				all.SetColor(new Color(0.071, 0.102, 0.184, 0.85));
		}

		for (int i = 0; i < 6; i++)
		{
			Widget button = m_wRoot.FindAnyWidget("Cat" + i.ToString());
			if (!button)
				continue;

			if (i == m_iCategory)
				button.SetColor(new Color(0.118, 0.227, 0.373, 1));
			else
				button.SetColor(new Color(0.071, 0.102, 0.184, 0.85));
		}
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

	protected int CategoryOf(SCR_ArsenalItem item)
	{
		if (!item)
			return 5;

		return HAM_ArsenalCatalog.CategoryOfPrefab(item.GetItemResourceName());
	}

	protected bool Kept(SCR_ArsenalItem item)
	{
		// Training : aucun type exclu par défaut (tenue, armes, gadgets…).
		return true;
	}

	protected IEntity CurrentVehicle()
	{
		if (m_iVehicle < 0 || m_iVehicle >= m_aVehicles.Count())
			return null;

		return m_aVehicles[m_iVehicle];
	}

	protected string CurrentItem()
	{
		if (m_iItem < 0 || m_iItem >= m_aItemPrefab.Count())
			return "";

		return m_aItemPrefab[m_iItem];
	}

	protected string CurrentPreset()
	{
		if (m_iPreset < 0 || m_iPreset >= m_aPresetNames.Count())
			return "";

		return m_aPresetNames[m_iPreset];
	}

	protected string EditText(string name)
	{
		EditBoxWidget box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (!box)
			return "";

		return box.GetText();
	}

	protected void LayoutScreen()
	{
		float screenW = 1920;
		float screenH = 1080;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.GetScreenSize(screenW, screenH);

		float sw = 1920;
		float sh = 1080;
		if (screenW > 100 && screenH > 100)
		{
			float fit = screenH / 1080;
			float fitW = screenW / 1920;
			if (fitW < fit)
				fit = fitW;

			if (fit > 1)
				fit = 1;

			sw = 1920 * fit;
			sh = 1080 * fit;
		}

		if (sw > screenW)
			sw = screenW;

		if (sh > screenH)
			sh = screenH;

		float margin = 12;
		BlurWidget blur = BlurWidget.Cast(m_wRoot.FindAnyWidget("Backdrop"));
		if (blur)
			blur.SetIntensity(0.35);

		Place("Backdrop", 0, 0, sw, sh);
		Place("Panel", margin, margin, sw - margin * 2, sh - margin * 2);
		Widget panel = m_wRoot.FindAnyWidget("Panel");
		if (panel)
			panel.SetColor(new Color(0.027, 0.051, 0.094, 0.96));
		// Enfants de Panel : origine locale. Le décalage écran est déjà sur Panel.
		float panelX = 0;
		float panelY = 0;
		float panelW = sw - margin * 2;
		float panelH = sh - margin * 2;
		float headerH = 72;
		float gap = 12;
		float colY = panelY + headerH;
		float colH = panelH - headerH - 16;
		float leftW = panelW * 0.24;
		float rightW = panelW * 0.28;
		float centerW = panelW - leftW - rightW - gap * 4;
		float leftX2 = panelX + gap;
		float centerX = leftX2 + leftW + gap;
		float rightX2 = centerX + centerW + gap;
		Place("ColL", leftX2, colY, leftW, colH);
		Place("ColLIn", 0, 0, leftW, colH);
		Place("ColM", centerX, colY, centerW, colH);
		Place("ColMIn", 0, 0, centerW, colH);
		Place("ColR", rightX2, colY, rightW, colH);
		Place("ColRIn", 0, 0, rightW, colH);
		Tint("ColL", 0.045, 0.07, 0.12);
		Tint("ColM", 0.045, 0.07, 0.12);
		Tint("ColR", 0.045, 0.07, 0.12);

		float btnGap = 8;
		float btnY = 10;
		float btnH = 36;
		float closeW = 140;
		float persistW = 290;
		float confirmW = 230;
		float supplyW = 210;
		float closeX = panelW - 8 - closeW;
		float persistX = closeX - btnGap - persistW;
		float confirmX = persistX - btnGap - confirmW;
		float supplyX = confirmX - btnGap - supplyW;
		float titleW = supplyX - margin - btnGap;
		if (titleW > 420)
			titleW = 420;
		if (titleW < 160)
			titleW = 160;

		Place("Title", panelX + 16, panelY + 10, titleW, 34);
		float messageX = panelX + 16 + titleW + btnGap;
		float messageW = supplyX - messageX - btnGap;
		if (messageW < 40)
			messageW = 0;

		Place("Message", messageX, panelY + 16, messageW, 24);
		Place("Supplies", supplyX, btnY, supplyW, btnH);
		Place("ConfirmCargo", confirmX, btnY, confirmW, btnH);
		Place("Persist", persistX, btnY, persistW, btnH);
		Place("Close", closeX, btnY, closeW, btnH);
		Tint("Close", 0.55, 0.16, 0.16);
		Tint("ConfirmCargo", 0.118, 0.227, 0.373);
		Tint("Supplies", 0.071, 0.102, 0.184);
		Tint("Persist", 0.071, 0.102, 0.184);

		float pad = 16;
		Place("VehName", leftX2 + pad, colY + 16, leftW - 32, 36);
		Place("VehPreview", leftX2 + pad, colY + 60, leftW - 32, 120);
		Place("VehPrev", leftX2 + pad, colY + 196, 56, 40);
		Place("VehNext", leftX2 + pad + 64, colY + 196, 56, 40);
		float qtyW = (leftW - 48) / 3;
		float qtyX = leftX2 + pad;
		float qtyTop = colY + 260;
		Place("Add1", qtyX, qtyTop, qtyW, 40);
		Place("Add5", qtyX + qtyW + 8, qtyTop, qtyW, 40);
		Place("Add10", qtyX + (qtyW + 8) * 2, qtyTop, qtyW, 40);
		Place("Rem1", qtyX, qtyTop + 48, qtyW, 40);
		Place("Rem5", qtyX + qtyW + 8, qtyTop + 48, qtyW, 40);
		Place("Rem10", qtyX + (qtyW + 8) * 2, qtyTop + 48, qtyW, 40);
		Tint("Add1", 0.118, 0.227, 0.373);
		Tint("Add5", 0.118, 0.227, 0.373);
		Tint("Add10", 0.118, 0.227, 0.373);
		Tint("Rem1", 0.28, 0.1, 0.1);
		Tint("Rem5", 0.28, 0.1, 0.1);
		Tint("Rem10", 0.28, 0.1, 0.1);
		Place("ClearCargo", leftX2 + pad, colY + colH - 110, leftW - 32, 44);
		Place("OpenVanilla", leftX2 + pad, colY + colH - 58, leftW - 32, 48);

		float catY = colY + 12;
		float catW = (centerW - 32) / 7;
		Place("CatAll", centerX + pad, catY, catW - 4, 32);
		Place("Cat0", centerX + pad + catW, catY, catW - 4, 32);
		Place("Cat1", centerX + pad + catW * 2, catY, catW - 4, 32);
		Place("Cat2", centerX + pad + catW * 3, catY, catW - 4, 32);
		Place("Cat3", centerX + pad + catW * 4, catY, catW - 4, 32);
		Place("Cat4", centerX + pad + catW * 5, catY, catW - 4, 32);
		Place("Cat5", centerX + pad + catW * 6, catY, catW - 4, 32);
		Place("SearchBg", centerX + pad, catY + 42, centerW - 160, 36);
		Place("Search", centerX + pad, catY + 42, centerW - 160, 36);
		Place("Chercher", centerX + centerW - 136, catY + 42, 120, 36);
		Widget depotSearchBg = m_wRoot.FindAnyWidget("SearchBg");
		if (depotSearchBg)
		{
			depotSearchBg.SetColor(new Color(0.16, 0.22, 0.34, 1));
			depotSearchBg.SetFlags(WidgetFlags.IGNORE_CURSOR);
			depotSearchBg.SetZOrder(20);
		}
		Widget depotSearch = m_wRoot.FindAnyWidget("Search");
		if (depotSearch)
		{
			depotSearch.SetColor(new Color(0.95, 0.96, 0.97, 1));
			depotSearch.SetZOrder(21);
			depotSearch.AddHandler(this);
		}
		Widget presetEdit = m_wRoot.FindAnyWidget("PresetEdit");
		if (presetEdit)
			presetEdit.SetZOrder(21);

		float listY = catY + 88;
		float pageY = colY + colH - 40;
		float listH = pageY - listY - 8;
		Place("ItemHost", centerX + pad, listY, centerW - 32, listH);
		m_fRowW = centerW - 48;
		m_iRows = 8;
		m_fRowStep = listH / 8;
		if (m_fRowStep < 36)
			m_fRowStep = 36;

		Place("ItemPrev", centerX + pad, pageY, 110, 32);
		Place("ItemNext", centerX + pad + 118, pageY, 110, 32);

		Place("PresetEdit", rightX2 + pad, colY + 12, rightW - 250, 32);
		Place("PresetSave", rightX2 + rightW - 226, colY + 12, 108, 32);
		Place("PresetDelete", rightX2 + rightW - 112, colY + 12, 96, 32);
		Tint("PresetSave", 0.118, 0.227, 0.373);
		Tint("PresetDelete", 0.55, 0.16, 0.16);
		Place("PresetPrev", rightX2 + pad, colY + 52, 36, 32);
		Place("PresetName", rightX2 + 58, colY + 52, rightW - 120, 32);
		Place("PresetNext", rightX2 + rightW - 52, colY + 52, 36, 32);
		Place("PresetApply", rightX2 + pad, colY + 96, rightW - 32, 40);
		Tint("PresetApply", 0.118, 0.227, 0.373);
		Place("CargoPrev", rightX2 + pad, colY + 146, 80, 28);
		Place("CargoNext", rightX2 + 100, colY + 146, 80, 28);
		Place("CargoPlus", rightX2 + 190, colY + 146, 64, 28);
		Place("CargoMinus", rightX2 + 260, colY + 146, 64, 28);
		float cargoY = colY + 184;
		float cargoW = rightW - 32;
		float cargoH = colY + colH - cargoY - 12;
		Place("CargoHost", rightX2 + pad, cargoY, cargoW, cargoH);
		m_fCargoRowW = cargoW - 8;
		m_fCargoRowStep = cargoH / 8;
	}

	protected void Tint(string name, float r, float g, float b)
	{
		Widget widget = m_wRoot.FindAnyWidget(name);
		if (widget)
			widget.SetColor(new Color(r, g, b, 1));
	}

	protected void Place(string name, float x, float y, float width, float height)
	{
		if (!m_wRoot)
			return;

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

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (FocusEdit(w))
			return false;

		return HandlePress(w);
	}

	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box || box.GetName() != "Search")
			return false;

		m_sSearch = box.GetText();
		m_iItemPage = 0;
		RebuildItems();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(box);
		return false;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (FocusEdit(w))
			return false;

		return HandlePress(w);
	}

	protected bool FocusEdit(Widget w)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		Widget cursor = w;
		while (cursor && !box)
		{
			box = EditBoxWidget.Cast(cursor);
			cursor = cursor.GetParent();
		}

		if (!box && w && w.GetName() == "SearchBg")
			box = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Search"));

		if (!box)
			return false;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
			workspace.SetFocusedWidget(box);

		return true;
	}

	protected bool HandlePress(Widget w)
	{
		string name = ButtonName(w);
		if (name.IsEmpty() || name == "Search" || name == "SearchBg" || name == "PresetEdit" || name == "ItemHost" || name == "CargoHost")
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextClick)
			return true;

		m_fNextClick = now + 250;
		if (name == "Close")
		{
			HAM_VehicleDepotBoard.Close();
			return true;
		}

		if (name == "Supplies")
		{
			ShowSupplies();
			return true;
		}

		if (name == "Chercher")
		{
			m_sSearch = EditText("Search");
			m_iItemPage = 0;
			RebuildItems();
			return true;
		}

		if (name == "VehPrev")
		{
			ShiftVehicle(-1);
			return true;
		}

		if (name == "VehNext")
		{
			ShiftVehicle(1);
			return true;
		}

		if (name == "ItemPrev")
		{
			if (m_iItemPage > 0)
				m_iItemPage = m_iItemPage - 1;
			PaintItemRows();
			return true;
		}

		if (name == "ItemNext")
		{
			int last = 0;
			if (m_aItemPrefab.Count() > 0)
				last = (m_aItemPrefab.Count() - 1) / m_iRows;

			if (m_iItemPage < last)
				m_iItemPage = m_iItemPage + 1;

			PaintItemRows();
			return true;
		}

		if (name == "CargoPrev")
		{
			if (m_iCargoPage > 0)
				m_iCargoPage = m_iCargoPage - 1;
			ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
			return true;
		}

		if (name == "CargoNext")
		{
			int last = 0;
			if (m_aCargoPrefab.Count() > 0)
				last = (m_aCargoPrefab.Count() - 1) / m_iRows;

			if (m_iCargoPage < last)
				m_iCargoPage = m_iCargoPage + 1;

			ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
			return true;
		}

		if (name == "PresetPrev")
		{
			if (m_iPreset > 0)
				m_iPreset = m_iPreset - 1;
			ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
			return true;
		}

		if (name == "PresetNext")
		{
			if (m_iPreset + 1 < m_aPresetNames.Count())
				m_iPreset = m_iPreset + 1;
			ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
			return true;
		}

		if (name == "CatAll")
		{
			m_iCategory = -1;
			m_iItemPage = 0;
			m_iItem = 0;
			RebuildItems();
			return true;
		}

		if (name.IndexOf("Cat") == 0)
		{
			m_iCategory = name.Substring(3, 1).ToInt();
			m_iItemPage = 0;
			m_iItem = 0;
			RebuildItems();
			return true;
		}

		if (name.IndexOf("Item") == 0)
		{
			int local = name.Substring(4, name.Length() - 4).ToInt();
			m_iItem = m_iItemPage * m_iRows + local;
			m_bCargoPick = false;
			PaintItemRows();
			return true;
		}

		if (name.IndexOf("Cargo") == 0)
		{
			bool cargoRow = true;
			if (name == "CargoPlus")
				cargoRow = false;
			if (name == "CargoMinus")
				cargoRow = false;
			if (name == "CargoHost")
				cargoRow = false;

			if (cargoRow)
			{
				int local = name.Substring(5, name.Length() - 5).ToInt();
				m_iCargo = m_iCargoPage * m_iRows + local;
				m_bCargoPick = true;
				ShowServer(HAM_VehicleDepotBoard.Cargo(), HAM_VehicleDepotBoard.Presets(), HAM_VehicleDepotBoard.Persistent());
				return true;
			}
		}

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player || !m_Post)
			return true;

		RplId depotId = HAM_RplUtil.IdOf(m_Post);
		RplId vehicleId = HAM_RplUtil.IdOf(CurrentVehicle());
		if (name == "Add1")
			player.HAM_DepotAdd(depotId, vehicleId, CurrentItem(), 1, -1);
		else if (name == "Add5")
			player.HAM_DepotAdd(depotId, vehicleId, CurrentItem(), 5, -1);
		else if (name == "Add10")
			player.HAM_DepotAdd(depotId, vehicleId, CurrentItem(), 10, -1);
		else if (name == "Rem1")
			AskRemove(player, depotId, vehicleId, 1);
		else if (name == "Rem5")
			AskRemove(player, depotId, vehicleId, 5);
		else if (name == "Rem10")
			AskRemove(player, depotId, vehicleId, 10);
		else if (name == "CargoPlus")
			AskTrunk(player, depotId, vehicleId, true);
		else if (name == "CargoMinus")
			AskTrunk(player, depotId, vehicleId, false);
		else if (name == "ClearCargo")
			player.HAM_DepotClear(depotId, vehicleId);
		else if (name == "OpenVanilla")
		{
			if (m_bFilter)
				ToggleFilterItem();
			else
				ShowTrunk();
		}
		else if (name == "Persist")
			player.HAM_DepotPersistent(vehicleId, HAM_VehicleDepotBoard.Persistent() == 0);
		else if (name == "ConfirmCargo")
			player.HAM_DepotConfirm(depotId, vehicleId);
		else if (name == "PresetSave")
			player.HAM_DepotPresetSave(vehicleId, EditText("PresetEdit"));
		else if (name == "PresetDelete")
			player.HAM_DepotPresetDelete(vehicleId, CurrentPreset());
		else if (name == "PresetApply")
			player.HAM_DepotPresetApply(depotId, vehicleId, CurrentPreset());

		return true;
	}

	protected void ToggleFilterItem()
	{
		string prefab = CurrentItem();
		if (prefab.IsEmpty())
			return;

		bool shown = true;
		array<SCR_ArsenalItem> items = {};
		CollectUsArsenalItems(items);
		foreach (SCR_ArsenalItem item : items)
		{
			if (!item)
				continue;

			if (item.GetItemResourceName() != prefab)
				continue;

			shown = HAM_VehicleDepotFilter.IsShown(prefab, Kept(item));
			break;
		}

		bool next = !shown;
		HAM_VehicleDepotFilter.SetShown(prefab, next);
		if (next)
			m_sFilterNote = "Inclus : " + DisplayName(prefab);
		else
			m_sFilterNote = "Exclus : " + DisplayName(prefab);

		RebuildItems();
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_DepotFilterSet(prefab, next);
	}

	protected void AskTrunk(SCR_PlayerController player, RplId depotId, RplId vehicleId, bool add)
	{
		if (m_iCargo < 0 || m_iCargo >= m_aCargoPrefab.Count())
			return;

		string prefab = m_aCargoPrefab[m_iCargo];
		if (prefab.IsEmpty())
			return;

		int ammo = -1;
		if (m_iCargo < m_aCargoAmmo.Count())
			ammo = m_aCargoAmmo[m_iCargo];

		if (add)
		{
			player.HAM_DepotAdd(depotId, vehicleId, prefab, 1, ammo);
			return;
		}

		player.HAM_DepotRemove(depotId, vehicleId, prefab, ammo, 1);
	}

	protected void AskRemove(SCR_PlayerController player, RplId depotId, RplId vehicleId, int count)
	{
		string prefab = CurrentItem();
		int ammo = -1;
		if (m_bCargoPick && m_iCargo >= 0 && m_iCargo < m_aCargoPrefab.Count())
		{
			prefab = m_aCargoPrefab[m_iCargo];
			ammo = m_aCargoAmmo[m_iCargo];
		}

		player.HAM_DepotRemove(depotId, vehicleId, prefab, ammo, count);
	}

	protected void ShiftVehicle(int step)
	{
		if (m_aVehicles.IsEmpty())
			return;

		m_iVehicle = m_iVehicle + step;
		if (m_iVehicle < 0)
			m_iVehicle = m_aVehicles.Count() - 1;
		if (m_iVehicle >= m_aVehicles.Count())
			m_iVehicle = 0;

		RefreshVehicles();
		AskView();
	}

	protected string ButtonName(Widget w)
	{
		Widget cursor = w;
		while (cursor)
		{
			string name = cursor.GetName();
			if (name == "Search" || name == "SearchBg" || name == "PresetEdit")
				return name;

			if (!name.IsEmpty() && name != "Root" && name != "Panel" && name != "Label" && name != "Row" && name != "Hit" && name != "Preview")
				return name;

			cursor = cursor.GetParent();
		}

		return "";
	}
}

string HAM_VehicleDepotPrefab(IEntity ent)
{
	if (!ent || !ent.GetPrefabData())
		return "";

	return ent.GetPrefabData().GetPrefabName();
}

string NiceName(string prefab)
{
	string rest = prefab;
	int cut = rest.IndexOf("/");
	while (cut >= 0)
	{
		rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
		cut = rest.IndexOf("/");
	}

	int dot = rest.IndexOf(".");
	if (dot > 0)
		rest = rest.Substring(0, dot);

	return rest;
}

string DisplayName(string prefab)
{
	return HAM_ArsenalCatalog.ItemLabel(prefab);
}

modded class SCR_PlayerController
{
	void HAM_DepotMirror(RplId depotId)
	{
		if (Replication.IsServer())
			HAM_VehicleDepot.MirrorSupplies(HAM_DepotEntity(depotId));
		else
			Rpc(HAM_RpcDepotMirror, depotId);
	}

	void HAM_DepotRefresh(RplId depotId, RplId vehicleId)
	{
		if (Replication.IsServer())
			HAM_DepotRefreshServer(depotId, vehicleId);
		else
			Rpc(HAM_RpcDepotRefresh, depotId, vehicleId);
	}

	void HAM_DepotAdd(RplId depotId, RplId vehicleId, string prefab, int count, int ammo)
	{
		if (Replication.IsServer())
			HAM_DepotAddServer(depotId, vehicleId, prefab, count, ammo);
		else
			Rpc(HAM_RpcDepotAdd, depotId, vehicleId, prefab, count, ammo);
	}

	void HAM_DepotRemove(RplId depotId, RplId vehicleId, string prefab, int ammo, int count)
	{
		if (Replication.IsServer())
			HAM_DepotRemoveServer(depotId, vehicleId, prefab, ammo, count);
		else
			Rpc(HAM_RpcDepotRemove, depotId, vehicleId, prefab, ammo, count);
	}

	void HAM_DepotClear(RplId depotId, RplId vehicleId)
	{
		if (Replication.IsServer())
			HAM_DepotClearServer(depotId, vehicleId);
		else
			Rpc(HAM_RpcDepotClear, depotId, vehicleId);
	}

	void HAM_DepotPresetSave(RplId vehicleId, string name)
	{
		if (Replication.IsServer())
			HAM_DepotPresetSaveServer(vehicleId, name);
		else
			Rpc(HAM_RpcDepotPresetSave, vehicleId, name);
	}

	void HAM_DepotPresetDelete(RplId vehicleId, string name)
	{
		if (Replication.IsServer())
			HAM_DepotPresetDeleteServer(vehicleId, name);
		else
			Rpc(HAM_RpcDepotPresetDelete, vehicleId, name);
	}

	void HAM_DepotPresetApply(RplId depotId, RplId vehicleId, string name)
	{
		if (Replication.IsServer())
			HAM_DepotPresetApplyServer(depotId, vehicleId, name);
		else
			Rpc(HAM_RpcDepotPresetApply, depotId, vehicleId, name);
	}

	// « Valider le coffre » : seule écriture de la cargaison, sur action explicite du joueur.
	void HAM_DepotConfirm(RplId depotId, RplId vehicleId)
	{
		if (Replication.IsServer())
			HAM_DepotConfirmServer(depotId, vehicleId);
		else
			Rpc(HAM_RpcDepotConfirm, depotId, vehicleId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotConfirm(RplId depotId, RplId vehicleId)
	{
		HAM_DepotConfirmServer(depotId, vehicleId);
	}

	protected void HAM_DepotConfirmServer(RplId depotId, RplId vehicleId)
	{
		HAM_VehicleDepot.ConfirmCargo(GetPlayerId(), HAM_DepotEntity(depotId), HAM_DepotEntity(vehicleId));
	}

	void HAM_DepotPersistent(RplId vehicleId, bool enabled)
	{
		if (Replication.IsServer())
			HAM_DepotPersistentServer(vehicleId, enabled);
		else
			Rpc(HAM_RpcDepotPersistent, vehicleId, enabled);
	}

	void HAM_DepotFilterAsk()
	{
		if (Replication.IsServer())
			HAM_DepotFilterAskServer();
		else
			Rpc(HAM_RpcDepotFilterAsk);
	}

	void HAM_DepotFilterSet(string prefab, bool shown)
	{
		if (Replication.IsServer())
			HAM_DepotFilterSetServer(prefab, shown);
		else
			Rpc(HAM_RpcDepotFilterSet, prefab, shown);
	}

	void HAM_DepotFilterPush(string pack, bool reset, bool done)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_VehicleDepotBoard.OnFilterPack(pack, reset, done);

		Rpc(HAM_RpcDepotFilterPack, pack, reset, done);
	}

	protected void HAM_DepotFilterPushAll()
	{
		array<string> chunks = {};
		HAM_VehicleDepotFilter.PackChunks(chunks);
		if (chunks.IsEmpty())
		{
			HAM_DepotFilterPush("", true, true);
			return;
		}

		int last = chunks.Count() - 1;
		for (int i = 0; i < chunks.Count(); i++)
		{
			bool reset = false;
			if (i == 0)
				reset = true;

			bool done = false;
			if (i == last)
				done = true;

			HAM_DepotFilterPush(chunks[i], reset, done);
		}
	}

	void HAM_DepotSuppliesAsk()
	{
		RplId depotId = RplId.Invalid();
		IEntity post = HAM_VehicleDepotMenu.GetPost();
		if (post)
		{
			RplComponent replication = RplComponent.Cast(post.FindComponent(RplComponent));
			if (replication)
				depotId = replication.Id();
		}

		if (Replication.IsServer())
			HAM_RpcDepotSuppliesAsk(depotId);
		else
			Rpc(HAM_RpcDepotSuppliesAsk, depotId);
	}

	void HAM_DepotSuppliesPush(int amount)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_VehicleDepotBoard.OnSupplies(amount);

		Rpc(HAM_RpcDepotSupplies, amount);
	}

	void HAM_DepotView(string cargo, string presets, int persistent, int supplies)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_VehicleDepotBoard.ApplyView(cargo, presets, persistent, supplies);

		Rpc(HAM_RpcDepotView, cargo, presets, persistent, supplies);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotFilterAsk()
	{
		HAM_DepotFilterAskServer();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotFilterSet(string prefab, bool shown)
	{
		HAM_DepotFilterSetServer(prefab, shown);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcDepotFilterPack(string pack, bool reset, bool done)
	{
		HAM_VehicleDepotBoard.OnFilterPack(pack, reset, done);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotRefresh(RplId depotId, RplId vehicleId)
	{
		HAM_DepotRefreshServer(depotId, vehicleId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotMirror(RplId depotId)
	{
		HAM_VehicleDepot.MirrorSupplies(HAM_DepotEntity(depotId));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotAdd(RplId depotId, RplId vehicleId, string prefab, int count, int ammo)
	{
		HAM_DepotAddServer(depotId, vehicleId, prefab, count, ammo);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotRemove(RplId depotId, RplId vehicleId, string prefab, int ammo, int count)
	{
		HAM_DepotRemoveServer(depotId, vehicleId, prefab, ammo, count);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotClear(RplId depotId, RplId vehicleId)
	{
		HAM_DepotClearServer(depotId, vehicleId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotPresetSave(RplId vehicleId, string name)
	{
		HAM_DepotPresetSaveServer(vehicleId, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotPresetDelete(RplId vehicleId, string name)
	{
		HAM_DepotPresetDeleteServer(vehicleId, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotPresetApply(RplId depotId, RplId vehicleId, string name)
	{
		HAM_DepotPresetApplyServer(depotId, vehicleId, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotPersistent(RplId vehicleId, bool enabled)
	{
		HAM_DepotPersistentServer(vehicleId, enabled);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDepotSuppliesAsk(RplId depotId)
	{
		vector pos = vector.Zero;
		IEntity depot = HAM_DepotEntity(depotId);
		if (depot)
			pos = depot.GetOrigin();

		HAM_DepotSuppliesPush(HAM_VehicleDepot.HomeSupplies(pos));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcDepotSupplies(int amount)
	{
		HAM_VehicleDepotBoard.OnSupplies(amount);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcDepotView(string cargo, string presets, int persistent, int supplies)
	{
		HAM_VehicleDepotBoard.ApplyView(cargo, presets, persistent, supplies);
	}

	protected void HAM_DepotRefreshServer(RplId depotId, RplId vehicleId)
	{
		HAM_VehicleDepot.PushView(GetPlayerId(), HAM_DepotEntity(vehicleId));
	}

	protected void HAM_DepotAddServer(RplId depotId, RplId vehicleId, string prefab, int count, int ammo)
	{
		HAM_VehicleDepot.AddCount(GetPlayerId(), HAM_DepotEntity(depotId), HAM_DepotEntity(vehicleId), prefab, count, ammo);
	}

	protected void HAM_DepotRemoveServer(RplId depotId, RplId vehicleId, string prefab, int ammo, int count)
	{
		HAM_VehicleDepot.RemoveCount(GetPlayerId(), HAM_DepotEntity(depotId), HAM_DepotEntity(vehicleId), prefab, ammo, count);
	}

	protected void HAM_DepotClearServer(RplId depotId, RplId vehicleId)
	{
		HAM_VehicleDepot.ClearCargo(GetPlayerId(), HAM_DepotEntity(depotId), HAM_DepotEntity(vehicleId));
	}

	protected void HAM_DepotPresetSaveServer(RplId vehicleId, string name)
	{
		HAM_VehicleDepot.SavePreset(GetPlayerId(), HAM_DepotEntity(vehicleId), name);
	}

	protected void HAM_DepotPresetDeleteServer(RplId vehicleId, string name)
	{
		HAM_VehicleDepot.DeletePreset(GetPlayerId(), HAM_DepotEntity(vehicleId), name);
	}

	protected void HAM_DepotPresetApplyServer(RplId depotId, RplId vehicleId, string name)
	{
		HAM_VehicleDepot.ApplyPreset(GetPlayerId(), HAM_DepotEntity(depotId), HAM_DepotEntity(vehicleId), name);
	}

	protected void HAM_DepotPersistentServer(RplId vehicleId, bool enabled)
	{
		HAM_VehicleDepot.SetPersistent(GetPlayerId(), HAM_DepotEntity(vehicleId), enabled);
	}

	protected void HAM_DepotFilterAskServer()
	{
		HAM_VehicleDepotFilter.Start();
		HAM_DepotFilterPushAll();
	}

	protected void HAM_DepotFilterSetServer(string prefab, bool shown)
	{
		if (!HAM_VehicleDepotFilter.IsStaff(GetPlayerId()))
			return;

		if (prefab.IsEmpty())
			return;

		HAM_VehicleDepotFilter.SetShown(prefab, shown);
		HAM_DepotFilterPushAll();
	}

	protected IEntity HAM_DepotEntity(RplId id)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(id));
		if (!rpl)
			return null;

		return rpl.GetEntity();
	}
}

modded class SCR_InventoryMenuUI
{
	void HAM_ShowDepot(BaseInventoryStorageComponent arsenal, BaseInventoryStorageComponent trunk)
	{
		if (m_iVicinityDiscoveryRadius < 40)
			m_iVicinityDiscoveryRadius = 40;

		if (arsenal)
			OpenStorageAsContainer(arsenal, false, true);

		if (!trunk)
			return;

		Widget root = GetRootWidget();
		if (!root)
			return;

		Widget right = root.FindAnyWidget("StoragesListSlot");
		if (!right)
			right = root.FindAnyWidget("OpenedStorages");

		if (!right)
			return;

		SCR_InventoryOpenedStorageUI handler = CreateOpenedStorageUI(trunk);
		if (!handler)
			return;

		Widget panel = GetGame().GetWorkspace().CreateWidgets("{E61475CBB560757F}UI/layouts/Menus/Inventory/InventoryContainerGrid.layout", right);
		if (!panel)
			return;

		panel.AddHandler(handler);
		m_aOpenedStoragesUI.Insert(handler);
		OpenLinkedStorages(trunk, false, true);
		FocusOnSlotInStorage(handler);
	}
}
