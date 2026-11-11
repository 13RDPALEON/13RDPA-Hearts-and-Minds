//------------------------------------------------------------------------------------------------
// 13RDPA — dump d'un personnage pour l'édition de tenue.
// Sortie: console + $profile:13RDPA_HeartsAndMinds/Debug/ (HAM_MannequinDump.txt, HAM_LoadoutDump.json)
// V2: slots nommés, cloth attachments, pochettes vides, counts, recette loadout
//------------------------------------------------------------------------------------------------
class HAM_MannequinDumper
{
	protected static const string LOG_PREFIX = "[CPR MannequinDump]";
	protected static const string OUT_DIR = "$profile:13RDPA_HeartsAndMinds/Debug/";
	protected static const string OUT_FILE = "$profile:13RDPA_HeartsAndMinds/Debug/HAM_MannequinDump.txt";

	//------------------------------------------------------------------------------------------------
	static void Dump(IEntity ent)
	{
		if (!ent)
		{
			Print(string.Format("%1 entity null", LOG_PREFIX), LogLevel.ERROR);
			return;
		}

		HAM_ProfileFiles.EnsureDir("$profile:13RDPA_HeartsAndMinds/");
		HAM_ProfileFiles.EnsureDir(OUT_DIR);

		array<string> lines = {};
		lines.Insert("========== CPR MANNEQUIN DUMP ==========");
		lines.Insert(string.Format("UnixTime: %1", System.GetUnixTime()));

		AppendIdentity(ent, lines);
		AppendBaconLoadout(ent, lines);
		AppendTransform(ent, lines);
		AppendFaction(ent, lines);
		AppendRank(ent, lines);
		AppendEditable(ent, lines);
		AppendLoadoutSlotsNamed(ent, lines);
		AppendWeapons(ent, lines);
		AppendEquipmentGadgets(ent, lines);
		AppendClothAttachments(ent, lines);
		AppendInventoryTree(ent, lines);
		AppendItemCounts(ent, lines);
		AppendLoadoutRecipe(ent, lines);
		AppendWarnings(ent, lines);
		AppendDamage(ent, lines);
		AppendComponents(ent, lines);
		lines.Insert("========== END DUMP ==========");

		string joined;
		foreach (string line : lines)
		{
			Print(string.Format("%1 %2", LOG_PREFIX, line), LogLevel.NORMAL);
			if (!joined.IsEmpty())
				joined += "\n";
			joined += line;
		}

		SCR_FileIOHelper.WriteFileContent(OUT_FILE, lines);
		Print(string.Format("%1 Écrit: %2 (%3 lignes / %4 chars)", LOG_PREFIX, OUT_FILE, lines.Count(), joined.Length()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static string PrefabOf(IEntity ent)
	{
		if (!ent)
			return "<null>";

		EntityPrefabData pd = ent.GetPrefabData();
		if (pd)
		{
			ResourceName rn = pd.GetPrefabName();
			if (rn)
				return string.Format("%1", rn);
		}

		return "<unknown>";
	}

	//------------------------------------------------------------------------------------------------
	protected static int IndexOfChar(string s, string needle)
	{
		if (!s || s.IsEmpty() || !needle || needle.IsEmpty())
			return -1;
		return s.IndexOf(needle);
	}

	//------------------------------------------------------------------------------------------------
	protected static int LastIndexOfChar(string s, string needle)
	{
		if (!s || s.IsEmpty() || !needle || needle.IsEmpty())
			return -1;

		int last = -1;
		int nLen = needle.Length();
		int maxStart = s.Length() - nLen;
		for (int i = 0; i <= maxStart; i++)
		{
			if (s.Substring(i, nLen) == needle)
				last = i;
		}
		return last;
	}

	//------------------------------------------------------------------------------------------------
	//! "{GUID}Prefabs/Foo.et" → "Prefabs/Foo.et" (pour TargetStorage)
	protected static string PrefabPathOf(IEntity ent)
	{
		string full = PrefabOf(ent);
		if (full == "<null>" || full == "<unknown>")
			return full;

		int brace = IndexOfChar(full, "}");
		if (brace >= 0 && brace + 1 < full.Length())
			return full.Substring(brace + 1, full.Length() - brace - 1);

		return full;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ShortName(string pathOrPrefab)
	{
		if (!pathOrPrefab || pathOrPrefab.IsEmpty())
			return "<empty>";

		string s = pathOrPrefab;
		int brace = IndexOfChar(s, "}");
		if (brace >= 0 && brace + 1 < s.Length())
			s = s.Substring(brace + 1, s.Length() - brace - 1);

		int slash = LastIndexOfChar(s, "/");
		if (slash >= 0 && slash + 1 < s.Length())
			s = s.Substring(slash + 1, s.Length() - slash - 1);

		return s;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ToLowerCopy(string s)
	{
		if (!s || s.IsEmpty())
			return string.Empty;

		// Enforce: ToLower() mutates in-place (void), does not return a string
		string outStr = s;
		outStr.ToLower();
		return outStr;
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendIdentity(IEntity ent, array<string> lines)
	{
		lines.Insert("--- IDENTITY ---");
		lines.Insert(string.Format("Type: %1", ent.Type().ToString()));
		lines.Insert(string.Format("EntityID: %1", ent.GetID()));
		lines.Insert(string.Format("Name: %1", ent.GetName()));
		lines.Insert(string.Format("Prefab: %1", PrefabOf(ent)));

		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(ent);
		if (ch)
			lines.Insert("Is SCR_ChimeraCharacter: yes");
		else
			lines.Insert("Is SCR_ChimeraCharacter: no");
	}

	//------------------------------------------------------------------------------------------------
	//! Même capture que Bacon : chaîne applicable par Coller stuff et par un slot de loadout.
	protected static void AppendBaconLoadout(IEntity ent, array<string> lines)
	{
		lines.Insert("--- BACON LOADOUT (édition) ---");
		lines.Insert("Réutilisable : Coller stuff, ou champ \"data\" d'un fichier BaconLoadoutEditor_Loadouts.");

		JsonSaveContext saveContext = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(ent, saveContext))
		{
			lines.Insert("ReadLoadoutString: ECHEC");
			return;
		}

		string data = saveContext.SaveToString();
		if (data.IsEmpty())
		{
			lines.Insert("ReadLoadoutString: vide");
			return;
		}

		HAM_LoadoutClipboard.SetRaw(data, ent.GetName());
		lines.Insert(string.Format("prefab: %1", PrefabOf(ent)));
		lines.Insert("data:");
		lines.Insert(data);

		array<string> jsonLines = {};
		jsonLines.Insert("{");
		jsonLines.Insert(string.Format("  \"prefab\": \"%1\",", PrefabOf(ent)));
		jsonLines.Insert("  \"data\":");
		jsonLines.Insert(data);
		jsonLines.Insert("}");
		string dumpPath = OUT_DIR + "HAM_LoadoutDump.json";
		SCR_FileIOHelper.WriteFileContent(dumpPath, jsonLines);
		lines.Insert("Fichier tenue: " + dumpPath);
		lines.Insert("Presse-papiers Coller stuff: rempli");
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendTransform(IEntity ent, array<string> lines)
	{
		vector o = ent.GetOrigin();
		vector a = ent.GetAngles();
		lines.Insert("--- TRANSFORM ---");
		lines.Insert(string.Format("Origin: %1 %2 %3", o[0], o[1], o[2]));
		lines.Insert(string.Format("Angles: %1 %2 %3", a[0], a[1], a[2]));
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendFaction(IEntity ent, array<string> lines)
	{
		lines.Insert("--- FACTION ---");

		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(ent);
		if (ch)
		{
			Faction f = ch.GetFaction();
			if (f)
			{
				lines.Insert(string.Format("Character faction key: %1", f.GetFactionKey()));
				lines.Insert(string.Format("Character faction name: %1", f.GetFactionName()));
			}
			else
			{
				lines.Insert("Character faction: <null>");
			}
		}

		SCR_FactionAffiliationComponent aff = SCR_FactionAffiliationComponent.Cast(ent.FindComponent(SCR_FactionAffiliationComponent));
		if (aff)
		{
			Faction af = aff.GetAffiliatedFaction();
			if (af)
				lines.Insert(string.Format("Affiliation key: %1", af.GetFactionKey()));
			else
				lines.Insert("Affiliation: <null>");
		}
		else
		{
			lines.Insert("FactionAffiliationComponent: <missing>");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendEditable(IEntity ent, array<string> lines)
	{
		lines.Insert("--- EDITABLE / GM ---");
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(ent.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
		{
			lines.Insert("EditableEntity: <missing>");
			return;
		}

		lines.Insert(string.Format("DisplayName: %1", editable.GetDisplayName()));
		lines.Insert(string.Format("Editable.GetPrefab(): %1", editable.GetPrefab()));
		lines.Insert(string.Format("Editable.GetPrefab(true): %1", editable.GetPrefab(true)));

		Faction ef = editable.GetFaction();
		if (ef)
			lines.Insert(string.Format("Editable faction: %1", ef.GetFactionKey()));
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendRank(IEntity ent, array<string> lines)
	{
		lines.Insert("--- RANK ---");
		if (!SCR_CharacterRankComponent.GetCharacterRankComponent(ent))
		{
			lines.Insert("RankComponent: <missing>");
			return;
		}

		lines.Insert(string.Format("Rank enum: %1", SCR_CharacterRankComponent.GetCharacterRank(ent)));
		lines.Insert(string.Format("Rank name: %1", SCR_CharacterRankComponent.GetCharacterRankName(ent)));
		lines.Insert(string.Format("Rank short: %1", SCR_CharacterRankComponent.GetCharacterRankNameShort(ent)));
	}

	//------------------------------------------------------------------------------------------------
	//! Slots loadout personnage avec NOM (Hat / Vest / ArmoredVest / …)
	protected static void AppendLoadoutSlotsNamed(IEntity ent, array<string> lines)
	{
		lines.Insert("--- LOADOUT SLOTS (named) ---");
		SCR_EquipedLoadoutStorageComponent loadout = SCR_EquipedLoadoutStorageComponent.Cast(ent.FindComponent(SCR_EquipedLoadoutStorageComponent));
		if (!loadout)
		{
			lines.Insert("EquipedLoadoutStorage: <missing>");
			return;
		}

		array<IEntity> items = {};
		loadout.GetAll(items, true);
		lines.Insert(string.Format("Equipped items: %1", items.Count()));

		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			string slotName = "<unknown_slot>";
			string areaName = "";
			EntitySlotInfo slotInfo = EntitySlotInfo.GetSlotInfo(item);
			if (slotInfo)
			{
				slotName = slotInfo.GetSourceName();
				if (slotName.IsEmpty())
					slotName = "<unnamed>";

				LoadoutSlotInfo lsi = LoadoutSlotInfo.Cast(slotInfo);
				if (lsi)
				{
					LoadoutAreaType area = lsi.GetAreaType();
					if (area)
						areaName = area.Type().ToString();
				}
			}

			if (areaName.IsEmpty())
				lines.Insert(string.Format("  [%1] %2", slotName, PrefabOf(item)));
			else
				lines.Insert(string.Format("  [%1] area=%2 | %3", slotName, areaName, PrefabOf(item)));

			lines.Insert(string.Format("       TargetStorage path: %1", PrefabPathOf(item)));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendWeapons(IEntity ent, array<string> lines)
	{
		lines.Insert("--- WEAPON SLOTS ---");
		array<Managed> comps = {};
		ent.FindComponents(CharacterWeaponSlotComponent, comps);
		if (comps.IsEmpty())
		{
			lines.Insert("No CharacterWeaponSlotComponent");
			return;
		}

		int i = 0;
		foreach (Managed m : comps)
		{
			CharacterWeaponSlotComponent slot = CharacterWeaponSlotComponent.Cast(m);
			if (!slot)
				continue;

			IEntity weapon = slot.GetWeaponEntity();
			lines.Insert(string.Format("[%1] slotIndex=%2 weapon=%3", i, slot.GetWeaponSlotIndex(), PrefabOf(weapon)));

			if (weapon)
			{
				array<EntitySlotInfo> wSlots = {};
				EntitySlotInfo.GetSlotInfos(weapon, wSlots);
				foreach (EntitySlotInfo ws : wSlots)
				{
					if (!ws)
						continue;
					IEntity att = ws.GetAttachedEntity();
					string sn = ws.GetSourceName();
					if (att)
						lines.Insert(string.Format("    att[%1] %2", sn, PrefabOf(att)));
					else
						lines.Insert(string.Format("    att[%1] <EMPTY>", sn));
				}
			}
			i++;
		}

		array<Managed> gComps = {};
		ent.FindComponents(CharacterGrenadeSlotComponent, gComps);
		int gi = 0;
		foreach (Managed gm : gComps)
		{
			CharacterGrenadeSlotComponent gslot = CharacterGrenadeSlotComponent.Cast(gm);
			if (!gslot)
				continue;
			IEntity gre = gslot.GetWeaponEntity();
			lines.Insert(string.Format("GrenadeSlot[%1] %2", gi, PrefabOf(gre)));
			gi++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendEquipmentGadgets(IEntity ent, array<string> lines)
	{
		lines.Insert("--- EQUIPMENT GADGETS (binocular / watch / ...) ---");
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			lines.Insert("InventoryStorageManager: <missing>");
			return;
		}

		bool any = false;
		array<BaseInventoryStorageComponent> allSt = {};
		inv.GetStorages(allSt, EStoragePurpose.PURPOSE_ANY);
		foreach (BaseInventoryStorageComponent st2 : allSt)
		{
			SCR_EquipmentStorageComponent eq2 = SCR_EquipmentStorageComponent.Cast(st2);
			if (!eq2)
				continue;

			array<IEntity> items = {};
			inv.GetAllItems(items, eq2);
			lines.Insert(string.Format("  SCR_EquipmentStorageComponent items=%1", items.Count()));
			foreach (IEntity it : items)
			{
				if (!it)
					continue;
				string sn = "<slot?>";
				EntitySlotInfo si = EntitySlotInfo.GetSlotInfo(it);
				if (si)
				{
					sn = si.GetSourceName();
					if (sn.IsEmpty())
						sn = "<unnamed>";
				}
				lines.Insert(string.Format("    [%1] %2", sn, PrefabOf(it)));
			}
			any = true;
		}

		if (!any)
			lines.Insert("(none)");
	}

	//------------------------------------------------------------------------------------------------
	//! Attachements cloth sur chaque pièce équipée (casque Goggles/Masks/…, vest pouches via slots)
	protected static void AppendClothAttachments(IEntity ent, array<string> lines)
	{
		lines.Insert("--- CLOTH ATTACHMENTS (slots on equipped gear) ---");
		SCR_EquipedLoadoutStorageComponent loadout = SCR_EquipedLoadoutStorageComponent.Cast(ent.FindComponent(SCR_EquipedLoadoutStorageComponent));
		if (!loadout)
		{
			lines.Insert("EquipedLoadoutStorage: <missing>");
			return;
		}

		array<IEntity> items = {};
		loadout.GetAll(items, true);
		foreach (IEntity cloth : items)
		{
			if (!cloth)
				continue;

			array<EntitySlotInfo> slotInfos = {};
			EntitySlotInfo.GetSlotInfos(cloth, slotInfos);
			if (slotInfos.IsEmpty())
				continue;

			string clothSlot = "<gear>";
			EntitySlotInfo parentSlot = EntitySlotInfo.GetSlotInfo(cloth);
			if (parentSlot)
				clothSlot = parentSlot.GetSourceName();

			lines.Insert(string.Format("[%1] %2  (%3 cloth slots)", clothSlot, ShortName(PrefabOf(cloth)), slotInfos.Count()));
			foreach (EntitySlotInfo si : slotInfos)
			{
				if (!si)
					continue;

				string sn = si.GetSourceName();
				if (sn.IsEmpty())
				{
					InventoryStorageSlot iss = InventoryStorageSlot.Cast(si);
					if (iss)
						sn = string.Format("id%1", iss.GetID());
					else
						sn = "<unnamed>";
				}

				IEntity att = si.GetAttachedEntity();
				string areaName = "";
				LoadoutSlotInfo lsi = LoadoutSlotInfo.Cast(si);
				if (lsi)
				{
					LoadoutAreaType area = lsi.GetAreaType();
					if (area)
						areaName = area.Type().ToString();
				}

				if (att)
				{
					if (areaName.IsEmpty())
						lines.Insert(string.Format("  + [%1] %2", sn, PrefabOf(att)));
					else
						lines.Insert(string.Format("  + [%1] area=%2 | %3", sn, areaName, PrefabOf(att)));
				}
				else
				{
					if (areaName.IsEmpty())
						lines.Insert(string.Format("  - [%1] <EMPTY>", sn));
					else
						lines.Insert(string.Format("  - [%1] area=%2 | <EMPTY>", sn, areaName));
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Arbre inventaire: storage + path TargetStorage + EMPTY explicite
	protected static void AppendInventoryTree(IEntity ent, array<string> lines)
	{
		lines.Insert("--- INVENTORY TREE (TargetStorage ready) ---");
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			lines.Insert("InventoryStorageManager: <missing>");
			return;
		}

		array<BaseInventoryStorageComponent> storages = {};
		inv.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);
		lines.Insert(string.Format("Storages: %1", storages.Count()));

		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			IEntity storageOwner = storage.GetOwner();
			string path = PrefabPathOf(storageOwner);
			array<IEntity> stored = {};
			inv.GetAllItems(stored, storage);

			if (stored.IsEmpty())
			{
				lines.Insert(string.Format("Storage %1", storage.Type().ToString()));
				lines.Insert(string.Format("  owner: %1", PrefabOf(storageOwner)));
				lines.Insert(string.Format("  TargetStorage \"%1\"", path));
				lines.Insert("  items: <EMPTY>");
				continue;
			}

			lines.Insert(string.Format("Storage %1", storage.Type().ToString()));
			lines.Insert(string.Format("  owner: %1", PrefabOf(storageOwner)));
			lines.Insert(string.Format("  TargetStorage \"%1\"", path));
			lines.Insert(string.Format("  items (%1):", stored.Count()));
			foreach (IEntity item : stored)
			{
				if (!item)
					continue;
				lines.Insert(string.Format("    - %1", PrefabOf(item)));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendItemCounts(IEntity ent, array<string> lines)
	{
		lines.Insert("--- ITEM COUNTS (by prefab) ---");
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			lines.Insert("InventoryStorageManager: <missing>");
			return;
		}

		array<IEntity> allItems = {};
		inv.GetItems(allItems, EStoragePurpose.PURPOSE_ANY);

		array<string> keys = {};
		array<int> vals = {};

		foreach (IEntity item : allItems)
		{
			if (!item)
				continue;

			string key = PrefabOf(item);
			int found = keys.Find(key);
			if (found >= 0)
			{
				vals.Set(found, vals.Get(found) + 1);
			}
			else
			{
				keys.Insert(key);
				vals.Insert(1);
			}
		}

		int mag556 = 0;
		int frag = 0;
		int smoke = 0;
		int stun = 0;

		for (int i = 0; i < keys.Count(); i++)
		{
			string key = keys.Get(i);
			int n = vals.Get(i);
			lines.Insert(string.Format("  x%1  %2", n, ShortName(key)));

			string low = ToLowerCopy(key);
			if (low.Contains("magazine_556") || (low.Contains("pmag") && low.Contains("556")))
				mag556 += n;
			if (low.Contains("grenade_m67"))
				frag += n;
			if (low.Contains("smoke"))
				smoke += n;
			if (low.Contains("stun"))
				stun += n;
		}

		lines.Insert("--- QUICK TOTALS ---");
		lines.Insert(string.Format("  Mag 5.56 (name heuristic): %1", mag556));
		lines.Insert(string.Format("  Frag M67 (name heuristic): %1", frag));
		lines.Insert(string.Format("  Smoke (name heuristic): %1", smoke));
		lines.Insert(string.Format("  Stun (name heuristic): %1", stun));
		lines.Insert(string.Format("  Total items PURPOSE_ANY: %1", allItems.Count()));
	}

	//------------------------------------------------------------------------------------------------
	//! Recette prête à coller dans un .et loadout
	protected static void AppendLoadoutRecipe(IEntity ent, array<string> lines)
	{
		lines.Insert("--- LOADOUT RECIPE (hints for .et) ---");
		SCR_EquipedLoadoutStorageComponent loadout = SCR_EquipedLoadoutStorageComponent.Cast(ent.FindComponent(SCR_EquipedLoadoutStorageComponent));
		if (loadout)
		{
			lines.Insert("// BaseLoadoutManagerComponent Slots:");
			array<IEntity> items = {};
			loadout.GetAll(items, true);
			foreach (IEntity item : items)
			{
				if (!item)
					continue;
				string slotName = "Slot";
				EntitySlotInfo si = EntitySlotInfo.GetSlotInfo(item);
				if (si)
				{
					string sn = si.GetSourceName();
					if (!sn.IsEmpty())
						slotName = sn;
				}
				lines.Insert(string.Format("//  LoadoutSlotInfo %1 { Prefab \"%2\" }", slotName, PrefabOf(item)));
			}
		}

		lines.Insert("// Cloth attachments (sur chaque pièce):");
		if (loadout)
		{
			array<IEntity> cloths = {};
			loadout.GetAll(cloths, true);
			foreach (IEntity cloth : cloths)
			{
				if (!cloth)
					continue;
				array<EntitySlotInfo> slotInfos = {};
				EntitySlotInfo.GetSlotInfos(cloth, slotInfos);
				bool any = false;
				foreach (EntitySlotInfo si : slotInfos)
				{
					if (!si || !si.GetAttachedEntity())
						continue;
					if (!any)
					{
						lines.Insert(string.Format("//  on %1:", ShortName(PrefabOf(cloth))));
						any = true;
					}
					lines.Insert(string.Format("//    RHS_LoadoutSlotInfo %1 { Prefab \"%2\" }", si.GetSourceName(), PrefabOf(si.GetAttachedEntity())));
				}
			}
		}

		lines.Insert("// InitialInventoryItems (non-vides seulement):");
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return;

		array<BaseInventoryStorageComponent> storages = {};
		inv.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);
		int cfgId = 1;
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			// Skip character-level equiped loadout / weapon storages for recipe noise
			if (SCR_EquipedLoadoutStorageComponent.Cast(storage))
				continue;
			if (EquipedWeaponStorageComponent.Cast(storage))
				continue;
			if (SCR_HandSlotStorageComponent.Cast(storage))
				continue;

			array<IEntity> stored = {};
			inv.GetAllItems(stored, storage);
			if (stored.IsEmpty())
				continue;

			IEntity owner = storage.GetOwner();
			string path = PrefabPathOf(owner);
			lines.Insert(string.Format("//  ItemsInitConfigurationItem {"));
			lines.Insert(string.Format("//    TargetStorage \"%1\"", path));
			string prefs = "";
			foreach (IEntity it : stored)
			{
				if (!it)
					continue;
				if (!prefs.IsEmpty())
					prefs += " ";
				prefs += string.Format("\"%1\"", PrefabOf(it));
			}
			lines.Insert(string.Format("//    PrefabsToSpawn { %1 }", prefs));
			lines.Insert("//  }");
			cfgId++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Alertes utiles pour reconstruire un loadout sans se tromper de pouch
	protected static void AppendWarnings(IEntity ent, array<string> lines)
	{
		lines.Insert("--- WARNINGS / CHECKS ---");
		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
		{
			lines.Insert("InventoryStorageManager: <missing>");
			return;
		}

		array<BaseInventoryStorageComponent> storages = {};
		inv.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);

		array<string> emptyDedicated = {};
		array<string> fragsElsewhere = {};
		array<string> utilCrowded = {};

		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			IEntity owner = storage.GetOwner();
			string path = PrefabPathOf(owner);
			string shortP = ShortName(path);
			string low = ToLowerCopy(shortP);

			array<IEntity> stored = {};
			inv.GetAllItems(stored, storage);
			bool empty = stored.IsEmpty();

			bool looksGrenadePouch = low.Contains("m67") || low.Contains("m18") || low.Contains("grenade") || low.Contains("frag");
			bool looksUtil = low.Contains("util") || low.Contains("ifak");

			if (empty && looksGrenadePouch)
				emptyDedicated.Insert(string.Format("EMPTY dedicated pouch: %1 (TargetStorage \"%2\")", shortP, path));

			if (!empty && looksUtil)
			{
				int n = stored.Count();
				string content = "";
				foreach (IEntity it : stored)
				{
					if (!it)
						continue;
					if (!content.IsEmpty())
						content += ", ";
					content += ShortName(PrefabOf(it));
				}
				utilCrowded.Insert(string.Format("util/ifak (%1 items): %2 -> [%3]", n, shortP, content));
			}

			foreach (IEntity it : stored)
			{
				if (!it)
					continue;
				string ip = PrefabOf(it);
				string il = ToLowerCopy(ip);
				if (il.Contains("grenade_m67") && !looksGrenadePouch)
					fragsElsewhere.Insert(string.Format("M67 in NON-M67 pouch: %1 (inside %2)", ShortName(ip), shortP));
			}
		}

		if (emptyDedicated.IsEmpty() && fragsElsewhere.IsEmpty() && utilCrowded.IsEmpty())
		{
			lines.Insert("  (no loadout reconstruction warnings)");
			return;
		}

		foreach (string w : emptyDedicated)
			lines.Insert(string.Format("  ! %1", w));
		foreach (string w : fragsElsewhere)
			lines.Insert(string.Format("  ! %1", w));
		foreach (string w : utilCrowded)
			lines.Insert(string.Format("  i %1", w));
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendDamage(IEntity ent, array<string> lines)
	{
		lines.Insert("--- DAMAGE / HEALTH ---");
		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(ent.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dmg)
		{
			lines.Insert("DamageManager: <missing>");
			return;
		}

		lines.Insert(string.Format("IsDestroyed: %1", dmg.IsDestroyed()));

		HitZone defaultZone = dmg.GetDefaultHitZone();
		if (defaultZone)
		{
			lines.Insert(string.Format("DefaultHitZone: %1 health=%2/%3 scaled=%4",
				defaultZone.GetName(),
				defaultZone.GetHealth(),
				defaultZone.GetMaxHealth(),
				defaultZone.GetHealthScaled()));
		}

		array<HitZone> zones = {};
		dmg.GetAllHitZonesInHierarchy(zones);
		lines.Insert(string.Format("HitZonesInHierarchy: %1", zones.Count()));
		foreach (HitZone zone : zones)
		{
			if (!zone)
				continue;
			lines.Insert(string.Format("  - %1 hp=%2/%3 scaled=%4 state=%5",
				zone.GetName(),
				zone.GetHealth(),
				zone.GetMaxHealth(),
				zone.GetHealthScaled(),
				zone.GetDamageState()));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendComponents(IEntity ent, array<string> lines)
	{
		lines.Insert("--- COMPONENTS ---");
		array<Managed> comps = {};
		ent.FindComponents(GenericComponent, comps);
		lines.Insert(string.Format("Count: %1", comps.Count()));
		foreach (Managed m : comps)
		{
			if (m)
				lines.Insert(string.Format("  - %1", m.Type().ToString()));
		}
	}
}
