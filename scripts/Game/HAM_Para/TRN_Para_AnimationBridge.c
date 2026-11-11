// Owns presentation only. Never installs itself as the movement command.
// Each machine supplies its existing replicated flight state to this attachment.
class HAM_Para_ParachuteAnimationBridge : CharacterCommandScripted
{
 protected BaseAnimPhysComponent m_Animation;
 protected TAnimGraphCommand m_Command = -1;
 protected bool m_Attached;
 protected int m_Mode;
 protected int m_Phase;
 protected int m_Direction;
 protected float m_X;
 protected float m_Y;

 void HAM_Para_ParachuteAnimationBridge(BaseAnimPhysComponent pAnimPhysComponent)
 {
  m_Animation = pAnimPhysComponent;
  m_Command = m_Animation.BindCommand("CMD_Gesture_NPC");
 }

 void SetState(int mode, int phase, int direction, float x, float y)
 {
  m_Mode = mode; m_Phase = phase; m_Direction = direction; m_X = x; m_Y = y;
  if (mode == 0) { Release(); return; }
  Update();
 }

 void Update()
 {
  if (!m_Animation || m_Mode == 0 || m_Command == -1) return;
  if (SCR_CharacterCommandLoiter.Cast(m_Animation.GetCommandScripted()))
  {
   // Native loiter has taken ownership of NPC; stop sending our packets.
   m_Attached = false;
   return;
  }
  if (!m_Attached)
  {
   // A native scripted action may already own the NPC attachment.
   if (m_Animation.GetCommandScripted()) return;
   ResourceName graph = "{A97A1DFE4A1C06AD}anims/GMS/ParachuteRuntime/Flight.agr";
   ResourceName instance = "{F26ABF7678CC2773}anims/GMS/ParachuteRuntime/Flight.asi";
   PreAnim_SetAttachment("NPC", graph, instance, "MasterControl");
   m_Attached = true;
  }
  // CallCommand4I packs four signed bytes. Read Ia, never the packed I.
  // Steering uses 1% steps; recovery retains X as float and Z in signed mm.
  int c = Math.Round(Math.Clamp(m_X, -1, 1) * 100);
  int d = Math.Round(Math.Clamp(m_Y, -1, 1) * 100);
  float f = 0;
  if (m_Mode == 2 && m_Phase == 3)
  {
   int z = Math.Round(Math.Clamp(m_Y, -3, 3) * 1000);
   c = z & 255;
   d = (z >> 8) & 255;
   f = m_X;
  }
  m_Animation.CallCommand4I(m_Command, 100 + m_Mode, m_Phase * 32 + m_Direction, c, d, f);
 }

 void Release()
 {
  m_Mode = 0;
  if (!m_Attached) return;
  // Do not clear an attachment installed by a subsequently started native action.
  if (m_Animation && !SCR_CharacterCommandLoiter.Cast(m_Animation.GetCommandScripted()))
   PreAnim_SetAttachment("NPC", "", "", "");
  m_Attached = false;
 }
}
