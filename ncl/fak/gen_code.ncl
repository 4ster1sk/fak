let util = import "util_functions.ncl" in

fun ir =>

let layer_count = std.array.length ir.key_map in

let rec codegen = {
  val = {
    auto = fun value =>
      if std.is_record value then
        struct value
      else if std.is_array value then
        array value
      else
        std.to_string value,
    struct = fun record =>
      "{\n" ++ util.array.join "\n" (
        record
        |> std.record.to_array
        |> std.array.map (fun { field, value } => ".%{field} = %{auto value},")
      ) ++ "\n}",
    array = fun arr => "{ %{util.array.join ", " (std.array.map auto arr)} }"
  },

  defines = fun defines =>
    let codegen_define = fun { field, value } => if value == false then "" else "#define %{field} %{
      if value == true then "" else std.to_string value
    }" in
    defines
    |> std.record.to_array
    |> std.array.map codegen_define
    |> std.array.filter (fun s => s != "")
    |> util.array.join "\n",
  
  pin_def = fun i pin prefix =>
    let port = std.to_string (std.number.truncate (pin / 10)) in
    let port_idx = std.to_string (pin % 10) in
    m%"
      #define %{prefix}%{std.to_string i}_PIN %{port_idx}
      SBIT(%{prefix}%{std.to_string i},      P%{port}_ADDR,        %{prefix}%{std.to_string i}_PIN);
      SBIT(%{prefix}%{std.to_string i}_MODE, P%{port}_MOD_OC_ADDR, %{prefix}%{std.to_string i}_PIN);
    "%,
} in

let h_file = m%"
  %{codegen.defines ir.defines}

  #include "keyboard.h"

  void keyboard_init_user();
  void keyboard_scan_user();
"% in

let side_central_defs =
m%"
  %{
    if layer_count == 0 then
      std.fail_with "No layers. There must be at least one."
    else if layer_count == 1 then
      "__code uint32_t key_map[KEY_COUNT] = %{codegen.val.array (std.array.at 0 ir.key_map)};"
    else
      "__code uint32_t key_map[LAYER_COUNT][KEY_COUNT] = %{codegen.val.array ir.key_map};"
  }

  %{
    if std.array.length ir.hold_tap_behaviors > 0 then
      m%"
        #include "hold_tap.h"
        __code fak_hold_tap_behavior_t hold_tap_behaviors[] = %{codegen.val.array ir.hold_tap_behaviors};
      "%
    else
      "// (No hold tap behaviors)"
  }

  %{
    if std.array.length ir.tap_dance_bindings > 0 then
      "__code uint32_t tap_dance_bindings[] = %{codegen.val.array ir.tap_dance_bindings};"
    else
      "// (No tap dance bindings)"
  }

  %{
    if std.array.length ir.split_periph_key_indices > 0 then
      "__code uint8_t split_periph_key_indices[SPLIT_PERIPH_KEY_COUNT] = %{codegen.val.array ir.split_periph_key_indices};"
    else
      "// (No split periph key indices)"
  }

  %{
    if std.array.length ir.split_periph_encoder_indices > 0 then
      "__code uint8_t split_periph_encoder_indices[SPLIT_PERIPH_ENCODER_COUNT] = %{codegen.val.array ir.split_periph_encoder_indices};"
    else
      "// (No split periph encoder indices)"
  }

  %{
    if std.array.length ir.combo_defs > 0 then
      m%"
        #include "combo.h"
        __code fak_combo_def_t combo_defs[COMBO_COUNT] = %{codegen.val.array ir.combo_defs};
      "%
    else
      "// (No combo defs)"
  }

  %{
    if std.array.length ir.encoder_defs > 0 then
      let defs = std.array.map (fun enc => 
        {
          resolution = enc.resolution,
          key_idx_cw = enc.key_idx_cw,
          key_idx_ccw = enc.key_idx_ccw,
        }) ir.encoder_defs
      in
      m%"
        #include "encoder.h"
        __code fak_encoder_def_t encoder_defs[ENCODER_COUNT] = %{codegen.val.array defs};
      "%
    else
      "// (No encoder defs)"
  }

  %{
    if ir.defines.NEOPIXEL_ENABLE then
    m%"
      #include "neopixel.h"
      #include "keymap.h"
      %{
      if layer_count == 0 then
        std.fail_with "No layers. There must be at least one."
      else if layer_count == 1 then
        "__code uint32_t led_map[LED_COUNT] = %{codegen.val.array (std.array.at 0 ir.led_map)};"
      else
        "__code uint32_t led_map[LAYER_COUNT][LED_COUNT] = %{codegen.val.array ir.led_map};"
      }

      __code void (*layer_hooks[])(const fak_layer_state_t) = {
        %{ if ir.defines.NEOPIXEL_ENABLE then "neopixel_on_layer_state_change," else "" }
        NULL
      };
    "%
    else
      "// (No neopixel defs)"
  }

  %{
    if std.array.length ir.conditional_layers > 0 then
      m%"
        #include "keymap.h"
        __code fak_conditional_layer_def_t conditional_layers[CONDITIONAL_LAYER_COUNT] = %{codegen.val.array ir.conditional_layers};
      "%
    else
      "// (No conditional layers)"
  }

  %{
    if std.array.length ir.macro_steps > 0 then
      m%"
        #include "macro.h"
        __code fak_macro_step_t macro_steps[] = %{codegen.val.array ir.macro_steps};
        __code uint32_t macro_step_args[MACRO_STEP_ARG_COUNT] = %{codegen.val.array ir.macro_step_args};
      "%
    else
      "// (No macros)"
  }
"% in

let c_file =
  let key_state_inform = fun key_idx prefix pin_idx =>
    "key_state_inform(%{std.to_string key_idx}, !%{prefix}%{std.to_string pin_idx});" in

  let set_pin = fun high prefix pin_idx =>
    "%{prefix}%{std.to_string pin_idx} = %{if high then "1" else "0"};" in

  let set_pin_mode = fun mode prefix pin_idx =>
    "%{prefix}%{std.to_string pin_idx}_MODE = %{std.to_string mode};" in

  let pin_defs_for = fun pins prefix => pins
    |> util.array.enumerate
    |> std.array.map (fun { index, value } => codegen.pin_def index value prefix)
    |> util.array.join "\n" in

  let gen_matrix_scan_code = fun col_to_row =>
    let mapping = if col_to_row then ir.kscan.matrix.mapping.col_to_row else ir.kscan.matrix.mapping.row_to_col in
    let ins =  if col_to_row then ir.kscan.matrix.cols else ir.kscan.matrix.rows in
    let outs = if col_to_row then ir.kscan.matrix.rows else ir.kscan.matrix.cols in
    let in_prefix =  if col_to_row then "COL" else "ROW" in
    let out_prefix = if col_to_row then "ROW" else "COL" in

    let out_scan = fun col_to_row out_pin_idx out_mapping =>
      out_mapping
      |> util.array.enumerate
      |> std.array.filter (fun { index, value } => value >= 0)
      |> std.array.map (fun { index, value } => 
          let pin_idx = std.array.at index ins in
          let key_idx = value in
          key_state_inform key_idx in_prefix pin_idx
        )
      |> (fun arr =>
        if std.array.length arr > 0 then
          arr
          |> std.array.prepend "matrix_switch_delay();"
          |> std.array.prepend (set_pin false out_prefix out_pin_idx)
          |> std.array.append  (set_pin true  out_prefix out_pin_idx)
          |> util.array.join "\n"
        else
          ""
      )
    in

    mapping
    |> util.array.enumerate
    |> std.array.map (fun { index, value } => out_scan false (std.array.at index outs) value)
    |> std.array.filter (fun str => std.string.length str > 0)
    |> util.array.join "\n\n"
  in

  let physical_encoders_enumerated =
    ir.encoder_defs
    |> util.array.enumerate
    |> std.array.filter (fun { index, .. } => !(std.array.elem index ir.split_periph_encoder_indices))
  in
m%"
  #include "ch55x.h"

  %{if ir.side != 'peripheral then side_central_defs else ""}

  %{pin_defs_for ir.kscan.ins "IN"}
  %{pin_defs_for ir.kscan.cols "COL"}
  %{pin_defs_for ir.kscan.rows "ROW"}
  %{
    physical_encoders_enumerated
    |> std.array.flat_map (fun { index, value } => [
      codegen.pin_def index value.pin_a "ENC_A",
      codegen.pin_def index value.pin_b "ENC_B",
    ])
    |> util.array.join "\n"
  }
  %{pin_defs_for (
    ir.led_defs
    |> std.array.filter (fun { value, ..} => !(value.type == 'ws2812b && value.data.num > 1)) # Drop chained Neopixel LEDs 
    |> std.array.map (fun { value, .. } => value.data.pin.data)
    ) "LED"}

  void keyboard_init_user() {
  %{
    [[ir.kscan.cols, "COL"], [ir.kscan.rows, "ROW"]]
    |> std.array.flat_map (fun k =>
      let pins = std.array.at 0 k in
      let prefix = std.array.at 1 k in

      pins
      |> std.array.length
      |> std.array.generate (fun i => set_pin true prefix i)
    )
    |> util.array.join "\n"
  }
  // LEDs
  %{
    let prefix = "LED" in 
    ir.led_defs
    # Drop chained Neopixel LEDs 
    |> std.array.filter (fun { value, ..} => !(value.type == 'ws2812b && value.data.num > 1))
    |> std.array.map_with_index (fun i { value, .. } => [set_pin_mode 0 prefix i, set_pin false prefix i])
    |> std.array.flatten
    |> util.array.join "\n"
  }
  %{
    if ir.defines.NEOPIXEL_ENABLE then
      m%"
        // LED test
        neopixel_on_layer_state_change(0);
      "%
    else
      ""
  }
  }

  static void matrix_switch_delay() {
    for (uint8_t i = 16; i; i--) {
      __asm__ ("nop");
    }
  }

  void keyboard_scan_user() {
  // Direct pins
  %{
    ir.kscan.direct
    |> std.array.map (fun { in_idx, key_idx } => key_state_inform key_idx "IN" in_idx)
    |> util.array.join "\n"
  }

  // Matrix (col-to-row)
  %{gen_matrix_scan_code true}

  // Matrix (row-to-col)
  %{gen_matrix_scan_code false}

  // Encoders
  %{
    physical_encoders_enumerated
    |> std.array.map (fun { index, .. } => let i = std.to_string index in
      "encoder_scan(%{i}, ENC_READ(%{i}));")
    |> util.array.join "\n"
  }
  }
"% in

{
  h = h_file,
  c = c_file,
}
