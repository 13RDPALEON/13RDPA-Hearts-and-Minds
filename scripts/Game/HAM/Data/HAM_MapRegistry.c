//------------------------------------------------------------------------------------------------
// Registre des cartes. Ajouter une carte : écrire HAM_Map_<Monde>.c, ajouter une ligne ici,
// et mettre la clé dans HAM_MissionComponent (m_sMapKey) sur le mode de jeu du scénario.
//------------------------------------------------------------------------------------------------

class HAM_MapRegistry
{
	static bool Build(string key, notnull HAM_MapDef def)
	{
		def.m_sKey = key;

		if (key == "Cain")
		{
			HAM_Map_Cain.Build(def);
			return true;
		}

		return false;
	}
}
