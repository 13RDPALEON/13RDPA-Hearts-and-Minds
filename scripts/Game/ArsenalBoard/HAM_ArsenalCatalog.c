//------------------------------------------------------------------------------------------------
// Noms et catégories des prefabs d'arsenal. Les catalogues vanilla ne sont pas modifiés.
//------------------------------------------------------------------------------------------------

class HAM_ArsenalCatalog
{
	protected static ref map<string, string> s_mItemNames;

	static string ItemLabel(string prefab)
	{
		string fallback = PrefabFileName(prefab);
		if (prefab.IsEmpty())
			return fallback;

		if (!s_mItemNames)
			s_mItemNames = new map<string, string>();

		string cached;
		if (s_mItemNames.Find(prefab, cached))
			return cached;

		string shown = fallback;
		if (HAM_ProfileFiles.IsProfilePath(prefab))
		{
			s_mItemNames.Set(prefab, shown);
			return shown;
		}

		Resource resource = Resource.Load(prefab);
		if (resource && resource.IsValid())
		{
			string raw = string.Empty;
			IEntityComponentSource inventory = SCR_BaseContainerTools.FindComponentSource(resource, InventoryItemComponent);
			if (inventory)
			{
				BaseContainer attributes = inventory.GetObject("Attributes");
				if (attributes)
					raw = ContainerName(attributes.GetObject("ItemDisplayName"));
			}

			if (raw.IsEmpty())
			{
				IEntityComponentSource editable = SCR_BaseContainerTools.FindComponentSource(resource, SCR_EditableEntityComponent);
				if (editable)
					raw = ContainerName(editable.GetObject("m_UIInfo"));
			}

			string translated = TranslateKeys(raw);
			if (!translated.IsEmpty())
				shown = translated;
		}

		s_mItemNames.Set(prefab, shown);
		return shown;
	}

	protected static string ContainerName(BaseContainer container)
	{
		if (!container)
			return string.Empty;

		string name = string.Empty;
		container.Get("Name", name);
		return name;
	}

	protected static string TranslateKeys(string raw)
	{
		if (raw.IsEmpty())
			return string.Empty;

		string shown = string.Empty;
		int i = 0;
		while (i < raw.Length())
		{
			string ch = raw.Substring(i, 1);
			if (ch != "#")
			{
				shown = shown + ch;
				i = i + 1;
				continue;
			}

			int start = i;
			i = i + 1;
			while (i < raw.Length())
			{
				string next = raw.Substring(i, 1);
				if (next == " " || next == "/" || next == "-" || next == ",")
					break;

				i = i + 1;
			}

			string key = raw.Substring(start, i - start);
			string translated = WidgetManager.Translate(key);
			if (translated.IsEmpty() || translated == key)
				shown = shown + key;
			else
				shown = shown + translated;
		}

		return shown;
	}

	protected static string PrefabFileName(string prefab)
	{
		int brace = prefab.IndexOf("}");
		string path = prefab;
		if (brace >= 0 && brace + 1 < prefab.Length())
			path = prefab.Substring(brace + 1, prefab.Length() - brace - 1);

		int slash = path.LastIndexOf("/");
		if (slash >= 0 && slash + 1 < path.Length())
			path = path.Substring(slash + 1, path.Length() - slash - 1);

		if (path.EndsWith(".et"))
			path = path.Substring(0, path.Length() - 3);

		return path;
	}

	static int CollectPrefabs(out array<string> prefabs)
	{
		prefabs.Clear();

		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalog)
			AppendCatalog(catalog.GetEntityCatalogOfType(EEntityCatalogType.ITEM), prefabs);

		FactionManager manager = GetGame().GetFactionManager();
		if (manager)
		{
			array<Faction> factions = {};
			manager.GetFactionsList(factions);
			foreach (Faction faction : factions)
			{
				SCR_Faction scr = SCR_Faction.Cast(faction);
				if (!scr)
					continue;

				AppendCatalog(scr.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, false), prefabs);
			}
		}

		return prefabs.Count();
	}

	protected static void AppendCatalog(SCR_EntityCatalog itemCatalog, notnull array<string> prefabs)
	{
		if (!itemCatalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		itemCatalog.GetEntityList(entries);

		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			if (!entry)
				continue;

			string prefab = entry.GetPrefab();
			if (prefab.IsEmpty())
				continue;

			if (prefabs.Find(prefab) >= 0)
				continue;

			prefabs.Insert(prefab);
		}
	}

	// 0 armes, 1 munitions, 2 équipement, 3 soins, 4 explosifs, 5 autre.
	static int CategoryOfPrefab(string prefab)
	{
		string low = prefab;
		low.ToLower();

		if (low.Contains("/magazines/") || low.Contains("/magazine/") || low.Contains("/ammo/") || low.Contains("/ammunition/") || low.Contains("/shells/"))
			return 1;

		if (low.Contains("/grenades/") || low.Contains("/grenade") || low.Contains("/mines/") || low.Contains("/mine_") || low.Contains("/explosives/") || low.Contains("/explosive") || low.Contains("/charges/") || low.Contains("/throwable"))
			return 4;

		if (low.Contains("/medical/") || low.Contains("/medicine/") || low.Contains("/health/") || low.Contains("bandage") || low.Contains("morphine") || low.Contains("tourniquet") || low.Contains("saline"))
			return 3;

		if (low.Contains("/attachments/") || low.Contains("/attachment/") || low.Contains("/optics/") || low.Contains("/optic_"))
			return 2;

		if (low.Contains("/weapons/") || low.Contains("/weapon/") || low.Contains("/launchers/") || low.Contains("/launcher") || low.Contains("/rifles/") || low.Contains("/pistols/") || low.Contains("/machineguns/"))
			return 0;

		if (IsBackpackPrefab(prefab) || low.Contains("/equipment/") || low.Contains("/characters/") || low.Contains("/uniforms/") || low.Contains("/uniform") || low.Contains("/headgear/") || low.Contains("/helmet") || low.Contains("/vests/") || low.Contains("/vest_") || low.Contains("/footwear/") || low.Contains("/gloves/") || low.Contains("/loadout/"))
			return 2;

		return 5;
	}

	static bool IsBackpackPrefab(string prefab)
	{
		string low = prefab;
		low.ToLower();
		if (low.Contains("backpack"))
			return true;

		if (low.Contains("/packs/"))
			return true;

		if (low.Contains("veshmeshok"))
			return true;

		if (low.Contains("radio_backpack") || low.Contains("radiobackpack"))
			return true;

		return false;
	}

	static bool IsCarrierPrefab(string prefab)
	{
		if (IsBackpackPrefab(prefab))
			return true;

		string low = prefab;
		low.ToLower();
		if (low.Contains("/vest") || low.Contains("vest_") || low.Contains("gilet") || low.Contains("platecarrier") || low.Contains("carrier_"))
			return true;

		if (low.Contains("/pants") || low.Contains("pants_") || low.Contains("trouser") || low.Contains("pantalon"))
			return true;

		return false;
	}
}

