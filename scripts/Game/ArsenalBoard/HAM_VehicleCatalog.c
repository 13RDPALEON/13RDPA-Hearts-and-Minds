//------------------------------------------------------------------------------------------------
// Véhicules de chaque faction et du catalogue global. Le catalogue n'est pas modifié.
//------------------------------------------------------------------------------------------------

class HAM_VehicleCatalog
{
	protected static ref map<string, int> s_mCategory;
	protected static ref map<string, string> s_mLabels;

	static string Label(string prefab)
	{
		if (!s_mLabels)
			BuildLabels();

		string cached;
		if (s_mLabels && s_mLabels.Find(prefab, cached))
			return cached;

		return BaseLabel(prefab);
	}

	protected static void BuildLabels()
	{
		array<string> prefabs = {};
		if (Collect(prefabs) <= 0)
			return;

		s_mLabels = new map<string, string>();
		array<string> names = {};
		ref map<string, int> counts = new map<string, int>();
		foreach (string prefab : prefabs)
		{
			string name = BaseLabel(prefab);
			names.Insert(name);
			int count;
			if (!counts.Find(name, count))
				count = 0;

			counts.Set(name, count + 1);
		}

		for (int i = 0; i < prefabs.Count(); i++)
		{
			string name = names[i];
			int count;
			counts.Find(name, count);
			if (count > 1)
				name = name + " — " + VariantOf(prefabs[i], names[i]);

			s_mLabels.Set(prefabs[i], name);
		}
	}

	protected static string BaseLabel(string prefab)
	{
		string fallback = FileName(prefab);
		string raw = NameFromComponent(prefab, SCR_EditableVehicleComponent);
		if (raw.IsEmpty())
			raw = NameFromComponent(prefab, SCR_EditableEntityComponent);

		string shown = TranslateKeys(raw);
		if (shown.IsEmpty())
			return fallback;

		return shown;
	}

	protected static string VariantOf(string prefab, string shown)
	{
		string file = FileName(prefab);
		file.Replace("_", " ");
		string shownLow = shown;
		shownLow.ToLower();
		string variant = string.Empty;
		int i = 0;
		while (i < file.Length())
		{
			while (i < file.Length() && file.Substring(i, 1) == " ")
				i = i + 1;

			int start = i;
			while (i < file.Length() && file.Substring(i, 1) != " ")
				i = i + 1;

			if (start >= i)
				break;

			string token = file.Substring(start, i - start);
			string low = token;
			low.ToLower();
			if (shownLow.Contains(low))
				continue;

			if (!variant.IsEmpty())
				variant = variant + " ";

			variant = variant + token;
		}

		if (variant.IsEmpty())
			return FileName(prefab);

		return variant;
	}

	static int Collect(out array<string> prefabs)
	{
		prefabs.Clear();
		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalog)
			Append(catalog.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE), prefabs);

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

				Append(scr.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false), prefabs);
			}
		}

		return prefabs.Count();
	}

	static bool Contains(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		array<string> all = {};
		Collect(all);
		string path = PathOf(prefab);
		foreach (string entry : all)
		{
			if (entry == prefab || PathOf(entry) == path)
				return true;
		}

		return false;
	}

	protected static string NameFromComponent(string prefab, typename componentType)
	{
		if (HAM_ProfileFiles.IsProfilePath(prefab))
			return string.Empty;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return string.Empty;

		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(resource, componentType);
		if (!component)
			return string.Empty;

		BaseContainer infoSource = component.GetObject("m_UIInfo");
		if (!infoSource)
			return string.Empty;

		string name = string.Empty;
		infoSource.Get("Name", name);
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

	static int VanillaCost(string prefab)
	{
		SCR_EntityCatalogSpawnerData data = DataOf(prefab);
		if (!data)
			return 0;

		return data.HAM_VanillaSupplyCost();
	}

	static SCR_ECharacterRank VanillaRank(string prefab)
	{
		SCR_EntityCatalogSpawnerData data = DataOf(prefab);
		if (!data)
			return SCR_ECharacterRank.PRIVATE;

		return data.HAM_VanillaRank();
	}

	// 1 léger, 2 char, 3 lourd, 4 aérien, 5 autre.
	static int Category(string prefab)
	{
		if (prefab.IsEmpty())
			return 5;

		if (!s_mCategory)
			s_mCategory = new map<string, int>();

		int cached;
		if (s_mCategory.Find(prefab, cached))
			return cached;

		int category = 5;
		if (LooksAir(prefab))
			category = 4;
		else
		{
			string low = prefab;
			low.ToLower();
			if (low.Contains("/tracked/") || low.Contains("/tank") || low.Contains("tank_"))
				category = 2;
			else if (low.Contains("apc") || low.Contains("truck") || low.Contains("ural") || low.Contains("m923") || low.Contains("heavy") || low.Contains("stryker") || low.Contains("lav25") || low.Contains("btr"))
				category = 3;
			else if (low.Contains("/wheeled/") || low.Contains("/car") || low.Contains("vehicle") || low.Contains("/boat"))
				category = 1;
		}

		s_mCategory.Set(prefab, category);
		return category;
	}

	static bool IsAir(string prefab)
	{
		return Category(prefab) == 4;
	}

	protected static bool LooksAir(string prefab)
	{
		string low = prefab;
		low.ToLower();
		return low.Contains("helicopter") || low.Contains("/heli") || low.Contains("aircraft") || low.Contains("/plane") || low.Contains("/air/") || low.Contains("rotor");
	}

	protected static void Append(SCR_EntityCatalog catalog, notnull array<string> prefabs)
	{
		if (!catalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
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

	protected static SCR_EntityCatalogSpawnerData DataOf(string prefab)
	{
		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalog)
		{
			SCR_EntityCatalogSpawnerData data = DataIn(catalog.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE), prefab);
			if (data)
				return data;
		}

		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return null;

		array<Faction> factions = {};
		manager.GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			SCR_Faction scr = SCR_Faction.Cast(faction);
			if (!scr)
				continue;

			SCR_EntityCatalogSpawnerData data = DataIn(scr.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false), prefab);
			if (data)
				return data;
		}

		return null;
	}

	protected static SCR_EntityCatalogSpawnerData DataIn(SCR_EntityCatalog catalog, string prefab)
	{
		if (!catalog)
			return null;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		string path = PathOf(prefab);
		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			if (!entry)
				continue;

			string name = entry.GetPrefab();
			if (name != prefab && PathOf(name) != path)
				continue;

			return SCR_EntityCatalogSpawnerData.Cast(entry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
		}

		return null;
	}

	protected static string PathOf(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	protected static string FileName(string prefab)
	{
		string path = PathOf(prefab);
		int slash = path.LastIndexOf("/");
		if (slash >= 0 && slash + 1 < path.Length())
			path = path.Substring(slash + 1, path.Length() - slash - 1);

		if (path.EndsWith(".et"))
			path = path.Substring(0, path.Length() - 3);

		return path;
	}
}
