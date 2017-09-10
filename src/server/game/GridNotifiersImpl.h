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

#ifndef TRINITY_GRIDNOTIFIERSIMPL_H
#define TRINITY_GRIDNOTIFIERSIMPL_H

#include "GridNotifiers.h"
#include "WorldPacket.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "ObjectAccessor.h"

inline void
Trinity::ObjectUpdater::Visit(CreatureMapType &m)
{
    for(auto & iter : m)
        if(iter.GetSource()->IsInWorld() && !iter.GetSource()->IsSpiritService())
            iter.GetSource()->Update(i_timeDiff);
}

template<class T>
inline void Trinity::VisibleNotifier::Visit(GridRefManager<T> &m)
{
	for (typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
	{
		vis_guids.erase(iter->GetSource()->GetGUID());
		i_player.UpdateVisibilityOf(iter->GetSource(), i_data, i_visibleNow);
	}
}

inline void Trinity::DynamicObjectUpdater::VisitHelper(Unit* target)
{
    if(!target->IsAlive())
        return;

    // (for not self casting case)
    if(target != i_check && !target->IsAttackableByAOE())
        return;

    if (!i_dynobject.IsWithinDistInMap(target, i_dynobject.GetRadius()))
        return;

    if (i_dynobject.IsAffecting(target))
        return;
    
    //return if target is friendly. Start combat if not.
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i_dynobject.GetSpellId());
    if (!spellInfo)
        return;

    uint32 eff_index = i_dynobject.GetEffIndex();
    if(spellInfo->Effects[eff_index].TargetB.GetTarget() == TARGET_DEST_DYNOBJ_ALLY
        || spellInfo->Effects[eff_index].TargetB.GetTarget() == TARGET_UNIT_DEST_AREA_ALLY)
    {
        if(!i_check->IsFriendlyTo(target))
            return;
    }
    else 
    {
        if(i_check->GetTypeId()==TYPEID_PLAYER )
        {
            if (i_check->IsFriendlyTo( target ))
                return;
        } else {
            if (!i_check->IsHostileTo( target ))
                return;
        }

        if (  spellInfo->HasInitialAggro() )
           i_check->CombatStart(target);
    }
    
    //return if already an area aura that can't stack with ours

    // Check target immune to spell or aura
    if (target->IsImmunedToSpell(spellInfo) || target->IsImmunedToSpellEffect(spellInfo, eff_index))
        return;
    
    // Add dynamic object as source to any existing aura from the same caster, or try to add one if none found
    if(Aura* aur = target->GetAuraByCasterSpell(spellInfo->Id,eff_index,i_check->GetGUID()))
    {
        if(PersistentAreaAura* pAur = dynamic_cast<PersistentAreaAura*>(aur))
        {
            pAur->AddSource(&i_dynobject);
            i_dynobject.AddAffected(target);
            return;
        }
    } else {
        auto  aura = new PersistentAreaAura(spellInfo, eff_index, nullptr, target, i_check);
        aura->AddSource(&i_dynobject);
        //also refresh aura duration
        aura->SetAuraDuration(i_dynobject.GetDuration());
        if(target->AddAura(aura))
            i_dynobject.AddAffected(target);
    }
}

template<>
inline void
Trinity::DynamicObjectUpdater::Visit(CreatureMapType  &m)
{
    for(auto & itr : m)
        VisitHelper(itr.GetSource());
}

template<>
inline void
Trinity::DynamicObjectUpdater::Visit(PlayerMapType  &m)
{
    for(auto & itr : m)
        VisitHelper(itr.GetSource());
}

// SEARCHERS & LIST SEARCHERS & WORKERS

// WorldObject searchers & workers

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(GameObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(PlayerMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(CreatureMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(CorpseMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(DynamicObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}


template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(GameObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(PlayerMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(CreatureMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(CorpseMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(DynamicObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(PlayerMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for(auto& itr : m)
        if(i_check(itr.GetSource()))
            Insert(itr.GetSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(CreatureMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            Insert(itr.GetSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(CorpseMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            Insert(itr.GetSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(GameObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            Insert(itr.GetSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(DynamicObjectMapType &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            Insert(itr.GetSource());
}

// Gameobject searchers

template<class Check>
void Trinity::GameObjectSearcher<Check>::Visit(GameObjectMapType &m)
{
    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::GameObjectLastSearcher<Check>::Visit(GameObjectMapType &m)
{
    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::GameObjectListSearcher<Check>::Visit(GameObjectMapType &m)
{
    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            i_objects.push_back(itr.GetSource());
}

// Unit searchers

template<class Check>
void Trinity::UnitSearcher<Check>::Visit(CreatureMapType &m)
{
    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::UnitSearcher<Check>::Visit(PlayerMapType &m)
{
    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::UnitLastSearcher<Check>::Visit(CreatureMapType &m)
{
    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::UnitLastSearcher<Check>::Visit(PlayerMapType &m)
{
    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::UnitListSearcher<Check>::Visit(PlayerMapType &m)
{
    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            i_objects.push_back(itr.GetSource());
}

template<class Check>
void Trinity::UnitListSearcher<Check>::Visit(CreatureMapType &m)
{
    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            i_objects.push_back(itr.GetSource());
}

template<class Check>
void Trinity::PlayerListSearcher<Check>::Visit(PlayerMapType &m)
{
    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            i_objects.push_back(itr.GetSource());
}

// Creature searchers

template<class Check>
void Trinity::CreatureSearcher<Check>::Visit(CreatureMapType &m)
{
    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::CreatureLastSearcher<Check>::Visit(CreatureMapType &m)
{
    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Check>
void Trinity::CreatureListSearcher<Check>::Visit(CreatureMapType &m)
{
    for(auto & itr : m)
        if(i_check(itr.GetSource()))
            i_objects.push_back(itr.GetSource());
}

template<class Check>
void Trinity::CreatureListSearcherWithRange<Check>::Visit(CreatureMapType &m)
{
    float range;
    for(auto & itr : m)
        if(i_check(itr.GetSource(), range))
        {
            auto pair = std::make_pair(itr.GetSource(),range);
            i_objects.push_back(pair);
        }
}

template<class Check>
void Trinity::PlayerSearcher<Check>::Visit(PlayerMapType &m)
{
    // already found
    if(i_object)
        return;

    for(auto & itr : m)
    {
        if(i_check(itr.GetSource()))
        {
            i_object = itr.GetSource();
            return;
        }
    }
}

template<class Check>
void Trinity::PlayerLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for (auto & itr : m)
    {
        if (!itr.GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr.GetSource()))
            i_object = itr.GetSource();
    }
}

template<class Builder>
void Trinity::LocalizedPacketDo<Builder>::operator()(Player* p)
{
    LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacket* data;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx + 1 || !i_data_cache[cache_idx])
    {
        if (i_data_cache.size() < cache_idx + 1)
            i_data_cache.resize(cache_idx + 1);

        data = new WorldPacket();

        i_builder(*data, loc_idx);

        i_data_cache[cache_idx] = data;
    }
    else
        data = i_data_cache[cache_idx];

    p->SendDirectMessage(data);
}

template<class Builder>
void Trinity::LocalizedPacketListDo<Builder>::operator()(Player* p)
{
    LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacketList* data_list;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx+1 || i_data_cache[cache_idx].empty())
    {
        if (i_data_cache.size() < cache_idx+1)
            i_data_cache.resize(cache_idx+1);

        data_list = &i_data_cache[cache_idx];

        i_builder(*data_list, loc_idx);
    }
    else
        data_list = &i_data_cache[cache_idx];

    for (auto & i : *data_list)
        p->SendDirectMessage(i);
}

#endif                                                      // TRINITY_GRIDNOTIFIERSIMPL_H
