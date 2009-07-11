/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Leotheras_The_Blind
SD%Complete: 100
SDComment: 
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"
#include "Player.h"

enum
{
    SAY_AGGRO               = -1548009,
    SAY_SWITCH_TO_DEMON     = -1548010,
    SAY_INNER_DEMONS        = -1548011,
    SAY_DEMON_SLAY1         = -1548012,
    SAY_DEMON_SLAY2         = -1548013,
    SAY_DEMON_SLAY3         = -1548014,
    SAY_NIGHTELF_SLAY1      = -1548015,
    SAY_NIGHTELF_SLAY2      = -1548016,
    SAY_NIGHTELF_SLAY3      = -1548017,
    SAY_FINAL_FORM          = -1548018,
    SAY_FREE                = -1548019,
    SAY_DEATH               = -1548020,

    SPELL_ENRAGE            = 26662,

    SPELL_WHIRLWIND         = 37640,
    SPELL_CHAOS_BLAST       = 37674,
    SPELL_INSIDIOUS_WHISPER = 37676,                        //not implemented yet. After cast (spellHit), do the inner demon
    SPELL_CONS_MADNESS      = 37749,

    SPELL_DEMON_ALIGNMENT   = 37713,                        //inner demon have this aura
    SPELL_SHADOW_BOLT       = 39309,                        //inner demon spell spam
    SPELL_SOUL_LINK         = 38007,

    FACTION_DEMON_1         = 1829,
    FACTION_DEMON_2         = 1830,
    FACTION_DEMON_3         = 1831,
    FACTION_DEMON_4         = 1832,
    FACTION_DEMON_5         = 1833,

    MODEL_NIGHTELF          = 20514,
    MODEL_DEMON             = 20125,

    NPC_INNER_DEMON         = 21857,
    NPC_SHADOW_LEO          = 21875
};

class MANGOS_DLL_DECL InsidiousAura : public Aura
{
public:
	InsidiousAura (SpellEntry *spell, uint32 eff, int32 *bp, Unit *target, Unit *caster) : Aura(spell, eff, bp, target, caster, NULL)
	{}
};

struct MANGOS_DLL_DECL mob_inner_demonAI : public ScriptedAI
{
    mob_inner_demonAI (Creature *c) : ScriptedAI(c)
    {
        victimGUID = 0;
        Reset();
    }
 
    uint32 ShadowBolt_Timer;
    uint32 Link_Timer;

    uint64 victimGUID;

    void Reset()
    {
        ShadowBolt_Timer = 10000;		
        Link_Timer = 1000;
    }
    void JustDied(Unit *victim)
    {
        Unit* pUnit = Unit::GetUnit((*m_creature),victimGUID);
        if (pUnit && pUnit->HasAura(SPELL_INSIDIOUS_WHISPER,0))
            pUnit->RemoveAurasDueToSpell(SPELL_INSIDIOUS_WHISPER);
    }
    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(done_by->GetGUID() != victimGUID && done_by->GetGUID() != m_creature->GetGUID())
        {
            damage = 0;
            m_creature->getThreatManager().modifyThreatPercent(done_by, -100);
        }
    }
    void Aggro(Unit *who)
    {
        if (!victimGUID) return;
    }
    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        if (m_creature->getVictim()->GetGUID() != victimGUID)
        {
            if (Unit* owner = Unit::GetUnit((*m_creature),victimGUID))
                AttackStart(owner);
        }
        if(Link_Timer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_SOUL_LINK, true);
            Link_Timer = 1000;
        }else Link_Timer -= diff;

        if(!m_creature->HasAura(SPELL_DEMON_ALIGNMENT, 0))
            DoCast(m_creature, SPELL_DEMON_ALIGNMENT,true);

        if(ShadowBolt_Timer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_SHADOW_BOLT, false);
            ShadowBolt_Timer = 10000;
        }else ShadowBolt_Timer -= diff; 

        DoMeleeAttackIfReady();
    }
};

//Original Leotheras the Blind AI
struct MANGOS_DLL_DECL boss_leotheras_the_blindAI : public ScriptedAI
{
    boss_leotheras_the_blindAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_uiShadowLeo = 0;
        Reset();
    }

    ScriptedInstance* m_pInstance;                          // the instance

    // timers
    uint32 m_uiWhirlwind_Timer;
    uint32 m_uiInnerDemon_Timer;
    uint32 m_uiSwitch_Timer;
    uint32 m_uiEnrage_Timer;

    bool m_bDemonForm;
    bool m_bIsFinalForm;
    bool WhirlwindStarted;    

    uint64 m_uiShadowLeo;
	uint64 InnderDemon[5];
	uint32 InnderDemon_Count;

    void Reset()
    {
        m_uiWhirlwind_Timer  = 15000;
        m_uiInnerDemon_Timer = 15000;
        m_uiSwitch_Timer     = 45000;
        m_uiEnrage_Timer     = MINUTE*10*IN_MILISECONDS;

        m_bDemonForm   = false;
        m_bIsFinalForm = false;
        WhirlwindStarted = false;    

        InnderDemon_Count = 0;
        m_creature->SetSpeed( MOVE_RUN, 1.3f, true);
        
        if (m_creature->GetDisplayId() != MODEL_NIGHTELF)
            m_creature->SetDisplayId(MODEL_NIGHTELF);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LEOTHERAS_EVENT, NOT_STARTED);
			// QUI AGGIUNGERE RESPAWN GreyheartSpellbinder
    }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LEOTHERAS_EVENT, IN_PROGRESS);
    }

    void KilledUnit(Unit* pVictim)
    {
        if (pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        switch(rand()%3)
        {
            case 0: DoScriptText(m_bDemonForm ? SAY_DEMON_SLAY1 : SAY_NIGHTELF_SLAY1, m_creature); break;
            case 1: DoScriptText(m_bDemonForm ? SAY_DEMON_SLAY2 : SAY_NIGHTELF_SLAY2, m_creature); break;
            case 2: DoScriptText(m_bDemonForm ? SAY_DEMON_SLAY3 : SAY_NIGHTELF_SLAY3, m_creature); break;
        }
    }
	
	//Despawn all Inner Demon summoned
    void DespawnDemon()
    {
        for(int i=0; i<5; i++)
        {
            if (InnderDemon[i])
            {
                //delete creature
                Unit* pUnit = Unit::GetUnit((*m_creature), InnderDemon[i]);
                if (pUnit && pUnit->isAlive())
     	            pUnit->DealDamage(pUnit, pUnit->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
				InnderDemon[i] = 0;
            }
        }
        InnderDemon_Count = 0;
    }
	
    //remove this once SPELL_INSIDIOUS_WHISPER is supported by core
    void CastConsumingMadness()	
    {
        for (int i=0; i<5; i++)
        {
            if (InnderDemon[i] > 0 )
            {
                    Unit* pUnit = Unit::GetUnit((*m_creature), InnderDemon[i]);
                    if (pUnit && pUnit->isAlive())
                    {
                        Unit* pUnit_target = Unit::GetUnit((*pUnit), ((mob_inner_demonAI *)((Creature *)pUnit)->AI())->victimGUID);
                        if( pUnit_target && pUnit_target->isAlive())
                        {
                            pUnit->CastSpell(pUnit_target, SPELL_CONS_MADNESS, true);
                            m_creature->getThreatManager().modifyThreatPercent(pUnit_target, -100);
                        }
                    }
            }
        }
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (m_creature->getVictim() && pSummoned->GetEntry() == NPC_SHADOW_LEO)
        {
            m_uiShadowLeo = pSummoned->GetGUID();
            pSummoned->AI()->AttackStart(m_creature->getVictim());
        }
    }

    void JustDied(Unit* pVictim)
    {
        DoScriptText(SAY_DEATH, m_creature);

        //despawn copy
        if (m_uiShadowLeo)
        {
            if (Creature* pShadowLeo = (Creature*)Unit::GetUnit((*m_creature), m_uiShadowLeo))
                pShadowLeo->ForcedDespawn();
        }

        if (m_pInstance)
            m_pInstance->SetData(TYPE_LEOTHERAS_EVENT, DONE);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;
			
        // reseting after ending whirlwind
        if (WhirlwindStarted && !m_creature->HasAura(SPELL_WHIRLWIND, 0))
        {      
            m_uiWhirlwind_Timer =  15000;
            WhirlwindStarted = false;
            DoResetThreat();
            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
        }	

        if (!m_bDemonForm)
        {
            //Whirlwind_Timer
            if (m_uiWhirlwind_Timer < uiDiff)
            {
                if (!m_creature->HasAura(SPELL_WHIRLWIND, 0))
                {
                    DoCast(m_creature, SPELL_WHIRLWIND);
                    WhirlwindStarted = true;
                }
                else
                {
                    if ( Unit *newTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                    {
                        DoResetThreat();                               
                        m_creature->GetMotionMaster()->Clear();
                        m_creature->GetMotionMaster()->MovePoint(0,newTarget->GetPositionX(),newTarget->GetPositionY(),newTarget->GetPositionZ());
                    }
                }
                m_uiWhirlwind_Timer = 2000;	
            } else m_uiWhirlwind_Timer -= uiDiff;

            //Switch_Timer
            if (!m_bIsFinalForm)
            {
                if (m_uiSwitch_Timer < uiDiff)
                {
                    DoScriptText(SAY_SWITCH_TO_DEMON, m_creature);

                    if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == TARGETED_MOTION_TYPE)
                    {
                        //set false, so MoveChase is not triggered in AttackStart
                        SetCombatMovement(false);

                        m_creature->GetMotionMaster()->Clear(false);
                        m_creature->GetMotionMaster()->MoveIdle();
                        m_creature->StopMoving();
                    }

                    //switch to demon form
                    m_creature->SetDisplayId(MODEL_DEMON);
                    DoResetThreat();
                    m_bDemonForm = true;

                    m_uiInnerDemon_Timer = 15000;
                    m_uiSwitch_Timer = 60000;
                }else m_uiSwitch_Timer -= uiDiff;
            }
        }
        else
        {
            /*//inner demon
            if (m_uiInnerDemon_Timer < uiDiff)
            {
                DoScriptText(SAY_INNER_DEMONS, m_creature);

                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                DoCast(m_creature, SPELL_INSIDIOUS_WHISPER);

                m_uiInnerDemon_Timer = 60000;
            }else m_uiInnerDemon_Timer -= uiDiff;*/

            //inner demon
            if (m_uiInnerDemon_Timer < uiDiff)
            {
                DoScriptText(SAY_INNER_DEMONS, m_creature);

                std::list<HostilReference *>& ThreatList = m_creature->getThreatManager().getThreatList();
                std::vector<Unit *> TargetList; 
                for (std::list<HostilReference *>::iterator itr = ThreatList.begin(); itr != ThreatList.end(); ++itr)
                {
                    Unit *tempTarget = Unit::GetUnit(*m_creature, (*itr)->getUnitGuid());
                    if (tempTarget && tempTarget->GetTypeId() == TYPEID_PLAYER && tempTarget->GetGUID() != m_creature->getVictim()->GetGUID() && TargetList.size()<5)
                	    TargetList.push_back( tempTarget );
                }
                SpellEntry *spell = (SpellEntry *)GetSpellStore()->LookupEntry(SPELL_INSIDIOUS_WHISPER);
                for (std::vector<Unit *>::iterator itr = TargetList.begin(); itr != TargetList.end(); ++itr)
                {
                    if ((*itr) && (*itr)->isAlive() )
                    {
                        Creature * demon = (Creature *)m_creature->SummonCreature(NPC_INNER_DEMON, (*itr)->GetPositionX()+10, (*itr)->GetPositionY()+10, (*itr)->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                        if (demon)
                        {
                            ((ScriptedAI *)demon->AI())->AttackStart( (*itr) );
                            ((mob_inner_demonAI *)demon->AI())->victimGUID = (*itr)->GetGUID();

                            for (int i=0; i<3; i++)
                            {
                                if (!spell->Effect[i])
                                    continue;
                                (*itr)->AddAura(new InsidiousAura(spell, i, NULL, (*itr), (*itr)));
                            }
                            if (InnderDemon_Count > 4) InnderDemon_Count = 0;

                            //Safe storing of creatures
                            InnderDemon [InnderDemon_Count] = demon->GetGUID();

                            //Update demon count
                            InnderDemon_Count++;
                        }
                    }
                }
                m_uiInnerDemon_Timer = 60000;
            }else m_uiInnerDemon_Timer -= uiDiff;
			
            //chaos blast spam
            if (!m_creature->IsNonMeleeSpellCasted(false))
                m_creature->CastSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, false);
				//m_creature->CastCustomSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, 100, NULL, NULL, false, NULL, NULL, m_creature->GetGUID());  

            //Switch_Timer
            if (m_uiSwitch_Timer < uiDiff)
            {
                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                //switch to nightelf form
                m_creature->SetDisplayId(MODEL_NIGHTELF);

                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != TARGETED_MOTION_TYPE)
                    m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());

                //set true
                SetCombatMovement(true);

                DoResetThreat();
                m_bDemonForm = false;

                m_uiWhirlwind_Timer = 18500;
                m_uiSwitch_Timer = 45000;
            }else m_uiSwitch_Timer -= uiDiff;
        }

        if (!m_bIsFinalForm && (m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) < 15)
        {
            DoScriptText(SAY_FINAL_FORM, m_creature);

            //at this point he divides himself in two parts
            m_creature->SummonCreature(NPC_SHADOW_LEO, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);

            if (m_bDemonForm)
            {
                if (m_creature->IsNonMeleeSpellCasted(false))
                    m_creature->InterruptNonMeleeSpells(false);

                //switch to nightelf form
                m_creature->SetDisplayId(MODEL_NIGHTELF);

                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != TARGETED_MOTION_TYPE)
                    m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());

                //set true
                SetCombatMovement(true);

                DoResetThreat();
                m_bDemonForm = false;
            }

            m_bIsFinalForm = true;
        }

        //m_uiEnrage_Timer
        if (m_uiEnrage_Timer < uiDiff)
        {
            if (m_creature->IsNonMeleeSpellCasted(false))
                m_creature->InterruptNonMeleeSpells(false);

            DoCast(m_creature, SPELL_ENRAGE);
            m_uiEnrage_Timer = MINUTE*5*IN_MILISECONDS;
        }else m_uiEnrage_Timer -= uiDiff;

        if (!m_bDemonForm)
            DoMeleeAttackIfReady();
    }
};

//Leotheras the Blind Demon Form AI
struct MANGOS_DLL_DECL boss_leotheras_the_blind_demonformAI : public ScriptedAI
{
    boss_leotheras_the_blind_demonformAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        SetCombatMovement(false);
        Reset();
    }

    void Reset() { }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_FREE, m_creature);
    }

    void KilledUnit(Unit* pVictim)
    {
        if (pVictim->GetTypeId() != TYPEID_PLAYER)
            return;

        switch(rand()%3)
        {
            case 0: DoScriptText(SAY_DEMON_SLAY1, m_creature); break;
            case 1: DoScriptText(SAY_DEMON_SLAY2, m_creature); break;
            case 2: DoScriptText(SAY_DEMON_SLAY3, m_creature); break;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        if (!m_creature->IsNonMeleeSpellCasted(false))
            m_creature->CastSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, false);

        //Do NOT deal any melee damage to the target.
    }
};

CreatureAI* GetAI_boss_leotheras_the_blind(Creature* pCreature)
{
    return new boss_leotheras_the_blindAI(pCreature);
}

CreatureAI* GetAI_boss_leotheras_the_blind_demonform(Creature* pCreature)
{
    return new boss_leotheras_the_blind_demonformAI(pCreature);
}

CreatureAI* GetAI_mob_inner_demon(Creature *_Creature)
{
	return new mob_inner_demonAI (_Creature);
}

void AddSC_boss_leotheras_the_blind()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_leotheras_the_blind";
    newscript->GetAI = &GetAI_boss_leotheras_the_blind;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_leotheras_the_blind_demonform";
    newscript->GetAI = &GetAI_boss_leotheras_the_blind_demonform;
    newscript->RegisterSelf();
	
	newscript = new Script;
    newscript->Name = "mob_inner_demon";
    newscript->GetAI = &GetAI_mob_inner_demon;
    newscript->RegisterSelf();
}
