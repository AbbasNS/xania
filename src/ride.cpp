/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#include "ride.hpp"
#include "Char.hpp"
#include "comm.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "skills.hpp"

namespace {

void char_ride(Char *ch, Char *ridee) {
    AFFECT_DATA af;

    act("You leap gracefully onto $N.", ch, nullptr, ridee, To::Char);
    act("$n swings up gracefully onto the back of $N.", ch, nullptr, ridee, To::Room);
    ch->riding = ridee;
    ridee->ridden_by = ch;

    af.type = gsn_ride;
    af.level = ridee->level;
    af.duration = -1;
    af.location = AffectLocation::Damroll;
    af.modifier = (ridee->level / 10) + (get_curr_stat(ridee, Stat::Dex) / 8);
    affect_to_char(ch, af);
    af.location = AffectLocation::Hitroll;
    af.modifier = -(((ridee->level / 10) + (get_curr_stat(ridee, Stat::Dex) / 8)) / 4);
    affect_to_char(ch, af);
}

}

void unride_char(Char *ch, Char *pet) {
    act("You swing your leg over and dismount $N.", ch, nullptr, pet, To::Char);
    act("$n smoothly dismounts $N.", ch, nullptr, pet, To::Room);
    pet->ridden_by = nullptr;
    ch->riding = nullptr;

    affect_strip(ch, gsn_ride);
}

void thrown_off(Char *ch, Char *pet) {
    act("|RYou are flung from $N!|w", ch, nullptr, pet, To::Char);
    act("$n is flung from $N!", ch, nullptr, pet, To::Room);
    fallen_off_mount(ch);
}

void fallen_off_mount(Char *ch) {
    Char *pet = ch->riding;
    if (pet == nullptr)
        return;

    pet->ridden_by = nullptr;
    ch->riding = nullptr;

    affect_strip(ch, gsn_ride);
    check_improve(ch, gsn_ride, false, 3);

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    ch->position = Position::Type::Resting;
    damage(pet, ch, number_range(2, 2 + 2 * ch->size + pet->size), &skill_table[gsn_bash], DAM_BASH);
}

void do_ride(Char *ch, const char *argument) {

    char arg[MAX_INPUT_LENGTH];
    Char *ridee;

    if (ch->is_npc())
        return;

    if (ch->get_skill(gsn_ride) == 0) {
        ch->send_line("Huh?");
        return;
    }

    if (ch->riding != nullptr) {
        ch->send_line("You can only ride one mount at a time.");
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        ch->send_line("Ride whom or what?");
        return;
    }

    if ((ridee = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if ((!IS_SET(ridee->act, ACT_CAN_BE_RIDDEN)) || (ridee->master != ch)) {
        act("You can't ride $N!", ch, nullptr, ridee, To::Char);
        return;
    }

    char_ride(ch, ridee);
}

void do_dismount(Char *ch) {
    if (ch->is_npc())
        return;

    if (ch->get_skill(gsn_ride) == 0) {
        ch->send_line("Huh?");
        return;
    }

    if (ch->riding == nullptr) {
        ch->send_line("But you aren't riding anything!");
        return;
    }
    unride_char(ch, ch->riding);
}
