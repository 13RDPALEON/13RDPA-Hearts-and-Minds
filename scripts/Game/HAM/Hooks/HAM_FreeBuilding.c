//------------------------------------------------------------------------------------------------
// Construction libre en H&M : partout sur l'île, sur tout terrain, sans distance minimale ni maximale.
// Garages, hélipads, QG et toutes les compositions passent par les mêmes contrôles vanilla.
//------------------------------------------------------------------------------------------------

modded class SCR_GameModeCampaign
{
	//! Le Conflit n'autorise l'établissement de bases qu'avec le rôle de commandant, coupé en H&M.
	override bool GetEstablishingBasesEnabled()
	{
		if (HAM_Mission.IsActive())
			return true;

		return super.GetEstablishingBasesEnabled();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingProviderComponent
{
	//! Couvre toute l'île depuis n'importe quel point.
	static const float HAM_FREE_BUILDING_RADIUS = 20000;

	//! Cercle dessiné et déclencheur à l'ouverture de la pelle. Le droit de construire reste le rayon libre.
	static const float HAM_BUILD_ZONE_DRAW = 150;

	protected static bool s_bDrawRadius;

	//------------------------------------------------------------------------------------------------
	static void BeginDrawRadius()
	{
		s_bDrawRadius = true;
	}

	//------------------------------------------------------------------------------------------------
	static void EndDrawRadius()
	{
		s_bDrawRadius = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Les fournisseurs non rattachés à une base (camions de construction) gardent leur rayon :
	//! le Conflit balaie toutes les entités de ce rayon à chaque ouverture.
	override float GetBuildingRadius()
	{
		if (HAM_Mission.IsActive() && GetMilitaryBaseComponent())
		{
			if (s_bDrawRadius)
				return HAM_BUILD_ZONE_DRAW;

			return HAM_FREE_BUILDING_RADIUS;
		}

		return super.GetBuildingRadius();
	}

	//------------------------------------------------------------------------------------------------
	//! Avec un rayon couvrant l'île, n'importe quel ennemi bloquerait la construction.
	override bool ObstrucViewWhenEnemyInRange()
	{
		if (HAM_Mission.IsActive())
			return false;

		return super.ObstrucViewWhenEnemyInRange();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingPlacingObstructionEditorComponent
{
	//! Ni obstacle, ni pente, ni eau : la composition se pose où le joueur la met.
	override bool CanCreate(out ENotification outNotification = -1, out SCR_EPreviewState previewStateToShow = SCR_EPreviewState.PLACEABLE)
	{
		if (!HAM_Mission.IsActive() || m_bSuperiorCanBeCreated)
			return super.CanCreate(outNotification, previewStateToShow);

		m_bCanBeCreated = true;
		previewStateToShow = SCR_EPreviewState.PLACEABLE;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingPlacingHQBaseEditorComponent
{
	//! Un QG se pose sans tâche d'établissement et sans distance minimale avec les autres bases.
	override bool IsNearAnyHQ(out ENotification outNotification = -1)
	{
		if (HAM_Mission.IsActive())
			return false;

		return super.IsNearAnyHQ(outNotification);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingBudgetEditorComponent
{
	//! Sans commandant, aucune tâche d'établissement n'existe : seules la faction et la limite de bases comptent.
	override protected bool CanEstablishBase()
	{
		if (!HAM_Mission.IsActive())
			return super.CanEstablishBase();

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign || !campaign.GetEstablishingBasesEnabled() || !campaign.GetBaseManager())
			return false;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!player || !player.GetFaction())
			return false;

		return campaign.GetBaseManager().CanFactionBuildNewBase(player.GetFaction());
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingAreaMeshComponent
{
	//! Le maillage vanilla s'étend du rayon de la zone. 20 km autour d'un QG sort de Cain
	//! (coin vers 29800, 200, 34922) et l'assertion « Entity out of world bounds » tombe dans GenerateAreaMesh.
	//! Le rayon de construction reste celui du fournisseur : seule la maille visible est ramenée dans le monde.
	override float GetRadius()
	{
		float radius = super.GetRadius();
		if (!HAM_Mission.IsActive())
			return radius;

		return HAM_MeshRadiusInsideWorld(radius);
	}

	//------------------------------------------------------------------------------------------------
	protected float HAM_MeshRadiusInsideWorld(float radius)
	{
		if (radius > SCR_CampaignBuildingProviderComponent.HAM_BUILD_ZONE_DRAW)
			radius = SCR_CampaignBuildingProviderComponent.HAM_BUILD_ZONE_DRAW;

		IEntity ent = GetOwner();
		if (!ent || radius <= 0 || !GetGame())
			return radius;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return radius;

		vector mins;
		vector maxs;
		world.GetBoundBox(mins, maxs);
		if (maxs[0] - mins[0] < 100 || maxs[2] - mins[2] < 100)
			return radius;

		vector center = ent.GetOrigin();
		float margin = 32;
		float roomX = Math.Min(center[0] - mins[0], maxs[0] - center[0]) - margin;
		float roomZ = Math.Min(center[2] - mins[2], maxs[2] - center[2]) - margin;
		float room = roomX;
		if (roomZ < room)
			room = roomZ;

		if (room < 25)
			room = 25;

		if (radius > room)
			radius = room;

		if (radius > SCR_CampaignBuildingProviderComponent.HAM_BUILD_ZONE_DRAW)
			return SCR_CampaignBuildingProviderComponent.HAM_BUILD_ZONE_DRAW;

		return radius;
	}
}

//------------------------------------------------------------------------------------------------
// Une tranchée terminée remonte chaque objet du contour, puis chaque buisson dans un rayon
// égal à la distance du premier enfant. Un contour ou des coordonnées cuites trop grands
// balayent l'île et bloquent la frame.
//------------------------------------------------------------------------------------------------
modded class ACE_Trenches_BuildingCompositionComponent
{
	//! Une fois par composition posée, pas à chaque image de creusement.
	override protected void SetIsCompositionSpawned()
	{
		if (Replication.IsServer())
		{
			IEntity owner = GetOwner();
			string prefab = "?";
			vector pos = vector.Zero;
			if (owner)
			{
				pos = owner.GetOrigin();
				if (owner.GetPrefabData())
					prefab = HAM_Util.PrefabFileName(owner.GetPrefabData().GetPrefabName());
			}

			Print(string.Format("[13RDPA H&M] Tranchée posée : %1 à %2", prefab, pos), LogLevel.NORMAL);
		}

		super.SetIsCompositionSpawned();
	}

	override protected void SnapItemsInOutline()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		IEntity outlineEntity = owner.GetChildren();
		if (!outlineEntity)
			return;

		vector mins;
		vector maxs;
		outlineEntity.GetBounds(mins, maxs);
		vector size = maxs - mins;
		if (size[0] > 80 || size[2] > 80 || size[1] > 30)
		{
			Print(string.Format("[13RDPA H&M] Tranchée : contour trop grand (%1), objets non remontés.", size), LogLevel.WARNING);
			return;
		}

		super.SnapItemsInOutline();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingLayoutComponent
{
	override protected void DestroyFoliage()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		IEntity child = owner.GetChildren();
		if (!child)
			return;

		float reach = vector.Distance(child.GetOrigin(), owner.GetOrigin());
		if (reach > 40)
		{
			Print(string.Format("[13RDPA H&M] Tranchée : végétation non détruite, premier élément à %1 m", reach), LogLevel.WARNING);
			return;
		}

		super.DestroyFoliage();
	}
}
