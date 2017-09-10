/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "HostileRefManager.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "DBCStructure.h"
#include "SpellMgr.h"

HostileRefManager::~HostileRefManager()
{
    deleteReferences();
}

//=================================================
// send threat to all my hateres for the pVictim
// The pVictim is hated than by them as well
// use for buffs and healing threat functionality

void HostileRefManager::threatAssist(Unit* pVictim, float pThreat, SpellInfo const *pThreatSpell, bool pSingleTarget, bool skipModifiers)
{
    if(getSize() == 0)
        return;

    if(!skipModifiers)
        pThreat = ThreatCalcHelper::calcThreat(pVictim, iOwner, pThreat, (pThreatSpell ? pThreatSpell->GetSchoolMask() : SPELL_SCHOOL_MASK_NORMAL), pThreatSpell);

    uint32 size = pSingleTarget ? 1 : getSize();            // if pSingleTarget, do not divide threat
    pThreat /= size;

    HostileReference* ref;
    ref = getFirst();


    while(ref != nullptr)
    {
        if (ThreatCalcHelper::isValidProcess(pVictim, ref->GetSource()->GetOwner(), pThreatSpell))
        {
            if(pVictim == GetOwner())
                ref->addThreat(pThreat);          // It is faster to modify the threat directly if possible
            else
                ref->GetSource()->addThreat(pVictim, pThreat);
        }
        ref = ref->next();
    }
}

//=================================================

void HostileRefManager::addThreatPercent(int32 pValue)
{
    HostileReference* ref = getFirst();
    while(ref != nullptr)
    {
        ref->addThreatPercent(pValue);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is given to the method. The calculation has to be done before

void HostileRefManager::setOnlineOfflineState(bool pIsOnline)
{
    HostileReference* ref = getFirst();
    while(ref != nullptr)
    {
        ref->setOnlineOfflineState(pIsOnline);
        ref = ref->next();
    }
}

//=================================================
// The online / offline status is calculated and set

void HostileRefManager::updateThreatTables()
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        ref->updateOnlineStatus();
        ref = ref->next();
    }
}

//=================================================
// The references are not needed anymore
// tell the source to remove them from the list and free the mem

void HostileRefManager::deleteReferences()
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        ref->removeReference();
        delete ref;
        ref = nextRef;
    }
}
//=================================================
// delete all references out of specified range

void HostileRefManager::deleteReferencesOutOfRange(float range)
{
    HostileReference* ref = getFirst();
    range = range*range;
    while (ref)
    {
        HostileReference* nextRef = ref->next();
        Unit* owner = ref->GetSource()->GetOwner();
        if (!owner->isActiveObject() && owner->GetExactDist2dSq(GetOwner()) > range)
        {
            ref->removeReference();
            delete ref;
        }
        ref = nextRef;
    }
}

//=================================================
// delete one reference, defined by Unit

void HostileRefManager::deleteReference(Unit *pCreature)
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        if(ref->GetSource()->GetOwner() == pCreature)
        {
            ref->removeReference();
            delete ref;
            break;
        }
        ref = nextRef;
    }
}

//=================================================
// set state for one reference, defined by Unit

void HostileRefManager::setOnlineOfflineState(Unit *pCreature,bool pIsOnline)
{
    HostileReference* ref = getFirst();
    while(ref)
    {
        HostileReference* nextRef = ref->next();
        if(ref->GetSource()->GetOwner() == pCreature)
        {
            ref->setOnlineOfflineState(pIsOnline);
            break;
        }
        ref = nextRef;
    }
}

//=================================================


//=================================================

void HostileRefManager::UpdateVisibility()
{
	HostileReference* ref = getFirst();
	while (ref)
	{
		HostileReference* nextRef = ref->next();
		if (!ref->GetSource()->GetOwner()->CanSeeOrDetect(GetOwner()))
		{
			nextRef = ref->next();
			ref->removeReference();
			delete ref;
		}
		ref = nextRef;
	}
}