//------------------------------------------------------------------------------------------------
// Canaux radio. On ne crée pas d'émetteur : une liste de fréquences règle celui du préfab.
// Pav. numérique 1 ajoute un canal, Pav. numérique 2 retire le dernier ajouté.
// Le fichier ne garde que le dernier réglage de chaque joueur et de chaque radio.
//------------------------------------------------------------------------------------------------

class HAM_RadioChannels
{
	protected static const string FILE = "$profile:13RDPA_HeartsAndMinds/RadioChannels.json";
	protected static const string RADIO_DIR = "$profile:13RDPA_HeartsAndMinds/Radio/";
	protected static const string RADIO_INDEX = "$profile:13RDPA_HeartsAndMinds/Radio/joueurs.json";
	// Comme les radios du Conflit : 0,1 à 100 MHz par pas de 0,1, 20 km, une seule clé (celle de la faction joueur).
	protected static const int MIN_KHZ = 100;
	protected static const int MAX_KHZ = 100000;
	protected static const int STEP_KHZ = 100;
	// A / E : un saut de 5,0 MHz (5 000 kHz), pas le pas fin de 0,1.
	static const int COARSE_KHZ = 5000;
	protected static const float RADIO_RANGE = 20000;
	// Poste d'écoute du canal ajouté. Le poste du joueur n'est pas retouché.
	protected static const ResourceName LISTEN_RADIO = "{508AB2013EEE1E00}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et";
	protected static const int MAX_CREATED = 32;

	protected static bool s_bStarted;
	protected static bool s_bBound;
	protected static bool s_bApplying;
	protected static bool s_bDirty;
	protected static bool s_bSaveQueued;
	protected static ref array<string> s_aIds;
	protected static ref array<string> s_aPrefabs;
	protected static ref array<int> s_aActive;
	protected static ref array<int> s_aHardware;
	protected static ref array<string> s_aFreqs;
	protected static ref array<string> s_aRanges;
	protected static ref array<IEntity> s_aSeen;
	protected static ref array<float> s_aQuietUntil;
	protected static ref array<string> s_aIndexed;

	static void Start()
	{
		if (s_bStarted)
			return;

		s_bStarted = true;
		s_aIds = {};
		s_aPrefabs = {};
		s_aActive = {};
		s_aHardware = {};
		s_aFreqs = {};
		s_aRanges = {};
		s_aSeen = {};
		s_aQuietUntil = {};

		if (Replication.IsServer())
		{
			Load();
			// 5 s : la découverte des radios n'a pas besoin d'être instantanée (avant : 500 ms puis 2 s).
			GetGame().GetCallqueue().CallLater(Poll, 5000, true);
		}

		BindLocal();
	}

	static void BindLocal()
	{
		if (s_bBound)
			return;

		InputManager input = GetGame().GetInputManager();
		if (!input)
			return;

		s_bBound = true;
		input.AddActionListener("HAM_RadioChannelAdd", EActionTrigger.DOWN, OnAddPressed);
		input.AddActionListener("HAM_RadioChannelRemove", EActionTrigger.DOWN, OnRemovePressed);
	}

	static void NotifyItem(IEntity owner, IEntity item)
	{
		if (!Replication.IsServer() || !owner || !item)
			return;

		Start();

		if (!item.FindComponent(BaseRadioComponent))
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		int playerId = players.GetPlayerIdFromControlledEntity(owner);
		if (playerId == 0)
			return;

		SeeRadio(playerId, item);
	}

	protected static void OnAddPressed()
	{
		if (!MenuOpen())
			return;

		int playerId = SCR_PlayerController.GetLocalPlayerId();
		if (playerId == 0)
			return;

		if (Replication.IsServer())
		{
			AddForPlayer(playerId);
			return;
		}

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_RequestRadioAdd();
	}

	protected static void OnRemovePressed()
	{
		if (!MenuOpen())
			return;

		int playerId = SCR_PlayerController.GetLocalPlayerId();
		if (playerId == 0)
			return;

		if (Replication.IsServer())
		{
			RemoveForPlayer(playerId);
			return;
		}

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_RequestRadioRemove();
	}

	static void AddForPlayer(int playerId)
	{
		if (!Replication.IsServer())
			return;

		Start();

		IEntity radio = FindRadio(playerId);
		if (!radio)
			return;

		int row = EnsureRow(playerId, radio);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		int created = freqs.Count() - s_aHardware[row];
		if (created >= MAX_CREATED)
			return;

		int current = Snap(KhzFromRaw(LiveFreq(radio, 0)));
		if (s_aActive[row] >= 0 && s_aActive[row] < freqs.Count())
			current = freqs[s_aActive[row]];

		// Nouveau canal = fréquence libre (évite le doublon qui écrase l'ancien au réglage).
		current = NextFreeFreq(freqs, Snap(current));
		freqs.Insert(current);
		s_aActive[row] = freqs.Count() - 1;
		s_aFreqs[row] = WriteFreqs(freqs);
		s_bDirty = true;
		QuietRadio(radio, 900);
		ApplyRow(radio, row);
		Save();
		Tell(playerId, s_aActive[row], current);
		PushSync(playerId, row);
		GetGame().GetCallqueue().CallLater(EnsureWheel, 0, false);
	}

	static void RemoveForPlayer(int playerId)
	{
		if (!Replication.IsServer())
			return;

		Start();

		IEntity radio = FindRadio(playerId);
		if (!radio)
			return;

		int row = EnsureRow(playerId, radio);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (freqs.Count() <= s_aHardware[row])
			return;

		freqs.Remove(freqs.Count() - 1);
		if (s_aActive[row] >= freqs.Count())
			s_aActive[row] = freqs.Count() - 1;

		s_aFreqs[row] = WriteFreqs(freqs);
		s_bDirty = true;
		ApplyRow(radio, row);
		Save();
		Tell(playerId, s_aActive[row], freqs[s_aActive[row]]);
		PushSync(playerId, row);
		GetGame().GetCallqueue().CallLater(EnsureWheel, 0, false);
	}

	protected static void Poll()
	{
		if (!Replication.IsServer() || s_bApplying)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		bool changed = false;
		foreach (int playerId : ids)
		{
			IEntity character = players.GetPlayerControlledEntity(playerId);
			if (!character)
				continue;

			array<IEntity> radios = {};
			CollectRadios(character, radios);
			foreach (IEntity radio : radios)
			{
				if (SeenIndex(radio) < 0)
				{
					SeeRadio(playerId, radio);
					continue;
				}

				if (AbsorbLive(playerId, radio))
					changed = true;
			}
		}

		if (changed)
			Save();
	}

	protected static void SeeRadio(int playerId, IEntity radio)
	{
		int seen = SeenIndex(radio);
		if (seen >= 0)
			return;

		int row = EnsureRow(playerId, radio);
		if (row < 0)
			return;

		s_aSeen.Insert(radio);
		float until = Now() + 700;
		s_aQuietUntil.Insert(until);
		Standardize(playerId, radio);
		ApplyRow(radio, row);
		GetGame().GetCallqueue().CallLater(Recheck, 650, false, radio, playerId);
		PushSync(playerId, row);
	}

	protected static void Recheck(IEntity radio, int playerId)
	{
		if (!Replication.IsServer() || !radio)
			return;

		int row = EnsureRow(playerId, radio);
		if (row < 0)
			return;

		Standardize(playerId, radio);
		ApplyRow(radio, row);
	}

	//------------------------------------------------------------------------------------------------
	//! Radio d'un autre mod (non remplacée dans Prefabs) : portée et clé alignées, plage et pas restent ceux du prefab.
	protected static void Standardize(int playerId, IEntity radio)
	{
		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		if (!component)
			return;

		Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
		if (faction)
		{
			string key = faction.GetFactionRadioEncryptionKey();
			if (!key.IsEmpty() && component.GetEncryptionKey() != key)
				component.SetEncryptionKey(key);
		}

		int count = component.TransceiversCount();
		for (int i = 0; i < count; i++)
		{
			BaseTransceiver transceiver = component.GetTransceiver(i);
			if (transceiver && transceiver.GetRange() < RADIO_RANGE)
				transceiver.SetRange(RADIO_RANGE);
		}
	}

	protected static bool AbsorbLive(int playerId, IEntity radio)
	{
		int seen = SeenIndex(radio);
		if (seen < 0)
			return false;

		if (Now() < s_aQuietUntil[seen])
			return false;

		int row = EnsureRow(playerId, radio);
		if (row < 0)
			return false;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		bool changed = false;
		int hardware = s_aHardware[row];
		int active = s_aActive[row];

		if (active >= hardware)
		{
			// Canal créé actif : TX0 = freq créée. Ne pas écraser freqs[0].
			for (int i = 1; i < hardware; i++)
			{
				if (i >= freqs.Count())
					break;

				int live = Snap(KhzFromRaw(LiveFreq(radio, i)));
				if (live == freqs[i])
					continue;

				freqs[i] = live;
				changed = true;
			}

			if (active < freqs.Count())
			{
				int talk = Snap(KhzFromRaw(LiveFreq(radio, 0)));
				if (talk != freqs[active])
				{
					freqs[active] = talk;
					changed = true;
				}
			}
		}
		else
		{
			// Hardware : chaque TX garde sa propre freq (pas TX0 → slot actif).
			for (int i = 0; i < hardware; i++)
			{
				if (i >= freqs.Count())
					break;

				int live = Snap(KhzFromRaw(LiveFreq(radio, i)));
				if (live == freqs[i])
					continue;

				freqs[i] = live;
				changed = true;
			}
		}

		if (!changed)
			return false;

		s_aFreqs[row] = WriteFreqs(freqs);
		s_bDirty = true;
		return true;
	}

	protected static int PlayerOfRadio(IEntity radio)
	{
		if (!radio)
			return 0;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return 0;

		IEntity body = radio.GetRootParent();
		if (!body)
			body = radio;

		return players.GetPlayerIdFromControlledEntity(body);
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity FindListenRadio(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return null;

		IEntity character = players.GetPlayerControlledEntity(playerId);
		if (!character)
			return null;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (inventory)
		{
			array<IEntity> items = {};
			inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
			foreach (IEntity item : items)
			{
				SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
				if (gadget && gadget.HAM_IsListenOnly())
					return item;
			}
		}

		IEntity child = character.GetChildren();
		while (child)
		{
			SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(child.FindComponent(SCR_GadgetComponent));
			if (gadget && gadget.HAM_IsListenOnly())
				return child;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Second poste, même préfab, uniquement pour le canal ajouté. Les émetteurs d'origine ne bougent pas.
	static IEntity OpenListenRadio(int playerId, IEntity main, int khz)
	{
		if (!main || playerId <= 0)
			return null;

		IEntity listen = FindListenRadio(playerId);
		if (!listen && Replication.IsServer())
			listen = SpawnListenRadio(playerId, main);

		if (!listen)
			return null;

		TuneListenRadio(listen, main, khz, true);
		return listen;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CloseListenRadio(int playerId)
	{
		IEntity listen = FindListenRadio(playerId);
		if (!listen)
			return;

		BaseRadioComponent radio = BaseRadioComponent.Cast(listen.FindComponent(BaseRadioComponent));
		if (radio)
			radio.SetPower(false);
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnListenRadio(int playerId, IEntity main)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return null;

		IEntity character = players.GetPlayerControlledEntity(playerId);
		if (!character)
			return null;

		Resource resource = Resource.Load(LISTEN_RADIO);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = character.GetOrigin();
		IEntity spawned = GetGame().SpawnEntityPrefab(resource, character.GetWorld(), params);
		if (!spawned)
			return null;

		SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(spawned.FindComponent(SCR_GadgetComponent));
		if (gadget)
			gadget.HAM_MarkListenOnly();

		spawned.SetFlags(EntityFlags.VISIBLE, false);
		Physics phys = spawned.GetPhysics();
		if (phys)
			phys.SetActive(ActiveState.INACTIVE);

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory || !inventory.TryInsertItem(spawned, EStoragePurpose.PURPOSE_ANY))
			character.AddChild(spawned, -1);

		return spawned;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TuneListenRadio(IEntity listen, IEntity main, int khz, bool powered)
	{
		BaseRadioComponent radio = BaseRadioComponent.Cast(listen.FindComponent(BaseRadioComponent));
		BaseRadioComponent source = BaseRadioComponent.Cast(main.FindComponent(BaseRadioComponent));
		if (!radio)
			return;

		if (source)
			radio.SetEncryptionKey(source.GetEncryptionKey());

		radio.SetPower(powered);
		int count = radio.TransceiversCount();
		for (int i = 0; i < count; i++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(i);
			if (!transceiver)
				continue;

			if (i == 0)
			{
				int sample = transceiver.GetFrequency();
				transceiver.SetFrequency(ScaleFreq(sample, khz));
				transceiver.SetRange(RADIO_RANGE);
				continue;
			}

			transceiver.SetRange(0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyRow(IEntity radio, int row)
	{
		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		if (!component)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		int hardware = s_aHardware[row];
		int active = s_aActive[row];
		if (active < 0 || active >= freqs.Count())
			active = 0;

		int sample = LiveFreq(radio, 0);
		s_bApplying = true;

		// Canal créé : un second poste écoute cette fréquence. Les émetteurs du poste
		// du joueur restent sur les canaux de base.
		if (active >= hardware && active < freqs.Count())
		{
			int ownerId = PlayerOfRadio(radio);
			if (ownerId > 0)
				OpenListenRadio(ownerId, radio, freqs[active]);

			s_bApplying = false;
			return;
		}

		int ownerId = PlayerOfRadio(radio);
		if (ownerId > 0)
			CloseListenRadio(ownerId);

		for (int i = 0; i < hardware; i++)
		{
			if (i >= freqs.Count())
				break;

			BaseTransceiver transceiver = component.GetTransceiver(i);
			if (!transceiver)
				continue;

			transceiver.SetFrequency(ScaleFreq(sample, freqs[i]));
			transceiver.SetRange(RADIO_RANGE);
		}

		s_bApplying = false;
	}

	protected static int EnsureRow(int playerId, IEntity radio)
	{
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		if (HAM_GroupAccessHelper.IsInvalidIdentityString(identity))
			return -1;

		string prefab = PrefabOf(radio);
		if (prefab.IsEmpty())
			return -1;

		int hardware = CountTransceivers(radio);
		if (hardware < 1)
			return -1;

		int row = FindRow(identity, prefab);
		if (row >= 0)
			return row;

		array<int> freqs = {};
		array<int> ranges = {};
		CaptureHardware(radio, hardware, freqs, ranges);
		s_aIds.Insert(identity);
		s_aPrefabs.Insert(prefab);
		s_aActive.Insert(0);
		s_aHardware.Insert(hardware);
		s_aFreqs.Insert(WriteFreqs(freqs));
		s_aRanges.Insert(WriteFreqs(ranges));
		s_bDirty = true;
		Save();
		return s_aIds.Count() - 1;
	}

	protected static void CaptureHardware(IEntity radio, int hardware, array<int> freqs, array<int> ranges)
	{
		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		for (int i = 0; i < hardware; i++)
		{
			int freq = MIN_KHZ;
			int rangeMeters = 0;
			if (component)
			{
				BaseTransceiver transceiver = component.GetTransceiver(i);
				if (transceiver)
				{
					freq = Snap(KhzFromRaw(transceiver.GetFrequency()));
					float liveRange = transceiver.GetRange();
					ranges.Insert(Math.Floor(liveRange));
					freqs.Insert(freq);
					continue;
				}
			}

			freqs.Insert(freq);
			ranges.Insert(rangeMeters);
		}
	}

	protected static IEntity FindRadio(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return null;

		IEntity character = players.GetPlayerControlledEntity(playerId);
		if (!character)
			return null;

		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(character.FindComponent(SCR_GadgetManagerComponent));
		if (gadgets)
		{
			IEntity held = gadgets.GetHeldGadget();
			if (held && held.FindComponent(BaseRadioComponent))
				return held;
		}

		array<IEntity> radios = {};
		CollectRadios(character, radios);
		if (radios.IsEmpty())
			return null;

		return radios[0];
	}

	protected static void CollectRadios(IEntity character, array<IEntity> radios)
	{
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : items)
		{
			if (!item || !item.FindComponent(BaseRadioComponent))
				continue;

			SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(item.FindComponent(SCR_GadgetComponent));
			if (gadget && gadget.HAM_IsListenOnly())
				continue;

			if (radios.Find(item) < 0)
				radios.Insert(item);
		}
	}

	protected static bool MenuOpen()
	{
		InputManager input = GetGame().GetInputManager();
		bool menu = false;
		if (input)
		{
			if (input.IsContextActive("MenuContext"))
				menu = true;

			if (input.IsContextActive("RadialMenuContext"))
				menu = true;

			if (input.IsContextActive("VONContext"))
				menu = true;

			if (input.IsContextActive("RadioContext"))
				menu = true;
		}

		int playerId = SCR_PlayerController.GetLocalPlayerId();
		if (!menu)
		{
			PlayerManager players = GetGame().GetPlayerManager();
			if (!players)
				return false;

			IEntity character = players.GetPlayerControlledEntity(playerId);
			if (!character)
				return false;

			SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(character.FindComponent(SCR_GadgetManagerComponent));
			if (!gadgets)
				return false;

			IEntity held = gadgets.GetHeldGadget();
			if (!held || !held.FindComponent(BaseRadioComponent))
				return false;

			return true;
		}

		return FindRadio(playerId) != null;
	}

	protected static int CountTransceivers(IEntity radio)
	{
		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		if (!component)
			return 0;

		int count = 0;
		for (int i = 0; i < 8; i++)
		{
			if (!component.GetTransceiver(i))
				break;

			count = count + 1;
		}

		return count;
	}

	protected static int LiveFreq(IEntity radio, int index)
	{
		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		if (!component)
			return MIN_KHZ;

		BaseTransceiver transceiver = component.GetTransceiver(index);
		if (!transceiver)
			return MIN_KHZ;

		return transceiver.GetFrequency();
	}

	static int KhzOf(int raw)
	{
		return KhzFromRaw(raw);
	}

	protected static int KhzFromRaw(int raw)
	{
		if (raw >= 1000000)
			return raw / 1000;

		return raw;
	}

	protected static int ScaleFreq(int sample, int khz)
	{
		if (sample >= 1000000)
			return khz * 1000;

		return khz;
	}

	protected static int Snap(int freq)
	{
		if (freq < MIN_KHZ)
			freq = MIN_KHZ;
		if (freq > MAX_KHZ)
			freq = MAX_KHZ;

		int delta = freq - MIN_KHZ;
		int steps = delta / STEP_KHZ;
		int rem = delta - steps * STEP_KHZ;
		if (rem >= 50)
			steps = steps + 1;

		int snapped = MIN_KHZ + steps * STEP_KHZ;
		if (snapped > MAX_KHZ)
			snapped = MAX_KHZ;

		return snapped;
	}

	protected static int IndexOfFreq(array<int> freqs, int freq, int skip)
	{
		for (int i = 0; i < freqs.Count(); i++)
		{
			if (i == skip)
				continue;

			if (freqs[i] == freq)
				return i;
		}

		return -1;
	}

	protected static int NextFreeFreq(array<int> freqs, int start)
	{
		int candidate = Snap(start + STEP_KHZ);
		for (int n = 0; n < 200; n++)
		{
			if (candidate > MAX_KHZ)
				candidate = MIN_KHZ;

			if (IndexOfFreq(freqs, candidate, -1) < 0)
				return candidate;

			candidate = Snap(candidate + STEP_KHZ);
		}

		return Snap(start);
	}

	protected static void QuietRadio(IEntity radio, int ms)
	{
		if (!radio)
			return;

		int seen = SeenIndex(radio);
		if (seen < 0)
			return;

		float until = Now() + ms;
		if (until > s_aQuietUntil[seen])
			s_aQuietUntil[seen] = until;
	}

	protected static int FindRow(string identity, string prefab)
	{
		for (int i = 0; i < s_aIds.Count(); i++)
		{
			if (s_aIds[i] != identity)
				continue;

			if (s_aPrefabs[i] == prefab)
				return i;
		}

		return -1;
	}

	protected static int SeenIndex(IEntity radio)
	{
		if (!s_aSeen)
			return -1;

		for (int i = s_aSeen.Count() - 1; i >= 0; i--)
		{
			if (!s_aSeen[i])
			{
				s_aSeen.Remove(i);
				s_aQuietUntil.Remove(i);
				continue;
			}

			if (s_aSeen[i] == radio)
				return i;
		}

		return -1;
	}

	protected static string PrefabOf(IEntity radio)
	{
		if (!radio || !radio.GetPrefabData())
			return "";

		return radio.GetPrefabData().GetPrefabName();
	}

	protected static string WriteFreqs(array<int> freqs)
	{
		string packed = "";
		foreach (int freq : freqs)
		{
			if (!packed.IsEmpty())
				packed = packed + ",";

			packed = packed + freq.ToString();
		}

		return packed;
	}

	protected static void ReadFreqs(string packed, array<int> freqs)
	{
		freqs.Clear();
		string rest = packed;
		while (!rest.IsEmpty())
		{
			int comma = rest.IndexOf(",");
			string part = rest;
			if (comma >= 0)
			{
				part = rest.Substring(0, comma);
				rest = rest.Substring(comma + 1, rest.Length() - comma - 1);
			}
			else
			{
				rest = "";
			}

			if (!part.IsEmpty())
				freqs.Insert(part.ToInt());
		}
	}

	protected static float Now()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime();
	}

	protected static void Tell(int playerId, int index, int freq)
	{
		int shown = index + 1;
		int whole = freq / 1000;
		int frac = (freq - whole * 1000) / 100;
		string text = "Canal " + shown.ToString() + " : " + whole.ToString() + "." + frac.ToString();
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (player)
			player.HAM_ShowAdminNote(text);
	}

	protected static void PushSync(int playerId, int row)
	{
		if (row < 0)
			return;

		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (!player)
			return;

		player.HAM_SyncRadio(s_aPrefabs[row], s_aFreqs[row], s_aHardware[row], s_aActive[row]);
	}

	static void ApplyClientSync(string prefab, string freqs, int hardware, int active)
	{
		if (!s_aIds)
		{
			s_aIds = {};
			s_aPrefabs = {};
			s_aActive = {};
			s_aHardware = {};
			s_aFreqs = {};
			s_aRanges = {};
		}

		int row = FindRow("local", prefab);
		if (row < 0)
		{
			s_aIds.Insert("local");
			s_aPrefabs.Insert(prefab);
			s_aActive.Insert(active);
			s_aHardware.Insert(hardware);
			s_aFreqs.Insert(freqs);
			s_aRanges.Insert("");
		}
		else
		{
			s_aActive[row] = active;
			s_aHardware[row] = hardware;
			s_aFreqs[row] = freqs;
		}

		if (active >= hardware)
		{
			array<int> parsed = {};
			ReadFreqs(freqs, parsed);
			int playerId = SCR_PlayerController.GetLocalPlayerId();
			IEntity radio = FindRadio(playerId);
			if (radio && active < parsed.Count())
				OpenListenRadio(playerId, radio, parsed[active]);

			RefreshOpenWheel(prefab);
			return;
		}

		int created = CreatedCount(prefab);
		int shown = 0;
		if (s_aWheel)
			shown = s_aWheel.Count();

		if (shown != created)
			RefreshOpenWheel(prefab);
		else
			PaintWheel(prefab);
	}

	static void TuneCreated(int playerId, string prefab, int index, int khz)
	{
		if (!Replication.IsServer())
			return;

		Start();
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		int row = FindRow(identity, prefab);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (index < 0 || index >= freqs.Count())
			return;

		int hardware = s_aHardware[row];
		freqs[index] = Snap(khz);
		s_aFreqs[row] = WriteFreqs(freqs);
		s_aActive[row] = index;
		s_bDirty = true;

		IEntity radio = FindRadio(playerId);
		if (radio && PrefabOf(radio) == prefab)
		{
			QuietRadio(radio, 900);
			// Hardware : le client a déjà réglé le TX — ne pas ApplyRow (sinon snap / blocage).
			// Créé : TX0 doit suivre la freq du slot créé.
			if (index >= hardware)
				ApplyRow(radio, row);
		}

		Save();
		PushSync(playerId, row);
	}

	static void SelectIndex(int playerId, string prefab, int index)
	{
		if (!Replication.IsServer())
			return;

		Start();
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		int row = FindRow(identity, prefab);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (index < 0 || index >= freqs.Count())
			return;

		if (s_aActive[row] == index)
			return;

		s_aActive[row] = index;
		s_bDirty = true;
		IEntity radio = FindRadio(playerId);
		if (radio && PrefabOf(radio) == prefab)
		{
			QuietRadio(radio, 900);
			// Toujours ApplyRow : quitter un canal créé doit remettre CH1/CH2
			// (sinon TX0 reste sur la freq perso → « CH1 saute » sur le custom).
			ApplyRow(radio, row);
		}

		Save();
		PushSync(playerId, row);
	}

	static void ApplyTalk(string prefab, int khz, bool createdRange)
	{
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		IEntity radio = FindRadio(playerId);
		if (!radio || PrefabOf(radio) != prefab)
			return;

		BaseRadioComponent component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
		if (!component)
			return;

		int ownerId = PlayerOfRadio(radio);
		if (ownerId <= 0)
			return;

		IEntity listen = OpenListenRadio(ownerId, radio, khz);
		if (!listen)
			return;

		BaseRadioComponent extra = BaseRadioComponent.Cast(listen.FindComponent(BaseRadioComponent));
		if (!extra)
			return;

		BaseTransceiver talk = extra.GetTransceiver(0);
		if (!talk)
			return;

		int sample = talk.GetFrequency();
		talk.SetFrequency(ScaleFreq(sample, khz));
		talk.SetRange(RADIO_RANGE);
		extra.SetPower(true);
	}

	static void UseCreated(string prefab, int index, int khz)
	{
		ApplyTalk(prefab, khz, true);
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		if (Replication.IsServer())
			SelectIndex(player.GetPlayerId(), prefab, index);
		else
			player.HAM_RequestSelect(prefab, index);
	}

	static void ReleaseCreated(string prefab)
	{
		// Ne jamais ramener le canal 0 ici : SetActive(false) arrive aussi au rebuild
		// et à chaque changement d'entrée. L'ancien canal était écrasé via AbsorbLive.
		// Le retour hardware se fait uniquement via SelectHardwareEntry (clic CH1…).
	}

	//------------------------------------------------------------------------------------------------
	// A / E : fréquence suivante à ±5,0 MHz. L'unité du préfab peut être le kHz ou le Hz.
	//------------------------------------------------------------------------------------------------
	static int NextCoarse(int freq, int modifier, int minFreq, int maxFreq)
	{
		int step = COARSE_KHZ;
		int floor = MIN_KHZ;
		int ceil = MAX_KHZ;
		if (freq >= 1000000)
		{
			step = COARSE_KHZ * 1000;
			floor = MIN_KHZ * 1000;
			ceil = MAX_KHZ * 1000;
		}

		if (minFreq < floor)
			minFreq = floor;
		if (maxFreq <= minFreq || maxFreq > ceil)
			maxFreq = ceil;

		int next = freq + modifier * step;
		if (next < minFreq)
			next = minFreq;
		if (next > maxFreq)
			next = maxFreq;

		return next;
	}

	//------------------------------------------------------------------------------------------------
	// Vrai si le micro est sur un canal créé : A/E sur CH1 ne doit pas écraser cette fréquence.
	//------------------------------------------------------------------------------------------------
	static bool CreatedTalkActive(SCR_VONEntryRadio entry)
	{
		if (!entry || HAM_VONChannelEntry.Cast(entry))
			return false;

		BaseTransceiver transceiver = entry.GetTransceiver();
		if (!transceiver)
			return false;

		IEntity radioEnt = transceiver.GetRadio().GetOwner();
		if (!radioEnt)
			return false;

		int row = LocalRow(PrefabOf(radioEnt));
		if (row < 0)
			return false;

		return s_aActive[row] >= s_aHardware[row];
	}

	//------------------------------------------------------------------------------------------------
	// Entrées VON hardware : figer l'affichage SEULEMENT si un canal créé est l'actif.
	// Sinon on efface l'overwrite — sinon CH1 reste coincé sur l'ancienne valeur visuelle.
	//------------------------------------------------------------------------------------------------
	static void OverlayHardwareEntry(SCR_VONEntryRadio entry)
	{
		if (!entry || HAM_VONChannelEntry.Cast(entry))
			return;

		BaseTransceiver transceiver = entry.GetTransceiver();
		if (!transceiver)
			return;

		IEntity radio = transceiver.GetRadio().GetOwner();
		if (!radio)
			return;

		string prefab = PrefabOf(radio);
		int row = LocalRow(prefab);
		if (row < 0)
		{
			entry.SetFrequencyTextOverwrite(string.Empty);
			return;
		}

		int hardware = s_aHardware[row];
		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (freqs.Count() <= hardware)
		{
			entry.SetFrequencyTextOverwrite(string.Empty);
			return;
		}

		// Canal hardware actif : laisser le vanilla afficher le TX live.
		if (s_aActive[row] < hardware)
		{
			entry.SetFrequencyTextOverwrite(string.Empty);
			return;
		}

		int slot = entry.GetTransceiverNumber() - 1;
		if (slot < 0 || slot >= hardware || slot >= freqs.Count())
		{
			entry.SetFrequencyTextOverwrite(string.Empty);
			return;
		}

		int khz = freqs[slot];
		float shown = Math.Round(khz * 0.1) * 0.01;
		entry.SetFrequencyTextOverwrite(shown.ToString(3, 1) + " " + "#AR-VON_FrequencyUnits_MHz");
		entry.SetChannelText(SCR_VONMenu.GetKnownChannel(khz));
	}

	static bool HandleHardwareScroll(SCR_VONEntryRadio entry, int modifier)
	{
		if (!entry || HAM_VONChannelEntry.Cast(entry))
			return false;

		BaseTransceiver transceiver = entry.GetTransceiver();
		if (!transceiver)
			return false;

		IEntity radioEnt = transceiver.GetRadio().GetOwner();
		if (!radioEnt)
			return false;

		string prefab = PrefabOf(radioEnt);
		int row = LocalRow(prefab);
		if (row < 0)
			return false;

		int hardware = s_aHardware[row];
		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (freqs.Count() <= hardware)
			return false;

		// Canal créé actif : CH1/CH2 ne doivent ni scroller le TX0 (freq créée),
		// ni enregistrer — le réglage se fait sur l'entrée créée.
		if (s_aActive[row] >= hardware)
		{
			OverlayHardwareEntry(entry);
			return true;
		}

		return false;
	}

	static void SelectHardwareEntry(SCR_VONEntryRadio entry)
	{
		if (!entry || HAM_VONChannelEntry.Cast(entry))
			return;

		BaseTransceiver transceiver = entry.GetTransceiver();
		if (!transceiver)
			return;

		IEntity radioEnt = transceiver.GetRadio().GetOwner();
		if (!radioEnt)
			return;

		string prefab = PrefabOf(radioEnt);
		int row = LocalRow(prefab);
		if (row < 0)
			return;

		int slot = entry.GetTransceiverNumber() - 1;
		if (slot < 0)
			slot = 0;

		if (slot >= s_aHardware[row])
			slot = 0;

		// Immédiat côté client : restaurer les TX hardware avant le round-trip serveur,
		// sinon l'affichage / le réglage gardent la freq du canal créé sur TX0.
		s_aActive[row] = slot;
		QuietRadio(radioEnt, 900);
		ApplyRow(radioEnt, row);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		if (Replication.IsServer())
			SelectIndex(player.GetPlayerId(), prefab, slot);
		else
			player.HAM_RequestSelect(prefab, slot);

		OverlayHardwareEntry(entry);
	}

	static void AfterHardwareScroll(SCR_VONEntryRadio entry)
	{
		if (!entry || HAM_VONChannelEntry.Cast(entry))
			return;

		BaseTransceiver transceiver = entry.GetTransceiver();
		if (!transceiver)
			return;

		IEntity radioEnt = transceiver.GetRadio().GetOwner();
		if (!radioEnt)
			return;

		string prefab = PrefabOf(radioEnt);
		int row = LocalRow(prefab);
		if (row < 0)
			return;

		if (s_aActive[row] >= s_aHardware[row])
			return;

		int slot = entry.GetTransceiverNumber() - 1;
		if (slot < 0 || slot >= s_aHardware[row])
			return;

		int khz = Snap(KhzFromRaw(transceiver.GetFrequency()));
		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		if (slot < freqs.Count())
		{
			freqs[slot] = khz;
			s_aFreqs[row] = WriteFreqs(freqs);
			s_aActive[row] = slot;
		}

		QuietRadio(radioEnt, 900);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		player.HAM_RequestTune(prefab, slot, khz);
	}

	static void EnsureWheel()
	{
		if (!s_Menu)
			return;

		SCR_RadialMenu radial = s_Menu.GetRadialMenu();
		if (!radial || !radial.IsOpened())
			return;

		int playerId = SCR_PlayerController.GetLocalPlayerId();
		IEntity radio = FindRadio(playerId);
		if (!radio)
			return;

		string prefab = PrefabOf(radio);
		int created = CreatedCount(prefab);
		int shown = 0;
		if (s_aWheel)
			shown = s_aWheel.Count();

		if (shown == created)
			return;

		RefreshOpenWheel(prefab);
	}

	protected static int LocalRow(string prefab)
	{
		if (!s_aIds)
			return -1;

		if (Replication.IsServer())
		{
			int playerId = SCR_PlayerController.GetLocalPlayerId();
			string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
			int serverRow = FindRow(identity, prefab);
			if (serverRow >= 0)
				return serverRow;
		}

		return FindRow("local", prefab);
	}

	protected static int CreatedCount(string prefab)
	{
		int row = LocalRow(prefab);
		if (row < 0)
			return 0;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		int extra = freqs.Count() - s_aHardware[row];
		if (extra < 0)
			return 0;

		return extra;
	}

	static void ForgetWheel()
	{
		if (s_aWheel)
			s_aWheel.Clear();
	}

	protected static void PaintWheel(string prefab)
	{
		if (!s_aWheel)
			return;

		int row = LocalRow(prefab);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		foreach (SCR_VONEntry entry : s_aWheel)
		{
			HAM_VONChannelEntry created = HAM_VONChannelEntry.Cast(entry);
			if (!created)
				continue;

			int index = created.GetIndex();
			if (index < 0 || index >= freqs.Count())
				continue;

			created.SetKhz(freqs[index]);
		}
	}

	protected static void RebuildWheel(string prefab)
	{
		if (!s_Menu)
			return;

		SCR_RadialMenu radial = s_Menu.GetRadialMenu();
		if (!radial)
			return;

		if (!s_aWheel)
			s_aWheel = {};

		int row = LocalRow(prefab);
		if (row < 0)
			return;

		array<int> freqs = {};
		ReadFreqs(s_aFreqs[row], freqs);
		int hardware = s_aHardware[row];
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		IEntity radio = FindRadio(playerId);
		BaseRadioComponent component;
		SCR_GadgetComponent gadget;
		if (radio)
		{
			component = BaseRadioComponent.Cast(radio.FindComponent(BaseRadioComponent));
			gadget = SCR_GadgetComponent.Cast(radio.FindComponent(SCR_GadgetComponent));
		}

		if (!component || !gadget)
			return;

		BaseTransceiver transceiver = component.GetTransceiver(0);
		IEntity listen = FindListenRadio(playerId);
		SCR_GadgetComponent listenGadget;
		if (listen)
		{
			BaseRadioComponent listenRadio = BaseRadioComponent.Cast(listen.FindComponent(BaseRadioComponent));
			if (listenRadio && listenRadio.GetTransceiver(0))
			{
				transceiver = listenRadio.GetTransceiver(0);
				listenGadget = SCR_GadgetComponent.Cast(listen.FindComponent(SCR_GadgetComponent));
			}
		}

		if (!transceiver)
			return;

		if (listenGadget)
			gadget = listenGadget;

		for (int i = hardware; i < freqs.Count(); i++)
		{
			HAM_VONChannelEntry entry = new HAM_VONChannelEntry();
			entry.Setup(prefab, i, freqs[i], transceiver, gadget);
			s_Menu.AddRadialEntry(entry);
			s_aWheel.Insert(entry);
		}
	}

	protected static void RefreshOpenWheel(string prefab)
	{
		if (!s_Menu)
			return;

		SCR_RadialMenu radial = s_Menu.GetRadialMenu();
		if (!radial || !radial.IsOpened())
			return;

		if (s_aWheel)
		{
			foreach (SCR_VONEntry old : s_aWheel)
			{
				if (old)
					radial.RemoveEntry(old);
			}

			s_aWheel.Clear();
		}

		RebuildWheel(prefab);
		radial.UpdateEntries();
	}

	static void RememberMenu(SCR_VONMenu menu)
	{
		s_Menu = menu;
	}

	protected static SCR_VONMenu s_Menu;
	protected static ref array<SCR_VONEntry> s_aWheel;

	static void Flush(int playerId)
	{
		if (!Replication.IsServer())
			return;

		Start();
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		if (HAM_GroupAccessHelper.IsInvalidIdentityString(identity))
			return;

		WriteIdentity(identity);
		TouchIndex(identity);
	}

	protected static void Save()
	{
		if (!Replication.IsServer() || !s_aIds)
			return;

		s_bDirty = true;
		if (s_bSaveQueued)
			return;

		s_bSaveQueued = true;
		GetGame().GetCallqueue().CallLater(FlushDirty, 2000, false);
	}

	protected static void FlushDirty()
	{
		s_bSaveQueued = false;
		if (!s_bDirty || !Replication.IsServer() || !s_aIds)
			return;

		s_bDirty = false;
		array<string> unique = {};
		foreach (string identity : s_aIds)
		{
			if (HAM_GroupAccessHelper.IsInvalidIdentityString(identity))
				continue;

			if (unique.Find(identity) >= 0)
				continue;

			unique.Insert(identity);
			WriteIdentity(identity);
			TouchIndex(identity);
		}
	}

	protected static void WriteIdentity(string identity)
	{
		array<string> prefabs = {};
		array<int> active = {};
		array<int> hardware = {};
		array<string> freqs = {};
		array<string> ranges = {};
		for (int i = 0; i < s_aIds.Count(); i++)
		{
			if (s_aIds[i] != identity)
				continue;

			prefabs.Insert(s_aPrefabs[i]);
			active.Insert(s_aActive[i]);
			hardware.Insert(s_aHardware[i]);
			freqs.Insert(s_aFreqs[i]);
			ranges.Insert(s_aRanges[i]);
		}

		string folder = RADIO_DIR + "Joueur_" + HAM_ProfileFiles.SafeId(identity) + "/";
		HAM_ProfileFiles.EnsureDir(RADIO_DIR);
		HAM_ProfileFiles.EnsureDir(folder);
		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("id", identity);
		ctx.WriteValue("prefabs", prefabs);
		ctx.WriteValue("active", active);
		ctx.WriteValue("hardware", hardware);
		ctx.WriteValue("freqs", freqs);
		ctx.WriteValue("ranges", ranges);
		HAM_JsonQueue.Add(ctx, folder + "frequences.json");
	}

	protected static void TouchIndex(string identity)
	{
		if (!s_aIndexed)
		{
			s_aIndexed = {};
			if (FileIO.FileExists(RADIO_INDEX))
			{
				JsonLoadContext ctx = new JsonLoadContext();
				if (ctx.LoadFromFile(RADIO_INDEX))
					ctx.ReadValue("ids", s_aIndexed);
			}

			if (!s_aIndexed)
				s_aIndexed = {};
		}

		if (s_aIndexed.Find(identity) < 0)
			s_aIndexed.Insert(identity);

		HAM_ProfileFiles.EnsureDir(RADIO_DIR);
		JsonSaveContext save = new JsonSaveContext();
		save.WriteValue("ids", s_aIndexed);
		HAM_JsonQueue.Add(save, RADIO_INDEX);
	}

	protected static void AppendRows(string identity, array<string> prefabs, array<int> active, array<int> hardware, array<string> freqs, array<string> ranges)
	{
		int count = prefabs.Count();
		if (active.Count() < count)
			count = active.Count();
		if (hardware.Count() < count)
			count = hardware.Count();
		if (freqs.Count() < count)
			count = freqs.Count();
		if (ranges.Count() < count)
			count = ranges.Count();

		for (int i = 0; i < count; i++)
		{
			s_aIds.Insert(identity);
			s_aPrefabs.Insert(prefabs[i]);
			s_aActive.Insert(active[i]);
			s_aHardware.Insert(hardware[i]);
			s_aFreqs.Insert(freqs[i]);
			s_aRanges.Insert(ranges[i]);
		}
	}

	protected static bool ReadIdentityFile(string path, out string identity)
	{
		identity = string.Empty;
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(path))
			return false;

		array<string> prefabs = {};
		array<int> active = {};
		array<int> hardware = {};
		array<string> freqs = {};
		array<string> ranges = {};
		ctx.ReadValue("id", identity);
		ctx.ReadValue("prefabs", prefabs);
		ctx.ReadValue("active", active);
		ctx.ReadValue("hardware", hardware);
		ctx.ReadValue("freqs", freqs);
		ctx.ReadValue("ranges", ranges);
		if (identity.IsEmpty())
			return false;

		AppendRows(identity, prefabs, active, hardware, freqs, ranges);
		return true;
	}

	protected static void Load()
	{
		if (FileIO.FileExists(RADIO_INDEX))
		{
			JsonLoadContext ctx = new JsonLoadContext();
			if (ctx.LoadFromFile(RADIO_INDEX))
			{
				array<string> ids = {};
				ctx.ReadValue("ids", ids);
				foreach (string identity : ids)
				{
					string path = RADIO_DIR + "Joueur_" + HAM_ProfileFiles.SafeId(identity) + "/frequences.json";
					string readId;
					ReadIdentityFile(path, readId);
				}

				return;
			}
		}

		if (!FileIO.FileExists(FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE))
			return;

		array<string> ids = {};
		array<string> prefabs = {};
		array<int> active = {};
		array<int> hardware = {};
		array<string> freqs = {};
		array<string> ranges = {};
		ctx.ReadValue("ids", ids);
		ctx.ReadValue("prefabs", prefabs);
		ctx.ReadValue("active", active);
		ctx.ReadValue("hardware", hardware);
		ctx.ReadValue("freqs", freqs);
		ctx.ReadValue("ranges", ranges);
		int count = ids.Count();
		if (prefabs.Count() < count)
			count = prefabs.Count();
		if (active.Count() < count)
			count = active.Count();
		if (hardware.Count() < count)
			count = hardware.Count();
		if (freqs.Count() < count)
			count = freqs.Count();
		if (ranges.Count() < count)
			count = ranges.Count();

		bool folderExists = false;
		for (int n = 0; n < ids.Count(); n++)
		{
			string probe = RADIO_DIR + "Joueur_" + HAM_ProfileFiles.SafeId(ids[n]) + "/frequences.json";
			if (FileIO.FileExists(probe))
				folderExists = true;
		}

		if (folderExists)
		{
			array<string> seen = {};
			for (int k = 0; k < ids.Count(); k++)
			{
				if (seen.Find(ids[k]) >= 0)
					continue;

				seen.Insert(ids[k]);
				string path = RADIO_DIR + "Joueur_" + HAM_ProfileFiles.SafeId(ids[k]) + "/frequences.json";
				if (FileIO.FileExists(path))
				{
					string readId;
					ReadIdentityFile(path, readId);
					TouchIndex(ids[k]);
					continue;
				}

				for (int row = 0; row < count; row++)
				{
					if (ids[row] != ids[k])
						continue;

					s_aIds.Insert(ids[row]);
					s_aPrefabs.Insert(prefabs[row]);
					s_aActive.Insert(active[row]);
					s_aHardware.Insert(hardware[row]);
					s_aFreqs.Insert(freqs[row]);
					s_aRanges.Insert(ranges[row]);
				}

				WriteIdentity(ids[k]);
				TouchIndex(ids[k]);
			}

			return;
		}

		for (int i = 0; i < count; i++)
		{
			s_aIds.Insert(ids[i]);
			s_aPrefabs.Insert(prefabs[i]);
			s_aActive.Insert(active[i]);
			s_aHardware.Insert(hardware[i]);
			s_aFreqs.Insert(freqs[i]);
			s_aRanges.Insert(ranges[i]);
		}

		array<string> unique = {};
		foreach (string identity : s_aIds)
		{
			if (unique.Find(identity) >= 0)
				continue;

			unique.Insert(identity);
			WriteIdentity(identity);
			TouchIndex(identity);
		}
	}
}

//------------------------------------------------------------------------------------------------
// Entrées VON vanilla (CH1…) : ne pas laisser AdjustEntryModif lire/écrire le TX0 live
// quand un canal créé est actif — sinon l'ancien canal affiche et enregistre la nouvelle freq.
//------------------------------------------------------------------------------------------------
modded class SCR_VONEntryRadio
{
	override void AdjustEntry(int modifier)
	{
		if (HAM_VONChannelEntry.Cast(this))
		{
			super.AdjustEntry(modifier);
			return;
		}

		if (!m_RadioTransceiver || modifier == 0)
			return;

		// Canal créé en émission : le réglage se fait sur son entrée, pas sur CH1.
		if (HAM_RadioChannels.CreatedTalkActive(this))
			return;

		int freq = m_RadioTransceiver.GetFrequency();
		int next = HAM_RadioChannels.NextCoarse(freq, modifier, m_RadioTransceiver.GetMinFrequency(), m_RadioTransceiver.GetMaxFrequency());
		if (next == freq)
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY_ERROR);
		else
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY);

		m_iFrequency = next;
		m_RadioTransceiver.SetFrequency(next);
		float shown = Math.Round(HAM_RadioChannels.KhzOf(next) * 0.1) * 0.01;
		m_sText = shown.ToString(3, 1) + " " + LABEL_FREQUENCY_UNITS;
		HAM_RadioChannels.AfterHardwareScroll(this);
	}

	override void AdjustEntryModif(int modifier)
	{
		if (HAM_VONChannelEntry.Cast(this))
		{
			super.AdjustEntryModif(modifier);
			return;
		}

		if (HAM_RadioChannels.HandleHardwareScroll(this, modifier))
			return;

		super.AdjustEntryModif(modifier);
		HAM_RadioChannels.AfterHardwareScroll(this);
	}

	override void SetActive(bool state)
	{
		super.SetActive(state);
		if (HAM_VONChannelEntry.Cast(this))
			return;

		if (state)
			HAM_RadioChannels.SelectHardwareEntry(this);
	}

	override void Update()
	{
		super.Update();

		// Overlay après le vanilla : fige CH1… seulement si un canal créé est actif.
		if (!HAM_VONChannelEntry.Cast(this))
			HAM_RadioChannels.OverlayHardwareEntry(this);
	}
}

modded class SCR_PlayerController
{
	void HAM_RequestRadioAdd()
	{
		Rpc(HAM_RpcRadioAdd);
	}

	void HAM_RequestRadioRemove()
	{
		Rpc(HAM_RpcRadioRemove);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcRadioAdd()
	{
		HAM_RadioChannels.AddForPlayer(GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcRadioRemove()
	{
		HAM_RadioChannels.RemoveForPlayer(GetPlayerId());
	}

	void HAM_RequestTune(string prefab, int index, int khz)
	{
		if (Replication.IsServer())
			HAM_RadioChannels.TuneCreated(GetPlayerId(), prefab, index, khz);
		else
			Rpc(HAM_RpcTune, prefab, index, khz);
	}

	void HAM_RequestSelect(string prefab, int index)
	{
		if (Replication.IsServer())
			HAM_RadioChannels.SelectIndex(GetPlayerId(), prefab, index);
		else
			Rpc(HAM_RpcSelect, prefab, index);
	}

	void HAM_SyncRadio(string prefab, string freqs, int hardware, int active)
	{
		Rpc(HAM_RpcSyncRadio, prefab, freqs, hardware, active);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcTune(string prefab, int index, int khz)
	{
		HAM_RadioChannels.TuneCreated(GetPlayerId(), prefab, index, khz);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcSelect(string prefab, int index)
	{
		HAM_RadioChannels.SelectIndex(GetPlayerId(), prefab, index);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcSyncRadio(string prefab, string freqs, int hardware, int active)
	{
		HAM_RadioChannels.ApplyClientSync(prefab, freqs, hardware, active);
	}
}

class HAM_VONChannelEntry : SCR_VONEntryRadio
{
	protected string m_sPrefab;
	protected int m_iIndex;
	protected int m_iKhz;

	void Setup(string prefab, int index, int khz, BaseTransceiver transceiver, SCR_GadgetComponent gadget)
	{
		m_sPrefab = prefab;
		m_iIndex = index;
		m_iKhz = khz;
		SetCustomLayout("{033302D7C8158EF8}UI/layouts/HUD/VON/VONEntry.layout");
		// Numéro élevé pour ne pas collisionner avec CH1 hardware dans GetTransceiver(n-1).
		SetRadioEntry(transceiver, 100 + index, gadget);
		Enable(true);
		ApplyVisual();
	}

	int GetIndex()
	{
		return m_iIndex;
	}

	void SetKhz(int khz)
	{
		m_iKhz = khz;
		ApplyVisual();
	}

	override void InitEntry()
	{
		if (m_RadioTransceiver)
			SetUsable(m_RadioTransceiver.GetRadio().IsPowered());

		ApplyVisual();
	}

	override string GetDisplayText()
	{
		WriteText();
		return m_sText;
	}

	override void AdjustEntry(int modifier)
	{
		// A/E : ±5,0 MHz sur ce canal créé (la molette reste le pas fin).
		StepBy(modifier, HAM_RadioChannels.COARSE_KHZ);
	}

	override void AdjustEntryModif(int modifier)
	{
		// Molette : réglage fin ±0,1 MHz.
		StepBy(modifier, 0);
	}

	override void SetActive(bool state)
	{
		super.SetActive(state);
		if (state)
			HAM_RadioChannels.UseCreated(m_sPrefab, m_iIndex, m_iKhz);
		else
			HAM_RadioChannels.ReleaseCreated(m_sPrefab);
	}

	// forcedStep à 0 : pas fin du préfab. Sinon le pas demandé, en kHz (5 000 = 5,0 MHz).
	protected void StepBy(int modifier, int forcedStep)
	{
		if (modifier != 0)
		{
			int step = 100;
			int minFreq = 0;
			int maxFreq = 0;
			if (forcedStep > 0)
			{
				step = forcedStep;
			}
			else if (m_RadioTransceiver)
			{
				int resolution = m_RadioTransceiver.GetFrequencyResolution();
				if (resolution > 0 && resolution < 100)
				{
					int times = 100 / resolution;
					if (times * resolution == 100)
						step = 100;
					else
						step = resolution;
				}
				else if (resolution > 0)
				{
					step = resolution;
				}
			}

			if (m_RadioTransceiver)
			{
				minFreq = m_RadioTransceiver.GetMinFrequency();
				maxFreq = m_RadioTransceiver.GetMaxFrequency();
				minFreq = HAM_RadioChannels.KhzOf(minFreq);
				maxFreq = HAM_RadioChannels.KhzOf(maxFreq);
			}

			int next = m_iKhz + modifier * step;
			if (maxFreq > minFreq)
			{
				if (next < minFreq)
					next = minFreq;
				if (next > maxFreq)
					next = maxFreq;
			}

			if (next < 100)
				next = 100;
			if (next > 100000)
				next = 100000;

			if (next == m_iKhz)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY_ERROR);
			else
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY);

			m_iKhz = next;
			if (IsActive())
				HAM_RadioChannels.ApplyTalk(m_sPrefab, m_iKhz, true);

			SendTune();
		}

		ApplyVisual();
	}

	protected void WriteText()
	{
		m_iFrequency = m_iKhz;
		float shown = Math.Round(m_iKhz * 0.1) * 0.01;
		m_sText = shown.ToString(3, 1) + " " + "#AR-VON_FrequencyUnits_MHz";
		SetFrequencyTextOverwrite(m_sText);
		SetChannelText(SCR_VONMenu.GetKnownChannel(m_iKhz));
		SetName(m_sText);
	}

	protected void ApplyVisual()
	{
		WriteText();
		Update();
	}

	protected void SendTune()
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		player.HAM_RequestTune(m_sPrefab, m_iIndex, m_iKhz);
	}
}

modded class SCR_VONController
{
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);
		// Filet de sécurité : si l'InputManager n'existait pas encore au OnGameStart du client.
		HAM_RadioChannels.BindLocal();
		if (!m_VONMenu)
			return;

		SCR_RadialMenu radial = m_VONMenu.GetRadialMenu();
		if (!radial || !radial.IsOpened())
		{
			HAM_RadioChannels.ForgetWheel();
			return;
		}

		HAM_RadioChannels.RememberMenu(m_VONMenu);
		HAM_RadioChannels.EnsureWheel();
	}
}
