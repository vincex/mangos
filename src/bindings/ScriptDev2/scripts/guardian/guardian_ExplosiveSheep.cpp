#include "precompiled.h"
#include "TargetedMovementGenerator.h"
#include "Pet.h"

// Item: ExplosiveSheep
// Complete: 100%
// Missing: ---

struct MANGOS_DLL_DECL ExplosiveSheepAI : public ScriptedAI
{
	#define EXPLOSIVE 4050

	Unit* owner;
	uint32 Reset_Timer;
	bool charge;
	
	ExplosiveSheepAI(Creature *c) : ScriptedAI(c) 
	{
        owner = m_creature->GetOwner();
		
		if (!m_creature->isPet() || !owner) return;
		
		charge = false;

        CreatureInfo const *info = m_creature->GetCreatureInfo();

		//Ricarico le informazione dal db
        m_creature->SetMaxHealth(info->maxhealth);
        m_creature->SetHealth(info->maxhealth);
	    m_creature->SetArmor(info->armor);
        
		// Danni del db piu formula dei pet (livello +/- (livello/4))
		m_creature->SetStatFloatValue(UNIT_FIELD_MINDAMAGE, info->mindmg + (m_creature->getLevel() - (m_creature->getLevel()/4)));
        m_creature->SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, info->maxdmg + (m_creature->getLevel() + (m_creature->getLevel()/4)));
		m_creature->SetStatFloatValue(UNIT_FIELD_ATTACK_POWER, info->attackpower);	
		m_creature->SetStatFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER , info->attackpower);	
		
		Reset();
	}
	
	void Reset()
	{
	}

	void MoveInLineOfSight(Unit *who)
    {
        if (!m_creature->isPet() || !owner) return;
        
        if( !m_creature->getVictim() && who->isTargetableForAttack() && ( m_creature->IsHostileTo( who )) && who->isInAccessablePlaceFor(m_creature) && owner->IsHostileTo(who))//don't attack not-pvp-flaged
        {
            if (m_creature->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                return;
            
            float attackRadius = m_creature->GetAttackDistance(who);
            if( m_creature->IsWithinDistInMap(who, attackRadius) && m_creature->IsWithinLOSInMap(who) )
            {
                 who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
                    
                 //make them not hit all the same time
				 AttackStart(who);
                 //InCombat = true;
            }
        }
        
    }

	void Aggro(Unit *who){}

	void JustDied(Unit *killer)
	{
		m_creature->SetVisibility(VISIBILITY_OFF);
		m_creature->RemoveFromWorld();
	}

	void UpdateAI(const uint32 diff)
	{
        if (!m_creature->isPet() || !owner) return;

		// Se muore il padrone il pet sparisce
		if(owner->isDead())
		{
			m_creature->setDeathState(JUST_DIED);
			m_creature->SetVisibility(VISIBILITY_OFF);
			m_creature->RemoveFromWorld();
		}

       //Segue il padrone se non � in combat
        if (!m_creature->hasUnitState(UNIT_STAT_FOLLOW)&& !m_creature->isInCombat())
        {
           m_creature->GetMotionMaster()->Clear();
           m_creature->GetMotionMaster()->MoveFollow(owner,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE);
        }
        
        //no victim -> get new from owner (need this because MoveInLineOfSight won't work while following -> corebug)
        if (!m_creature->getVictim())
        {
            if (m_creature->isInCombat()) DoStopAttack();
            //InCombat = false;
            
            if(owner->getAttackerForHelper())
                AttackStart(owner->getAttackerForHelper());

            return;
        }
		
		if (charge)
		{
			m_creature->SetHealth(0);
			m_creature->setDeathState(JUST_DIED);
			m_creature->SetVisibility(VISIBILITY_OFF);
			m_creature->RemoveFromWorld();				
		}

		if (m_creature->getVictim() && m_creature->GetDistance(m_creature->getVictim()->GetPositionX(),m_creature->getVictim()->GetPositionY(),m_creature->getVictim()->GetPositionZ()) < 5)
		{
			if (!charge)
			{
				DoCast(m_creature->getVictim(), EXPLOSIVE);
				charge = true;
			}
		}
	
	}
};

CreatureAI* GetAI_ExplosiveSheep(Creature *_Creature)
{
  	return new ExplosiveSheepAI (_Creature);
}

void AddSC_ExplosiveSheep()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="guardian_ExplosiveSheep";
    newscript->GetAI = GetAI_ExplosiveSheep;
    newscript->RegisterSelf();
}
