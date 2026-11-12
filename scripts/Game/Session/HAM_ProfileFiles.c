//------------------------------------------------------------------------------------------------
// Dossiers $profile du training : un JSON par objet, pas un fichier unique.
//------------------------------------------------------------------------------------------------

class HAM_ProfileFiles
{
	protected static ref array<string> s_aFound;

	static void EnsureDir(string path)
	{
		if (path.IsEmpty())
			return;

		FileIO.MakeDirectory(path);
	}

	static void ListJson(string directory, out array<string> paths)
	{
		paths = {};
		if (!s_aFound)
			s_aFound = {};

		s_aFound.Clear();
		FileIO.FindFiles(OnFoundFile, directory, ".json");
		foreach (string fileName : s_aFound)
		{
			if (fileName.IsEmpty())
				continue;

			if (fileName.Contains("/") || fileName.Contains("\\"))
				paths.Insert(fileName);
			else
				paths.Insert(directory + fileName);
		}
	}

	static void OnFoundFile(string fileName, FileAttribute attributes = 0, string filesystem = string.Empty)
	{
		if (!s_aFound || fileName.IsEmpty())
			return;

		s_aFound.Insert(fileName);
	}

	static string Token(string prefab)
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

		string clean = "";
		int count = path.Length();
		if (count > 24)
			count = 24;

		string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		for (int i = 0; i < count; i++)
		{
			string ch = path.Substring(i, 1);
			if (letters.IndexOf(ch) >= 0)
				clean = clean + ch;
			else
				clean = clean + "_";
		}

		if (clean.IsEmpty())
			clean = "objet";

		return clean;
	}

	static string SafeId(string identity)
	{
		string clean = "";
		int count = identity.Length();
		if (count > 48)
			count = 48;

		string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		for (int i = 0; i < count; i++)
		{
			string ch = identity.Substring(i, 1);
			if (letters.IndexOf(ch) >= 0)
				clean = clean + ch;
		}

		if (clean.IsEmpty())
			clean = "inconnu";

		return clean;
	}

	static int IndexInName(string fileName, string prefix)
	{
		int at = fileName.LastIndexOf(prefix);
		if (at < 0)
			return 0;

		int start = at + prefix.Length();
		if (start >= fileName.Length())
			return 0;

		string rest = fileName.Substring(start, fileName.Length() - start);
		int cut = rest.IndexOf("_");
		if (cut <= 0)
			return 0;

		string num = rest.Substring(0, cut);
		return num.ToInt();
	}

	//! Le préfixe est coupé pour que le scan de ressources ne prenne pas le dossier pour une ResourceName.
	static string ProfilePath(string rest)
	{
		return "$" + "profile:" + rest;
	}

	static bool IsProfilePath(string path)
	{
		if (path.IsEmpty())
			return false;

		string prefix = "$" + "profile:";
		if (path.IndexOf(prefix) == 0)
			return true;

		if (path.EndsWith(".json"))
			return true;

		return false;
	}

	//! Lecture disque. Le chemin de fichier n'est pas passé à Resource.Load.
	static bool ReadJson(JsonLoadContext ctx, string path)
	{
		if (!ctx || path.IsEmpty() || !FileIO.FileExists(path))
			return false;

		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return false;

		string body;
		file.Read(body, -1);
		file.Close();
		if (body.IsEmpty())
			return false;

		return ctx.LoadFromString(body);
	}

	static string ItemPath(string directory, string prefix, int index, string prefab)
	{
		return directory + prefix + index.ToString() + "_" + Token(prefab) + ".json";
	}
}

//------------------------------------------------------------------------------------------------
// Un seul JSON à la fois, puis 1,5 s. Tri, dépôts, radios et sauvegarde du front passent ici.
class HAM_JsonQueue
{
	protected static const int GAP_MS = 1500;
	protected static ref array<string> s_aPath;
	protected static ref array<string> s_aBody;
	protected static ref array<int> s_aFrontSave;
	protected static bool s_bBusy;
	protected static bool s_bWatch;
	protected static int s_iEpoch;
	protected static int s_iEpochSeen;

	static void Add(JsonSaveContext ctx, string path)
	{
		if (!ctx)
			return;

		Enqueue(ctx.SaveToString(), path, false);
	}

	//! La sauvegarde du front passe devant les petits fichiers déjà en attente.
	static void AddFrontSave(JsonSaveContext ctx, string path)
	{
		if (!ctx)
			return;

		Enqueue(ctx.SaveToString(), path, true);
	}

	static void AddText(string path, string body)
	{
		Enqueue(body, path, false);
	}

	protected static void Enqueue(string body, string path, bool frontSave)
	{
		if (path.IsEmpty())
			return;

		if (!s_aPath)
		{
			s_aPath = {};
			s_aBody = {};
			s_aFrontSave = {};
		}

		int flag = 0;
		if (frontSave)
			flag = 1;

		if (!s_bWatch && GetGame() && GetGame().GetCallqueue())
		{
			s_bWatch = true;
			GetGame().GetCallqueue().CallLater(WatchQueue, 5000, true);
		}

		int at = s_aPath.Find(path);
		if (at >= 0)
		{
			s_aBody.Set(at, body);
			if (flag == 1)
				s_aFrontSave.Set(at, 1);

			if (frontSave && at != 0)
			{
				s_aPath.Remove(at);
				s_aBody.Remove(at);
				s_aFrontSave.Remove(at);
				s_aPath.InsertAt(path, 0);
				s_aBody.InsertAt(body, 0);
				s_aFrontSave.InsertAt(1, 0);
			}

			return;
		}

		if (frontSave)
		{
			s_aPath.InsertAt(path, 0);
			s_aBody.InsertAt(body, 0);
			s_aFrontSave.InsertAt(1, 0);
		}
		else
		{
			s_aPath.Insert(path);
			s_aBody.Insert(body);
			s_aFrontSave.Insert(flag);
		}

		if (!s_bBusy)
			WriteNext();
	}

	// Si le CallLater de la file est perdu, s_bBusy reste vrai et plus rien ne s'écrit.
	protected static void WatchQueue()
	{
		if (!s_aPath || s_aPath.IsEmpty())
			return;

		if (s_iEpoch != s_iEpochSeen)
		{
			s_iEpochSeen = s_iEpoch;
			return;
		}

		s_bBusy = false;
		WriteNext();
		s_iEpochSeen = s_iEpoch;
	}

	protected static void WriteNext()
	{
		s_iEpoch = s_iEpoch + 1;
		if (!s_aPath || s_aPath.IsEmpty())
		{
			s_bBusy = false;
			return;
		}

		s_bBusy = true;
		string path = s_aPath[0];
		string body = s_aBody[0];
		int frontSave = s_aFrontSave[0];
		s_aPath.Remove(0);
		s_aBody.Remove(0);
		s_aFrontSave.Remove(0);

		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			Print("[13RDPA H&M] Échec de l'écriture de " + path, LogLevel.ERROR);
		}
		else
		{
			file.WriteLine(body);
			file.Close();
			if (frontSave == 1)
				HAM_Save.OnJsonSaved();
		}

		if (!s_aPath || s_aPath.IsEmpty())
		{
			s_bBusy = false;
			return;
		}

		GetGame().GetCallqueue().CallLater(WriteNext, GAP_MS, false);
	}
}
