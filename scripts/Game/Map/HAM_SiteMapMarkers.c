//------------------------------------------------------------------------------------------------
// Marqueurs de carte pour les points du stand : véhicules, hélicos, médical, garage, arsenal.
//------------------------------------------------------------------------------------------------

class HAM_SiteMapMarkerComponentClass : ScriptComponentClass
{
}

class HAM_SiteMapMarkerComponent : ScriptComponent
{
	[Attribute("Point")]
	protected string m_sLabel;

	protected static ref array<HAM_SiteMapMarkerComponent> s_aSites;
	protected static bool s_bScheduled;
	protected static bool s_bPlaced;
	protected static int s_iPlaceTries;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!s_aSites)
			s_aSites = {};

		if (s_aSites.Find(this) < 0)
			s_aSites.Insert(this);

		if (s_bPlaced)
		{
			PlaceOne(this);
			return;
		}

		if (s_bScheduled)
			return;

		s_bScheduled = true;
		GetGame().GetCallqueue().CallLater(HAM_PlaceSiteMarkers, 6000, false);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_aSites)
			s_aSites.RemoveItem(this);

		// Dernier point du monde parti : le monde suivant repose ses marqueurs.
		if (!s_aSites || s_aSites.IsEmpty())
		{
			s_bScheduled = false;
			s_bPlaced = false;
			s_iPlaceTries = 0;
		}

		super.OnDelete(owner);
	}

	string GetLabel()
	{
		return m_sLabel;
	}

	static void PlaceAll()
	{
		if (s_bPlaced || !s_aSites)
			return;

		// Le serveur dédié n'a pas la config d'affichage : l'appel y plante.
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			s_bPlaced = true;
			return;
		}

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
		{
			s_iPlaceTries++;
			if (s_iPlaceTries > 10)
			{
				s_bPlaced = true;
				return;
			}

			GetGame().GetCallqueue().CallLater(HAM_PlaceSiteMarkers, 6000, false);
			return;
		}

		s_bPlaced = true;
		int color = MarkerColor(manager);
		foreach (HAM_SiteMapMarkerComponent site : s_aSites)
			PlaceSite(manager, site, color);
	}

	protected static void PlaceOne(HAM_SiteMapMarkerComponent site)
	{
		if (!site || RplSession.Mode() == RplMode.Dedicated)
			return;

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
			return;

		PlaceSite(manager, site, MarkerColor(manager));
	}

	protected static int MarkerColor(SCR_MapMarkerManagerComponent manager)
	{
		int color = 0;
		if (!manager || !manager.GetMarkerConfig())
			return color;

		SCR_MapMarkerEntryPlaced placed = SCR_MapMarkerEntryPlaced.Cast(manager.GetMarkerConfig().GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
		if (!placed || !placed.GetColorEntries())
			return color;

		array<ref SCR_MarkerColorEntry> colors = placed.GetColorEntries();
		for (int i = 0; i < colors.Count(); i++)
		{
			if (!colors[i])
				continue;

			string name = colors[i].GetName();
			name.ToLower();
			if (name.Contains("orange"))
				return i;
		}

		return color;
	}

	protected static void PlaceSite(SCR_MapMarkerManagerComponent manager, HAM_SiteMapMarkerComponent site, int color)
	{
		if (!manager || !site)
			return;

		IEntity owner = site.GetOwner();
		if (!owner)
			return;

		vector origin = owner.GetOrigin();
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetWorldPos(origin[0], origin[2]);
		marker.SetCustomText(site.GetLabel());
		marker.SetColorEntry(color);
		manager.InsertStaticMarker(marker, true, false);
	}
}

void HAM_PlaceSiteMarkers()
{
	HAM_SiteMapMarkerComponent.PlaceAll();
}
