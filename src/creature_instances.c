/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_instances.c
 *     creature_instances support functions.
 * @par Purpose:
 *     Functions to creature_instances.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     11 Mar 2010 - 11 Sep 2011
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "creature_instances.h"

#include "globals.h"
#include "bflib_basics.h"

#include "bflib_math.h"
#include "thing_list.h"
#include "thing_creature.h"
#include "thing_effects.h"
#include "thing_traps.h"
#include "thing_stats.h"
#include "creature_control.h"
#include "room_data.h"
#include "map_blocks.h"
#include "spdigger_stack.h"
#include "config_magic.hpp"
#include "gui_soundmsgs.h"
#include "sounds.h"

#include "keeperfx.hpp"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
DLLIMPORT void _DK_process_creature_instance(struct Thing *thing);
DLLIMPORT long _DK_instf_attack_room_slab(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_creature_cast_spell(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_creature_fire_shot(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_damage_wall(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_destroy(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_dig(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_eat(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_fart(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_first_person_do_imp_task(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_pretty_path(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_reinforce(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_tortured(struct Thing *thing, long *param);
DLLIMPORT long _DK_instf_tunnel(struct Thing *thing, long *param);

DLLIMPORT struct InstanceInfo _DK_instance_info[48];
#define instance_info _DK_instance_info
/******************************************************************************/
long instf_attack_room_slab(struct Thing *thing, long *param);
long instf_creature_cast_spell(struct Thing *thing, long *param);
long instf_creature_fire_shot(struct Thing *thing, long *param);
long instf_damage_wall(struct Thing *thing, long *param);
long instf_destroy(struct Thing *thing, long *param);
long instf_dig(struct Thing *thing, long *param);
long instf_eat(struct Thing *thing, long *param);
long instf_fart(struct Thing *thing, long *param);
long instf_first_person_do_imp_task(struct Thing *thing, long *param);
long instf_pretty_path(struct Thing *thing, long *param);
long instf_reinforce(struct Thing *thing, long *param);
long instf_tortured(struct Thing *thing, long *param);
long instf_tunnel(struct Thing *thing, long *param);

const struct NamedCommand creature_instances_func_type[] = {
  {"attack_room_slab",         1},
  {"creature_cast_spell",      2},
  {"creature_fire_shot",       3},
  {"creature_damage_wall",     4},
  {"creature_destroy",         5},
  {"creature_dig",             6},
  {"creature_eat",             7},
  {"creature_fart",            8},
  {"first_person_do_imp_task", 9},
  {"creature_pretty_path",     10},
  {"creature_reinforce",       11},
  {"creature_tortured",        12},
  {"creature_tunnel",          13},
  {"none",                     14},
  {NULL,                       0},
};

Creature_Instf_Func creature_instances_func_list[] = {
  NULL,
  instf_attack_room_slab,
  instf_creature_cast_spell,
  instf_creature_fire_shot,
  instf_damage_wall,
  instf_destroy,
  instf_dig,
  instf_eat,
  instf_fart,
  instf_first_person_do_imp_task,
  instf_pretty_path, //[10]
  instf_reinforce,
  instf_tortured,
  instf_tunnel,
  NULL,
  NULL,
};

/******************************************************************************/

/** Returns creature instance info structure for given instance index.
 *
 * @param inst_idx Instance index.
 * @param func_name Name of the caller function, for logging purposes.
 * @return Instance Info struct is returned.
 */
struct InstanceInfo *creature_instance_info_get_ptr(long inst_idx,const char *func_name)
{
    if ((inst_idx < 0) || (inst_idx >= sizeof(instance_info)/sizeof(instance_info[0])))
    {
        ERRORMSG("%s: Tried to get invalid instance info %ld!",func_name,inst_idx);
        return &instance_info[0];
    }
    return &instance_info[inst_idx];
}

TbBool creature_instance_info_invalid(const struct InstanceInfo *inst_inf)
{
    return (inst_inf < &instance_info[1]);
}

long creature_instance_is_available(struct Thing *thing, long inum)
{
    struct CreatureControl *cctrl;
    TRACE_THING(thing);
    cctrl = creature_control_get_from_thing(thing);
    if (creature_control_invalid(cctrl))
        return 0;
    return cctrl->instances[inum];
}

TbBool instance_is_ranged_weapon(long inum)
{
    switch (inum)
    {
      case 4:
      case 5:
      case 6:
      case 9:
      case 12:
      case 17:
      case 19:
      case 20:
      case 27:
        return true;
        break;
    }
    return false;
}

TbBool creature_has_ranged_weapon(const struct Thing *creatng)
{
    struct CreatureControl *cctrl;
    long inum;
    TRACE_THING(creatng);
    //return _DK_creature_has_ranged_weapon(creatng);
    cctrl = creature_control_get_from_thing(creatng);
    for (inum = 1; inum < CREATURE_INSTANCES_COUNT; inum++)
    {
        if (cctrl->instances[inum] > 0)
        {
            if (instance_is_ranged_weapon(inum))
                return true;
        }
    }
    return false;
}

void process_creature_instance(struct Thing *thing)
{
    struct CreatureControl *cctrl;
    struct InstanceInfo *inst_inf;
    TRACE_THING(thing);
    cctrl = creature_control_get_from_thing(thing);
    if (cctrl->instance_id != CrInst_NULL)
    {
        cctrl->field_D4++;
        if (cctrl->field_D6 == cctrl->field_D4)
        {
            inst_inf = creature_instance_info_get(cctrl->instance_id);
            if (inst_inf->func_cb != NULL)
            {
                inst_inf->func_cb(thing, &inst_inf->field_22);
            }
        }
        if (cctrl->field_D8 == cctrl->field_D4)
        {
            if (cctrl->field_D3)
            {
                cctrl->field_D4--;
                cctrl->field_D3 = 0;
                return;
            }
            cctrl->instance_use_turn[cctrl->instance_id] = game.play_gameturn;
            cctrl->instance_id = CrInst_NULL;
        }
        cctrl->field_D3 = 0;
    }
}

long instf_creature_fire_shot(struct Thing *thing, long *param)
{
    struct CreatureControl *cctrl;
    struct Thing *target;
    int i;
    TRACE_THING(thing);
    cctrl = creature_control_get_from_thing(thing);
    if (cctrl->field_DA <= 0)
    {
        if ((thing->alloc_flags & TAlF_IsControlled) == 0)
            i = 4;
        else
            i = 1;
    }
    else if ((thing->alloc_flags & TAlF_IsControlled) != 0)
    {
        target = thing_get(cctrl->field_DA);
        TRACE_THING(target);
        if (target->class_id == TCls_Object)
            i = 1;
        else
            i = 2;
    }
    else
    {
        target = thing_get(cctrl->field_DA);
        TRACE_THING(target);
        if (target->class_id == TCls_Object)
            i = 1;
        else if (target->owner == thing->owner)
            i = 2;
        else
            i = 4;
    }
    if (cctrl->field_DA > 0)
    {
        target = thing_get(cctrl->field_DA);
        TRACE_THING(target);
    } else
    {
        target = NULL;
    }
    creature_fire_shot(thing, target, *param, 1, i);
    return 0;
}

long instf_creature_cast_spell(struct Thing *thing, long *param)
{
    struct CreatureControl *cctrl;
    struct Thing *trthing;
    struct SpellInfo *spinfo;
    long mgc_idx;
    TRACE_THING(thing);
    cctrl = creature_control_get_from_thing(thing);
    mgc_idx = *param;
    spinfo = get_magic_info(mgc_idx);
    if (spinfo->cast_at_thing)
    {
        trthing = thing_get(cctrl->field_DA);
        if (!thing_is_invalid(trthing))
        {
            creature_cast_spell_at_thing(thing, trthing, mgc_idx, 1);
            return 0;
        }
    }
    creature_cast_spell(thing, mgc_idx, 1, cctrl->target_x, cctrl->target_y);
    return 0;
}

long instf_dig(struct Thing *thing, long *param)
{
    struct CreatureControl *cctrl;
    struct Dungeon *dungeon;
    struct SlabMap *slb;
    long stl_x,stl_y;
    long task_idx,taskkind;
    long dig_damage,gold;
    SYNCDBG(16,"Starting");
    TRACE_THING(thing);
    //return _DK_instf_dig(thing, param);
    cctrl = creature_control_get_from_thing(thing);
    dungeon = get_dungeon(thing->owner);
    task_idx = cctrl->word_91;
    {
      struct MapTask *task;
      task = get_dungeon_task_list_entry(dungeon,task_idx);
      taskkind = task->kind;
      if (task->field_1 != cctrl->word_8F) {
        return 0;
      }
      stl_x = stl_num_decode_x(cctrl->word_8F);
      stl_y = stl_num_decode_y(cctrl->word_8F);
    }
    slb = get_slabmap_for_subtile(stl_x, stl_y);
    if (slabmap_block_invalid(slb)) {
        return 0;
    }
    dig_damage = calculate_damage_did_to_slab_with_single_hit(thing, slb);
    if (slb->health > dig_damage)
    {
        if (slb->kind != SlbT_GEMS)
          slb->health -= dig_damage;
        thing_play_sample(thing, 63 + UNSYNC_RANDOM(6), 100, 0, 3, 0, 2, 256);
        create_effect(&thing->mappos, TngEff_Unknown25, thing->owner);
        if (taskkind == SDDigTask_MineGold)
        {
            gold = calculate_gold_digged_out_of_slab_with_single_hit(dig_damage, thing->owner, cctrl->explevel, slb);
            thing->creature.gold_carried += gold;
            dungeon->lvstats.gold_mined += gold;
        }
        return 0;
    }
    // slb->health <= dig_damage - we're going to destroy the slab
    remove_from_task_list(thing->owner, task_idx);
    if (taskkind == SDDigTask_MineGold)
    {
        gold = calculate_gold_digged_out_of_slab_with_single_hit(slb->health, thing->owner, cctrl->explevel, slb);
        thing->creature.gold_carried += gold;
        dungeon->lvstats.gold_mined += gold;
        mine_out_block(stl_x, stl_y, thing->owner);
        if (dig_has_revealed_area(stl_x, stl_y, thing->owner))
        {
            event_create_event_or_update_nearby_existing_event(
                get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                EvKind_AreaDiscovered, thing->owner, 0);
            if (is_my_player_number(thing->owner))
                output_message(SMsg_DugIntoNewArea, 0, true);
        }
    } else
    if (taskkind == SDDigTask_DigEarth)
    {
        dig_out_block(stl_x, stl_y, thing->owner);
        if (dig_has_revealed_area(stl_x, stl_y, thing->owner))
        {
            event_create_event_or_update_nearby_existing_event(
                get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y),
                EvKind_AreaDiscovered, thing->owner, 0);
            if (is_my_player_number(thing->owner))
                output_message(SMsg_DugIntoNewArea, 0, true);
        }
    }
    check_map_explored(thing, stl_x, stl_y);
    thing_play_sample(thing, 72 + UNSYNC_RANDOM(3), 100, 0, 3, 0, 4, 256);
    return 1;
}

long instf_destroy(struct Thing *thing, long *param)
{
    struct Dungeon *dungeon;
    struct Room *room;
    struct SlabMap *slb;
    MapSlabCoord slb_x,slb_y;
    long prev_owner;

    TRACE_THING(thing);
    slb_x = subtile_slab_fast(thing->mappos.x.stl.num);
    slb_y = subtile_slab_fast(thing->mappos.y.stl.num);
    dungeon = get_dungeon(thing->owner);
    slb = get_slabmap_block(slb_x, slb_y);
    room = room_get(slb->room_index);
    prev_owner = slabmap_owner(slb);

    if ( !room_is_invalid(room) && (prev_owner != thing->owner) )
    {
        if (room->field_C > 1)
        {
            room->field_C--;
            return 0;
        }
        clear_dig_on_room_slabs(room, thing->owner);
        if (room->owner == game.neutral_player_num)
        {
            claim_room(room, thing);
        } else
        {
            MapCoord ccor_x,ccor_y;
            ccor_x = subtile_coord_center(room->central_stl_x);
            ccor_y = subtile_coord_center(room->central_stl_y);
            event_create_event_or_update_nearby_existing_event(ccor_x, ccor_y, EvKind_RoomLost, room->owner, 0);
            claim_enemy_room(room, thing);
        }
        thing_play_sample(thing, 76, 100, 0, 3, 0, 2, 256);
        create_effects_on_room_slabs(room, imp_spangle_effects[thing->owner], 0, thing->owner);
        return 0;
    }
    if (slb->health > 1)
    {
        slb->health--;
        return 0;
    }
    if (prev_owner != game.neutral_player_num) {
        struct Dungeon *prev_dungeon;
        prev_dungeon = get_dungeon(prev_owner);
        prev_dungeon->lvstats.territory_lost++;
    }
    decrease_dungeon_area(prev_owner, 1);
    neutralise_enemy_block(thing->mappos.x.stl.num, thing->mappos.y.stl.num, thing->owner);
    remove_traps_around_subtile(slab_subtile_center(slb_x), slab_subtile_center(slb_y), NULL);
    dungeon->lvstats.territory_destroyed++;
    return 1;
}

long instf_attack_room_slab(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_attack_room_slab(thing, param);
}

long instf_damage_wall(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_damage_wall(thing, param);
}

long instf_eat(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_eat(thing, param);
}

long instf_fart(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_fart(thing, param);
}

long instf_first_person_do_imp_task(struct Thing *thing, long *param)
{
    MapSlabCoord slb_x,slb_y;
    long locparam;
    TRACE_THING(thing);
    //return _DK_instf_first_person_do_imp_task(thing, param);
    slb_x = subtile_slab_fast(thing->mappos.x.stl.num);
    slb_y = subtile_slab_fast(thing->mappos.y.stl.num);
    if ( check_place_to_pretty_excluding(thing, slb_x, slb_y) )
    {
        instf_pretty_path(thing, NULL);
    } else
    {
        locparam = 23;
        instf_creature_fire_shot(thing, &locparam);
    }
    return 1;
}

long instf_pretty_path(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_pretty_path(thing, param);
}

long instf_reinforce(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_reinforce(thing, param);
}

long instf_tortured(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return 1;
}

long instf_tunnel(struct Thing *thing, long *param)
{
    TRACE_THING(thing);
    return _DK_instf_tunnel(thing, param);
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
