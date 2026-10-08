# ClsMes::Init standalone proposal

`Init__6ClsMesFv` at 0x001F38E0 remains assembly-supplied in dngmenu.
`nd_meswin.hpp` already contains its native inline definition; the normal
unit has no active C++ caller that emits the standalone copy. The guarded
`DngTreeMapInit` inlines that body instead of emitting the function.

A temporary proposal replacing the inline definition in `nd_meswin.hpp`
with `void Init();` and placing the same body out of line in dngmenu.cpp
matches all 174 nonpadding instructions: 0x2B8 bytes, with eight bytes of
retail alignment padding completing the 0x2C0 manifest reservation. An
isolated normal dngmenu object using this proposal also passes the linked
PAL image check against the last complete build. The proposed shared-header
change has not been validated by rebuilding every caller; existing caller
inlining must be preserved or rematched before adopting it.

Both temporary edits were reverted. No shared-header edit or promotion is
retained. The exact header removal patch is in the worktree at
`.private/clsmes-shared-header.patch`; function and isolated-link receipts
are `.private/receipts/clsmes-out-of-line.log` and
`.private/receipts/clsmes-proposed-unit-link.log`. Reconsider when the shared
header owner coordinates standalone emission and validates all affected
callers. This is an emission/ownership blocker, separate from the
placement-new blocker in `DngTreeMapInit`.

The out-of-line body for that coordinated change is:

```cpp
void ClsMes::Init() {
    int name_count;
    int i;

    npc_name_mode = 0;
    char_num = 0;
    text_w = 0;
    text_h = 0;
    page = 0;
    page_num = 0;

    for (i = 0; i < MES_PAGE_MAX; i++) {
        page_chars[i] = 0;
    }

    last_x = 0;
    last_y = 0;
    fade = 0.0f;
    open = 1;
    draw_speed = GetDrawSpeedDef();
    page_wait = 0;
    scroll_wait = 0;
    reveal = 0.0f;
    reveal_num = 0;
    page_top = 0;
    unk_1f4 = 0;
    InitMesWinTbl();
    color = def_color;
    wait = 0;
    page_time = 0;
    page_auto_time = 30;
    mes_no = -1;
    text_ptr = 0;
    alpha = 0x80;
    name_count = 0;

    do {
        memset(name[name_count], 0, MES_NAME_LEN);
        name_count++;
    } while (name_count < MES_NAME_MAX);

    for (int item_index = 0; item_index < MES_ITEM_MAX; item_index++) {
        item_mes[item_index] = -1;
    }

    for (int value_index = 0; value_index < MES_VALUE_MAX; value_index++) {
        values[value_index] = 0;
        value_width[value_index] = 0;
    }

    value = 0;
    value_sign = 0;
    value_zero = 1;
    value_half = 0;
    value_space = 0;
    digit_font = 0;
    space_w = -1;
    justify_w = -1;
    select = -1;
    goal_cursor_x = 0;
    goal_cursor_y = 0;
    cursor_x = 0;
    cursor_y = 0;
    select_shade = MES_SELECT_SHADE_DARK;
    cursor_centering = 0;
    cursor_time = 0;
    choice_pos[0][0] = -1;
    choice_pos[0][1] = -1;
    choice_pos[1][0] = -1;
    choice_pos[1][1] = -1;
    select_top = 0;
    cursor_off_y = 0;
    voice_on = 0;
    voice_type = 0;
    voice_cnt = 0;
    close_time = 0;
    scissor_on = 0;
    scissor.x = 0;
    scissor.width = 0;
    scissor.y = 0;
    scissor.height = 0;
    int line_index;
    line_index = 0;

    do {
        line_indent[line_index] = 0;
        line_pos[line_index][0] = 0;
        line_pos[line_index][1] = 0;
        line_pos_on[line_index] = 0;
        line_shade[line_index] = MES_SHADE_AUTO;
        line_color[line_index] = 0;
        equip_on[line_index] = 0;
        equip_x[line_index] = 0;
        equip_y[line_index] = 0;
        line_w[line_index] = 0;
        line_alpha[line_index] = -1;
        cross_on[line_index] = 0;
        cross_x[line_index] = 0;
        cross_y[line_index] = 0;
        unk_271c[line_index] = -1;
        unk_276c[line_index] = -1;
        unk_27bc[line_index] = 0;
        unk_280c[line_index] = 0;
        delta_on[line_index] = 0;
        delta_x[line_index] = 0;
        delta_y[line_index] = 0;
        line_index++;
    } while (line_index < MES_LINE_MAX);
}
```
