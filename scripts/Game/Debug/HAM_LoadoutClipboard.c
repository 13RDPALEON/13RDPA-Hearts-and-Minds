//------------------------------------------------------------------------------------------------
// 13RDPA — presse-papiers de tenue, même sérialisation que Bacon (Read / Apply loadout string).
//------------------------------------------------------------------------------------------------

class HAM_LoadoutClipboard
{
	protected static string s_sData;
	protected static string s_sSourceName;
	protected static IEntity s_PasteTarget;

	static void SetRaw(string data, string sourceName)
	{
		s_sData = data;
		s_sSourceName = sourceName;
	}

	static void Copy(IEntity character)
	{
		if (!character)
			return;

		JsonSaveContext saveContext = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(character, saveContext))
		{
			Print("[13RDPA H&M] Copier stuff : lecture de la tenue échouée", LogLevel.ERROR);
			return;
		}

		s_sData = saveContext.SaveToString();
		s_sSourceName = character.GetName();
		Print(string.Format("[13RDPA H&M] Stuff copié depuis %1", s_sSourceName), LogLevel.NORMAL);
	}

	static void Paste(IEntity character)
	{
		if (!character)
			return;

		if (s_sData.IsEmpty())
		{
			Print("[13RDPA H&M] Coller stuff : rien en mémoire. Copie d'abord un personnage.", LogLevel.WARNING);
			return;
		}

		s_PasteTarget = character;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedDeliberate, true);

		GetGame().GetCallqueue().CallLater(PasteDelayed, 150, false);
	}

	protected static void PasteDelayed()
	{
		IEntity character = s_PasteTarget;
		s_PasteTarget = null;
		if (!character || s_sData.IsEmpty())
			return;

		GameEntity ent = GameEntity.Cast(character);
		if (!ent)
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(s_sData))
		{
			Print("[13RDPA H&M] Coller stuff : tenue illisible", LogLevel.ERROR);
			return;
		}

		SCR_PlayerArsenalLoadout.ApplyLoadoutString(ent, ctx);
		Print(string.Format("[13RDPA H&M] Stuff collé sur %1 (source %2)", character.GetName(), s_sSourceName), LogLevel.NORMAL);
	}

	static IEntity FindEntity(RplId id)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(id));
		if (!rpl)
			return null;

		return rpl.GetEntity();
	}
}

modded class SCR_PlayerController
{
	void HAM_CopyLoadout(RplId id)
	{
		if (Replication.IsServer())
		{
			HAM_CopyLoadoutServer(id);
			return;
		}

		Rpc(HAM_CopyLoadoutServer, id);
	}

	void HAM_PasteLoadout(RplId id)
	{
		if (Replication.IsServer())
		{
			HAM_PasteLoadoutServer(id);
			return;
		}

		Rpc(HAM_PasteLoadoutServer, id);
	}

	// Le menu n'existe que dans l'éditeur, mais un client peut envoyer le RPC lui-même.
	protected bool HAM_IsFullGameMaster()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editor = core.GetEditorManager(GetPlayerId());
		return editor && !editor.IsLimited();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_CopyLoadoutServer(RplId id)
	{
		if (!HAM_IsFullGameMaster())
			return;

		HAM_LoadoutClipboard.Copy(HAM_LoadoutClipboard.FindEntity(id));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_PasteLoadoutServer(RplId id)
	{
		if (!HAM_IsFullGameMaster())
			return;

		HAM_LoadoutClipboard.Paste(HAM_LoadoutClipboard.FindEntity(id));
	}
}
