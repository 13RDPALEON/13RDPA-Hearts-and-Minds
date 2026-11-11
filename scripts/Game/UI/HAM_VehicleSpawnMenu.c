//------------------------------------------------------------------------------------------------
// Menu de sortie des véhicules : liste, image, puis placement.
//------------------------------------------------------------------------------------------------

modded enum ChimeraMenuPreset
{
	HAM_VehicleSpawn
}

void HAM_VehicleSpawnTryPlace()
{
	HAM_VehicleSpawnUI.WatchAim();
}

void HAM_VehicleSpawnWatchPlace()
{
	HAM_VehicleSpawnUI.WatchAim();
}

modded class SCR_BasePreviewEntity
{
	void HAM_Move(float x, float y, float z, float yaw, float lift)
	{
		vector angles = Vector(yaw, 0, 0);
		vector mat[4];
		Math3D.AnglesToMatrix(angles, mat);
		mat[3] = Vector(x, y - lift, z);
		m_fHeightTerrain = 0;
		SetPreviewTransform(mat, EEditorTransformVertical.TERRAIN, lift, false, null);
	}
}

class HAM_VehiclePlaceInput
{
	protected bool m_bListening;

	void Start()
	{
		if (m_bListening)
			return;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		input.AddActionListener("HAM_VehiclePlace", EActionTrigger.DOWN, OnPlace);
		input.AddActionListener("HAM_VehiclePlaceCancel", EActionTrigger.DOWN, OnCancel);
		input.AddActionListener("MenuBack", EActionTrigger.DOWN, OnCancel);
		m_bListening = true;
	}

	void Stop()
	{
		if (!m_bListening)
			return;

		InputManager input = GetGame().GetInputManager();
		if (input)
		{
			input.RemoveActionListener("HAM_VehiclePlace", EActionTrigger.DOWN, OnPlace);
			input.RemoveActionListener("HAM_VehiclePlaceCancel", EActionTrigger.DOWN, OnCancel);
			input.RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnCancel);
		}

		m_bListening = false;
	}

	void OnPlace(float value, EActionTrigger reason)
	{
		HAM_VehicleSpawnUI.OnPlacePressed(value, reason);
	}

	void OnCancel(float value, EActionTrigger reason)
	{
		HAM_VehicleSpawnUI.OnCancelPressed(value, reason);
	}
}

class HAM_VehicleSpawnMenu : ChimeraMenuBase
{
	protected static int s_iKind;
	protected ref HAM_VehicleSpawnHandler m_Handler;

	static void SetKind(int kind)
	{
		s_iKind = kind;
	}

	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		Widget root = GetRootWidget();
		if (!root)
			return;

		m_Handler = new HAM_VehicleSpawnHandler(root, s_iKind);
		root.AddHandler(m_Handler);
		m_Handler.Bind();
		HAM_VehicleSpawnUI.Remember(this, m_Handler);
	}

	override void OnMenuClose()
	{
		super.OnMenuClose();
		m_Handler = null;
		HAM_VehicleSpawnUI.Forget();
	}
}

class HAM_VehicleSpawnUI
{
	protected static MenuBase s_Menu;
	protected static HAM_VehicleSpawnHandler s_Handler;
	protected static int s_iPlaceMode;
	protected static string s_sPlacePrefab;
	protected static string s_sConfirmPrefab;
	protected static bool s_bWatchPlace;
	protected static float s_fNextConfirm;
	protected static vector s_vAim;
	protected static vector s_vAimDir;
	protected static float s_fAimYaw;
	protected static IEntity s_Preview;
	protected static bool s_bListening;
	protected static ref HAM_VehiclePlaceInput s_Input;
	protected static SCR_CatalogEntitySpawnerComponent s_Spawner;

	//------------------------------------------------------------------------------------------------
	//! Liste complète d'un garage ou d'un hélipad du Conflit.
	static void OpenForSpawner(SCR_CatalogEntitySpawnerComponent spawner)
	{
		if (!spawner)
			return;

		s_Spawner = spawner;
		Open(HAM_VehicleSpawnHandler.KIND_GARAGE);
	}

	//------------------------------------------------------------------------------------------------
	static SCR_CatalogEntitySpawnerComponent Spawner()
	{
		return s_Spawner;
	}

	//------------------------------------------------------------------------------------------------
	//! Même demande que l'action vanilla du garage : le serveur revérifie fournitures, grade et place,
	//! puis fait payer la base. Rend la raison d'un refus, vide si la demande est partie.
	static string RequestFromSpawner(int assetIndex)
	{
		SCR_CatalogEntitySpawnerComponent spawner = s_Spawner;
		if (!spawner)
			return "Garage introuvable.";

		SCR_EntityCatalogEntry entry = spawner.GetEntryAtIndex(assetIndex);
		if (!entry)
			return "Véhicule introuvable.";

		SCR_EntityCatalogSpawnerData data = SCR_EntityCatalogSpawnerData.Cast(entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
		PlayerController controller = GetGame().GetPlayerController();
		if (!data || !controller)
			return "Véhicule introuvable.";

		IEntity user = controller.GetControlledEntity();
		if (!user)
			return "Personnage introuvable.";

		SCR_EEntityRequestStatus status = spawner.GetRequestState(entry, user);
		if (status != SCR_EEntityRequestStatus.CAN_SPAWN)
			return StatusText(status);

		SCR_EntitySpawnerSlotComponent slot = spawner.GetFreeSlot(data);
		if (!slot)
			return "Aucune place de sortie libre pour ce véhicule : dégager l'emplacement.";

		SCR_SpawnerRequestComponent request = SCR_SpawnerRequestComponent.Cast(controller.FindComponent(SCR_SpawnerRequestComponent));
		if (!request)
			return "Demande impossible.";

		request.RequestCatalogEntitySpawn(assetIndex, spawner, user, slot);
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static string StatusText(SCR_EEntityRequestStatus status)
	{
		switch (status)
		{
			case SCR_EEntityRequestStatus.NOT_ENOUGH_SUPPLIES: return "Fournitures insuffisantes à la base.";
			case SCR_EEntityRequestStatus.NOT_ENOUGH_SPACE: return "Pas assez de place.";
			case SCR_EEntityRequestStatus.RANK_LOW: return "Grade insuffisant.";
			case SCR_EEntityRequestStatus.COOLDOWN: return "Demande trop rapprochée.";
			case SCR_EEntityRequestStatus.REQUESTER_NOT_GROUPLEADER: return "Réservé au chef de groupe.";
			case SCR_EEntityRequestStatus.GROUP_FULL: return "Groupe complet.";
			case SCR_EEntityRequestStatus.AI_LIMIT_REACHED: return "Limite d'IA atteinte.";
		}

		return "Véhicule indisponible ici.";
	}

	static void Open(int kind)
	{
		if (kind == 0)
			return;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus)
			return;

		if (s_Menu)
		{
			Close();
			return;
		}

		HAM_VehicleSpawnMenu.SetKind(kind);
		s_Menu = menus.OpenMenu(ChimeraMenuPreset.HAM_VehicleSpawn);
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehicleBoardAskSync();
	}

	static void RefreshIfOpen()
	{
		if (s_Handler)
			s_Handler.Refresh();
	}

	static void Close()
	{
		StopAim();
		GetGame().GetCallqueue().Remove(HAM_VehicleSpawnWatchPlace);
		MenuBase menu = s_Menu;
		s_Menu = null;
		s_Handler = null;
		if (menu)
			menu.Close();
	}

	static void Remember(MenuBase menu, HAM_VehicleSpawnHandler handler)
	{
		s_Menu = menu;
		s_Handler = handler;
	}

	static void Forget()
	{
		s_Menu = null;
		s_Handler = null;
	}

	static void BeginPlacement(string prefab, int kind)
	{
		if (prefab.IsEmpty())
			return;

		s_sPlacePrefab = prefab;
		s_sConfirmPrefab = prefab;
		s_iPlaceMode = kind;
		if (s_iPlaceMode != 2)
			s_iPlaceMode = 1;
		Close();
		s_sPlacePrefab = prefab;
		s_sConfirmPrefab = prefab;
		s_iPlaceMode = kind;
		if (s_iPlaceMode != 2)
			s_iPlaceMode = 1;
		s_bWatchPlace = true;
		ReleaseEditor();
		World world = GetGame().GetWorld();
		if (world)
			s_fNextConfirm = world.GetWorldTime() + 400;
		Listen(true);
		GetGame().GetCallqueue().CallLater(HAM_VehicleSpawnTryPlace, 50, false);
	}

	// L'éditeur de Game Master lance une animation d'entrée qui reste bloquée. On ne l'ouvre plus.
	static void WatchAim()
	{
		if (!s_bWatchPlace || s_sConfirmPrefab.IsEmpty())
			return;

		InputManager input = GetGame().GetInputManager();
		if (input)
			input.ActivateContext("HAM_VehiclePlaceContext");

		if (FindAim())
			MoveGhost();

		GetGame().GetCallqueue().CallLater(HAM_VehicleSpawnTryPlace, 50, false);
	}

	static void OnPlacePressed(float value, EActionTrigger reason)
	{
		if (!s_bWatchPlace)
			return;

		CommitAim();
	}

	static void OnCancelPressed(float value, EActionTrigger reason)
	{
		if (!s_bWatchPlace)
			return;

		s_sConfirmPrefab = string.Empty;
		s_sPlacePrefab = string.Empty;
		StopAim();
		ReleaseEditor();
	}

	static void CommitAim()
	{
		if (s_sConfirmPrefab.IsEmpty())
			return;

		World world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		if (now < s_fNextConfirm)
		{
			GetGame().GetCallqueue().CallLater(HAM_VehicleSpawnTryPlace, 50, false);
			return;
		}

		if (!FindAim())
		{
			GetGame().GetCallqueue().CallLater(HAM_VehicleSpawnTryPlace, 50, false);
			return;
		}

		string prefab = s_sConfirmPrefab;
		int mode = s_iPlaceMode;
		float x = s_vAim[0];
		float y = s_vAim[1];
		float z = s_vAim[2];
		float yaw = s_fAimYaw;
		s_fNextConfirm = now + 600;
		s_sConfirmPrefab = string.Empty;
		s_sPlacePrefab = string.Empty;
		StopAim();
		ReleaseEditor();

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_VehiclePlaceAsk(prefab, mode, x, y, z, yaw);
	}

	static bool ConfirmEditorPlace()
	{
		ReleaseEditor();
		s_sConfirmPrefab = string.Empty;
		s_sPlacePrefab = string.Empty;
		StopAim();
		return true;
	}

	protected static bool FindAim()
	{
		s_vAim = "0 0 0";
		s_fAimYaw = 0;
		CameraManager cameras = GetGame().GetCameraManager();
		if (!cameras || !cameras.CurrentCamera())
			return false;

		vector mat[4];
		cameras.CurrentCamera().GetWorldTransform(mat);
		vector start = mat[3];
		vector dir = mat[2];
		s_fAimYaw = dir.VectorToAngles()[0];
		s_vAimDir = dir;
		vector end = start + dir * 50;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		TraceParam param = new TraceParam();
		param.Start = start;
		param.End = end;
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		if (s_Preview)
			param.Exclude = s_Preview;

		float hit = world.TraceMove(param, null);
		vector point;
		if (hit <= 0 || hit >= 1)
			point = start + dir * 8;
		else
			point = start + (end - start) * hit;

		TraceParam down = new TraceParam();
		down.Start = point + Vector(0, 3, 0);
		down.End = point + Vector(0, -8, 0);
		down.Flags = TraceFlags.WORLD;
		float ground = world.TraceMove(down, null);
		if (ground > 0 && ground < 1)
			point = down.Start + (down.End - down.Start) * ground;

		point[1] = point[1] + 0.4;
		s_vAim = point;
		KeepAwayFromPlayer();
		return true;
	}

	// Les gros véhicules et les hélicos ont leur origine au centre : trop près, la carlingue traverse le joueur.
	protected static void KeepAwayFromPlayer()
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		IEntity body = player.GetControlledEntity();
		if (!body)
			return;

		float clearance = 3.5;
		float minDist = 8;
		if (s_iPlaceMode == 2)
		{
			clearance = 7;
			minDist = 16;
		}

		if (s_Preview)
		{
			vector mins;
			vector maxs;
			s_Preview.GetBounds(mins, maxs);
			float reach = Math.AbsFloat(mins[0]);
			float other = Math.AbsFloat(maxs[0]);
			if (other > reach)
				reach = other;
			other = Math.AbsFloat(mins[2]);
			if (other > reach)
				reach = other;
			other = Math.AbsFloat(maxs[2]);
			if (other > reach)
				reach = other;
			if (reach + clearance > minDist)
				minDist = reach + clearance;
		}

		vector from = body.GetOrigin();
		vector flat = s_vAim - from;
		flat[1] = 0;
		if (flat.Length() < 1)
		{
			flat = s_vAimDir;
			flat[1] = 0;
		}

		if (flat.Length() < 0.05)
			return;

		if (flat.Length() >= minDist)
			return;

		flat.Normalize();
		vector pushed = from + flat * minDist;
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			TraceParam down = new TraceParam();
			down.Start = pushed + Vector(0, 8, 0);
			down.End = pushed + Vector(0, -12, 0);
			down.Flags = TraceFlags.WORLD;
			float ground = world.TraceMove(down, null);
			if (ground > 0 && ground < 1)
				pushed = down.Start + (down.End - down.Start) * ground;
		}

		pushed[1] = pushed[1] + 0.4;
		s_vAim = pushed;
	}

	protected static void Listen(bool on)
	{
		if (!s_Input)
			s_Input = new HAM_VehiclePlaceInput();

		if (on)
		{
			if (s_bListening)
				return;

			s_Input.Start();
			s_bListening = true;
			return;
		}

		if (!s_bListening)
			return;

		s_Input.Stop();
		s_bListening = false;
	}

	protected static void StopAim()
	{
		s_bWatchPlace = false;
		GetGame().GetCallqueue().Remove(HAM_VehicleSpawnTryPlace);
		Listen(false);
		if (!s_Preview)
			return;

		SCR_EntityHelper.DeleteEntityAndChildren(s_Preview);
		s_Preview = null;
	}

	protected static bool s_bGhostWarned;

	protected static void MoveGhost()
	{
		if (!s_Preview)
			CreateGhost();

		if (!s_Preview)
			return;

		SCR_BasePreviewEntity preview = SCR_BasePreviewEntity.Cast(s_Preview);
		if (preview)
		{
			preview.HAM_Move(s_vAim[0], s_vAim[1], s_vAim[2], s_fAimYaw, 0);
			return;
		}

		vector aimAngles = Vector(s_fAimYaw, 0, 0);
		vector aimMat[4];
		Math3D.AnglesToMatrix(aimAngles, aimMat);
		aimMat[3] = s_vAim;
		s_Preview.SetWorldTransform(aimMat);
		Physics phys = s_Preview.GetPhysics();
		if (phys)
		{
			phys.SetActive(ActiveState.INACTIVE);
			phys.EnableGravity(false);
		}
	}

	protected static void CreateGhost()
	{
		Resource res = Resource.Load(s_sConfirmPrefab);
		if (!res || !res.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector angles = Vector(s_fAimYaw, 0, 0);
		vector mat[4];
		Math3D.AnglesToMatrix(angles, mat);
		mat[3] = s_vAim;
		params.Transform[0] = mat[0];
		params.Transform[1] = mat[1];
		params.Transform[2] = mat[2];
		params.Transform[3] = mat[3];

		ResourceName previewClass = "SCR_PrefabPreviewEntity";
		ResourceName ghostMat = "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat";
		s_Preview = SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(res, previewClass, GetGame().GetWorld(), params, ghostMat);
		if (s_Preview)
			return;

		s_Preview = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (s_Preview || s_bGhostWarned)
			return;

		s_bGhostWarned = true;
		Print("[13RDPA H&M] Fantôme non créé", LogLevel.WARNING);
	}

	protected static void ReleaseEditor()
	{
		SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
		if (editor && editor.IsOpened())
			editor.Close();

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		IEntity body = player.GetControlledEntity();
		if (!body)
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetDisableMovementControls(false);
	}



	static void SpawnConfirmed(string prefab, int mode, float x, float y, float z, float yaw)
	{
		if (!Replication.IsServer() || prefab.IsEmpty())
			return;

		HAM_VehicleBoardStore.Start();
		if (!HAM_VehicleBoardStore.IsShown(prefab) || !HAM_VehicleCatalog.Contains(prefab))
			return;

		bool air = HAM_VehicleCatalog.IsAir(prefab);
		if (mode == 2 && !air)
			return;

		if (mode == 1 && air)
			return;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector angles = Vector(yaw, 0, 0);
		vector mat[4];
		Math3D.AnglesToMatrix(angles, mat);
		mat[3] = Vector(x, y, z);
		params.Transform[0] = mat[0];
		params.Transform[1] = mat[1];
		params.Transform[2] = mat[2];
		params.Transform[3] = mat[3];

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, world, params);
		if (!spawned || !spawned.IsInherited(Vehicle))
			return;

		HAM_VehiclePersist.NotePlayer(spawned);
		CarControllerComponent car = CarControllerComponent.Cast(spawned.FindComponent(CarControllerComponent));
		if (car)
			car.SetPersistentHandBrake(true);

		Physics physics = spawned.GetPhysics();
		if (physics)
			physics.SetVelocity("0 -0.1 0");
	}
}

class HAM_VehicleSpawnHandler : ScriptedWidgetEventHandler
{
	protected static const int PAGE_SIZE = 8;
	//! Liste d'un garage ou d'un hélipad du Conflit (1 = sol et 2 = air servent aux panneaux Training).
	static const int KIND_GARAGE = 3;

	protected Widget m_wRoot;
	protected int m_iKind;
	protected int m_iPage;
	protected int m_iSelected;
	protected string m_sSearch;
	protected string m_sSelected;
	protected float m_fNextClick;
	protected ref array<string> m_aPrefab;
	protected ref array<string> m_aLabel;
	protected ref array<int> m_aAssetIndex;

	void HAM_VehicleSpawnHandler(Widget root, int kind)
	{
		m_wRoot = root;
		m_iKind = kind;
		m_iPage = 0;
		m_iSelected = -1;
		m_sSearch = string.Empty;
		m_sSelected = string.Empty;
		m_aPrefab = {};
		m_aLabel = {};
		m_aAssetIndex = {};
	}

	void Refresh()
	{
		Rebuild();
	}

	void Bind()
	{
		Fit();
		GetGame().GetCallqueue().CallLater(Refit, 100, false);
		BindButton("SearchBtn");
		BindButton("Prev");
		BindButton("Next");
		BindButton("Validate");
		BindButton("Close");
		for (int i = 0; i < PAGE_SIZE; i++)
			BindButton("Item" + i.ToString());

		if (m_iKind == KIND_GARAGE)
			SetText("Title", "Véhicules disponibles");
		else if (m_iKind == 2)
			SetText("Title", "Hélicoptères");
		else
			SetText("Title", "Véhicules");

		if (m_iKind == KIND_GARAGE)
			SetText("Subtitle", GarageSubtitle());
		else
			SetText("Subtitle", "Choisir, valider, placer");
		SetText("Hint", "");
		Widget hint = m_wRoot.FindAnyWidget("Hint");
		if (hint)
			hint.SetVisible(false);
		Rebuild();
	}

	void Refit()
	{
		Fit();
	}

	protected void Fit()
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

		Place("Panel", 0, 0, panelW, panelH);
		Widget panel = m_wRoot.FindAnyWidget("Panel");
		if (panel)
			panel.SetColor(new Color(0.027, 0.051, 0.094, 0.96));

		Place("Accent", 18, 78, panelW - 36, 2);
		Widget accent = m_wRoot.FindAnyWidget("Accent");
		if (accent)
			accent.SetColor(new Color(0.776, 0.631, 0.357, 1));

		Place("Title", 28, 16, panelW * 0.45, 34);
		Place("Subtitle", 28, 50, panelW * 0.45, 20);
		Place("SearchBg", panelW - 520, 18, 360, 40);
		Place("Search", panelW - 520, 18, 360, 40);
		Place("SearchBtn", panelW - 150, 18, 122, 40);
		Widget searchBg = m_wRoot.FindAnyWidget("SearchBg");
		if (searchBg)
			searchBg.SetColor(new Color(0.16, 0.22, 0.34, 1));
		Widget searchBox = m_wRoot.FindAnyWidget("Search");
		if (searchBox)
		{
			searchBox.SetColor(new Color(0.95, 0.96, 0.97, 1));
			searchBox.AddHandler(this);
		}

		float listX = 20;
		float listW = panelW * 0.46;
		if (listW < 420)
			listW = 420;
		if (listW > 760)
			listW = 760;

		float rowTop = 96;
		float rowGap = 4;
		float rowH = 44;
		for (int i = 0; i < PAGE_SIZE; i++)
			Place("Item" + i.ToString(), listX, rowTop + i * (rowH + rowGap), listW, rowH);

		Place("Prev", listX, panelH - 52, 140, 36);
		Place("Next", listX + 152, panelH - 52, 140, 36);

		float cardX = listX + listW + 18;
		float cardW = panelW - cardX - 20;
		float cardTop = 96;
		float buttonH = 46;
		float cardH = panelH - cardTop - buttonH * 2 - 28;
		Place("Card", cardX, cardTop, cardW, cardH);
		Widget card = m_wRoot.FindAnyWidget("Card");
		if (card)
			card.SetColor(new Color(0.071, 0.102, 0.184, 1));

		float innerW = cardW - 32;
		Place("DetailName", 16, 12, innerW, 32);
		float previewH = cardH - 150;
		if (previewH < 120)
			previewH = 120;
		Place("VehPreview", 16, 50, innerW, previewH);
		float statW = innerW / 3;
		float statY = 50 + previewH + 8;
		Place("CostValue", 16, statY, statW, 28);
		Place("RankValue", 16 + statW, statY, statW, 28);
		Place("CargoValue", 16 + statW * 2, statY, statW, 28);
		Place("Validate", cardX, panelH - buttonH * 2 - 28, cardW, buttonH);
		Place("Close", cardX, panelH - buttonH - 16, cardW, buttonH);
		Widget validate = m_wRoot.FindAnyWidget("Validate");
		if (validate)
			validate.SetColor(new Color(0.118, 0.227, 0.373, 1));
		Widget closeButton = m_wRoot.FindAnyWidget("Close");
		if (closeButton)
			closeButton.SetColor(new Color(0.55, 0.16, 0.16, 1));
	}

	protected void Rebuild()
	{
		m_aPrefab.Clear();
		m_aLabel.Clear();
		m_aAssetIndex.Clear();
		if (m_iKind == KIND_GARAGE)
		{
			RebuildGarage();
			return;
		}

		array<string> all = {};
		HAM_VehicleCatalog.Collect(all);
		foreach (string prefab : all)
		{
			if (prefab.IsEmpty() || !HAM_VehicleBoardStore.IsShown(prefab))
				continue;

			bool air = HAM_VehicleCatalog.IsAir(prefab);
			if (m_iKind == 2 && !air)
				continue;

			if (m_iKind == 1 && air)
				continue;

			string label = HAM_VehicleCatalog.Label(prefab);
			if (!m_sSearch.IsEmpty())
			{
				string low = label;
				low.ToLower();
				string query = m_sSearch;
				query.ToLower();
				if (!low.Contains(query))
					continue;
			}

			m_aPrefab.Insert(prefab);
			m_aLabel.Insert(label);
		}

		FinishRebuild();
	}

	//------------------------------------------------------------------------------------------------
	//! Les véhicules du garage, dans l'ordre de sa liste : l'indice est celui que relit le serveur.
	protected void RebuildGarage()
	{
		SCR_CatalogEntitySpawnerComponent spawner = HAM_VehicleSpawnUI.Spawner();
		if (spawner)
		{
			int count = spawner.HAM_AssetCount();
			for (int i = 0; i < count; i++)
			{
				SCR_EntityCatalogEntry entry = spawner.GetEntryAtIndex(i);
				if (!entry)
					continue;

				string prefab = entry.GetPrefab();
				string label = HAM_VehicleCatalog.Label(prefab);
				if (label.IsEmpty())
					label = entry.GetEntityName();

				if (!m_sSearch.IsEmpty())
				{
					string low = label;
					low.ToLower();
					string query = m_sSearch;
					query.ToLower();
					if (!low.Contains(query))
						continue;
				}

				SCR_EntityCatalogSpawnerData data = SCR_EntityCatalogSpawnerData.Cast(entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
				if (data)
					label = label + "   ·   " + data.GetSupplyCost().ToString();

				m_aPrefab.Insert(prefab);
				m_aLabel.Insert(label);
				m_aAssetIndex.Insert(i);
			}
		}

		SetText("Subtitle", GarageSubtitle());
		FinishRebuild();
	}

	//------------------------------------------------------------------------------------------------
	protected string GarageSubtitle()
	{
		SCR_CatalogEntitySpawnerComponent spawner = HAM_VehicleSpawnUI.Spawner();
		if (!spawner)
			return "Choisir puis demander";

		int supplies = spawner.GetSpawnerResourceValue();
		return string.Format("Choisir puis demander  ·  fournitures de la base : %1", supplies);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowHint(string text)
	{
		Widget hint = m_wRoot.FindAnyWidget("Hint");
		if (!hint)
			return;

		hint.SetVisible(!text.IsEmpty());
		SetText("Hint", text);
	}

	//------------------------------------------------------------------------------------------------
	protected void FinishRebuild()
	{
		if (m_iSelected >= m_aPrefab.Count())
			m_iSelected = -1;

		if (m_iSelected < 0)
		{
			m_sSelected = string.Empty;
		}

		int pages = 1;
		if (m_aPrefab.Count() > 0)
			pages = ((m_aPrefab.Count() - 1) / PAGE_SIZE) + 1;

		if (m_iPage >= pages)
			m_iPage = pages - 1;

		if (m_iPage < 0)
			m_iPage = 0;

		Paint();
		ShowDetail();
	}

	protected void Paint()
	{
		int start = m_iPage * PAGE_SIZE;
		for (int i = 0; i < PAGE_SIZE; i++)
		{
			int index = start + i;
			ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Item" + i.ToString()));
			TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("Item" + i.ToString() + "Text"));
			if (!button)
				continue;

			if (index >= m_aPrefab.Count())
			{
				button.SetVisible(false);
				continue;
			}

			button.SetVisible(true);
			if (label)
				label.SetText(m_aLabel[index]);

			if (index == m_iSelected)
				button.SetColor(new Color(0.118, 0.227, 0.373, 1));
			else
				button.SetColor(new Color(0.05, 0.08, 0.14, 1));
		}
	}

	protected void ShowDetail()
	{
		if (m_iSelected < 0 || m_iSelected >= m_aPrefab.Count())
		{
			SetText("DetailName", "Sélectionner un véhicule");
			SetText("CostValue", "Coût  —");
			SetText("RankValue", "Grade  —");
			SetText("CargoValue", "Soute  —");
			ShowVehiclePreview("");
			return;
		}

		string prefab = m_aPrefab[m_iSelected];
		SetText("DetailName", m_aLabel[m_iSelected]);
		ShowVehiclePreview(prefab);
		int cost = HAM_VehicleCatalog.VanillaCost(prefab);
		if (HAM_VehicleBoardStore.HasCostOverride(prefab))
			cost = HAM_VehicleBoardStore.BoardPrice(prefab);

		SCR_ECharacterRank rank = HAM_VehicleCatalog.VanillaRank(prefab);
		if (HAM_VehicleBoardStore.HasRankOverride(prefab))
			rank = HAM_VehicleBoardStore.GetRankOverride(prefab, rank);

		// Au garage : les valeurs mêmes que le Conflit appliquera à la demande.
		SCR_EntityCatalogSpawnerData data = GarageData(m_iSelected);
		if (data)
		{
			cost = data.GetSupplyCost();
			rank = data.GetMinimumRequiredRank();
		}

		SetText("CostValue", "Coût  " + cost.ToString());

		SetText("RankValue", "Grade  " + typename.EnumToString(SCR_ECharacterRank, rank));
		string cargoKey = HAM_VehicleBoardStore.CargoKeyOfPrefab(prefab);
		if (!cargoKey.IsEmpty())
			SetText("CargoValue", "Soute  " + HAM_VehicleBoardStore.GetCapacityOverride(cargoKey, 0).ToString() + " cm³");
		else
			SetText("CargoValue", "Soute  standard");
	}

	protected void ShowVehiclePreview(string prefab)
	{
		if (!m_wRoot)
			return;

		ItemPreviewWidget preview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("VehPreview"));
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

	protected void Select(int index)
	{
		if (index < 0 || index >= m_aPrefab.Count())
			return;

		m_iSelected = index;
		m_sSelected = m_aPrefab[index];
		Paint();
		ShowDetail();
		if (m_iKind == KIND_GARAGE)
			ShowHint(string.Empty);
	}

	protected void Validate()
	{
		if (m_sSelected.IsEmpty())
			return;

		if (m_iKind != KIND_GARAGE)
		{
			HAM_VehicleSpawnUI.BeginPlacement(m_sSelected, m_iKind);
			return;
		}

		if (m_iSelected < 0 || m_iSelected >= m_aAssetIndex.Count())
			return;

		string refusal = HAM_VehicleSpawnUI.RequestFromSpawner(m_aAssetIndex[m_iSelected]);
		if (!refusal.IsEmpty())
		{
			ShowHint(refusal);
			return;
		}

		HAM_VehicleSpawnUI.Close();
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_EntityCatalogSpawnerData GarageData(int index)
	{
		if (m_iKind != KIND_GARAGE || index < 0 || index >= m_aAssetIndex.Count())
			return null;

		SCR_CatalogEntitySpawnerComponent spawner = HAM_VehicleSpawnUI.Spawner();
		if (!spawner)
			return null;

		SCR_EntityCatalogEntry entry = spawner.GetEntryAtIndex(m_aAssetIndex[index]);
		if (!entry)
			return null;

		return SCR_EntityCatalogSpawnerData.Cast(entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
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
			return;
		}

		FrameSlot.SetAnchorMin(widget, 0, 0);
		FrameSlot.SetAnchorMax(widget, 0, 0);
		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
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

	protected void SetText(string name, string text)
	{
		TextWidget widget = TextWidget.Cast(m_wRoot.FindAnyWidget(name));
		if (widget)
			widget.SetText(text);
	}

	protected string ButtonName(Widget w)
	{
		while (w)
		{
			string name = w.GetName();
			if (name == "Close" || name == "Validate" || name == "Prev" || name == "Next" || name == "SearchBtn")
				return name;

			if (name.IndexOf("Item") == 0 && !name.Contains("Text"))
				return name;

			w = w.GetParent();
		}

		return string.Empty;
	}

	override bool OnChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box || box.GetName() != "Search")
			return false;

		m_sSearch = box.GetText();
		m_iPage = 0;
		m_iSelected = -1;
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

		m_fNextClick = now + 250;
		if (name == "Close")
		{
			HAM_VehicleSpawnUI.Close();
			return true;
		}

		if (name == "Validate")
		{
			Validate();
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
			m_iSelected = -1;
			Rebuild();
			return true;
		}

		if (name == "Prev")
		{
			if (m_iPage > 0)
			{
				m_iPage = m_iPage - 1;
				Paint();
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
				Paint();
			}
			return true;
		}

		if (name.Length() >= 4 && name.Substring(0, 4) == "Item")
		{
			int local = name.Substring(4, name.Length() - 4).ToInt();
			Select(m_iPage * PAGE_SIZE + local);
			return true;
		}

		return false;
	}
}
